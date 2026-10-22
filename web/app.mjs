import {attachGameInput} from './game-input.mjs';
import {localFontInputs} from './font-input.mjs';
const tbody = document.querySelector('#results'), log = document.querySelector('#log');
const button = document.querySelector('#run'), summary = document.querySelector('#summary');
let reportURL;
function row(name, status, detail) {
  const tr = document.createElement('tr');
  for (const value of [name, status.toUpperCase(), detail]) {
    const td = document.createElement('td'); td.textContent = value; tr.append(td);
  }
  tr.children[1].className = status; tbody.append(tr);
}
function timeout(promise, label, milliseconds = 30000) {
  if(milliseconds===0)return promise;
  let id;
  return Promise.race([promise, new Promise((_, reject) => { id = setTimeout(() => reject(new Error(`${label}: timeout`)), milliseconds); })]).finally(() => clearTimeout(id));
}
async function core(target) {
  const records = [];
  let resolve, reject;
  const done = new Promise((yes, no) => { resolve = yes; reject = no; });
  const completed = timeout(done, target);
  // Attach rejection immediately, including when module import/instantiation fails.
  completed.catch(() => {});
  const { default: create } = await import(`./${target}.mjs`);
  await timeout(create({
    print(line) {
      log.textContent += `[${target}] ${line}\n`;
      if (line.startsWith('SSBU_')) {
        const at = line.indexOf(' '), kind = line.slice(0, at);
        try {
          const value = JSON.parse(line.slice(at + 1)); records.push({ kind, ...value });
          if (kind.endsWith('_RESULT')) resolve(value);
        } catch (error) { reject(error); }
      }
    },
    printErr: line => { log.textContent += `[${target}:stderr] ${line}\n`; },
    onAbort: reason => reject(new Error(String(reason))),
  }), `${target} initialization`);
  const result = await completed;
  const expectedBits = target.endsWith('64') ? 64 : 32;
  if (result.failures !== 0 || result.pointerBits !== expectedBits || (target.startsWith('core') && !result.checks)) throw new Error(`${target}: ${JSON.stringify(result)}`);
  return { ...result, records };
}
async function gpuProbe() {
  if (!navigator.gpu) throw new Error('WebGPU API unavailable');
  const adapter = await navigator.gpu.requestAdapter();
  if (!adapter) throw new Error('No WebGPU adapter');
  const requiredFeatures = ['texture-compression-bc', 'depth32float-stencil8'].filter(feature => adapter.features.has(feature));
  const device = await adapter.requestDevice({ requiredFeatures });
  const errors = [];
  device.addEventListener('uncapturederror', event => errors.push(event.error.message));
  try {
    device.pushErrorScope('validation');
    const shader = device.createShaderModule({ code: `
      @group(0) @binding(0) var<storage, read_write> values: array<u32>;
      @compute @workgroup_size(64) fn main(@builtin(global_invocation_id) id: vec3<u32>) {
        if (id.x < 64u) { values[id.x] = id.x * 3u + 7u; }
      }` });
    const info = await shader.getCompilationInfo();
    if (info.messages.some(m => m.type === 'error')) throw new Error('WGSL compilation failed');
    const pipeline = await device.createComputePipelineAsync({ layout: 'auto', compute: { module: shader, entryPoint: 'main' } });
    const values = device.createBuffer({ size: 256, usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_SRC });
    const readback = device.createBuffer({ size: 256, usage: GPUBufferUsage.MAP_READ | GPUBufferUsage.COPY_DST });
    const group = device.createBindGroup({ layout: pipeline.getBindGroupLayout(0), entries: [{ binding: 0, resource: { buffer: values } }] });
    const context = document.querySelector('#gpu').getContext('webgpu');
    if (!context) throw new Error('WebGPU canvas unavailable');
    context.configure({ device, format: navigator.gpu.getPreferredCanvasFormat(), alphaMode: 'opaque' });
    const encoder = device.createCommandEncoder();
    const compute = encoder.beginComputePass(); compute.setPipeline(pipeline); compute.setBindGroup(0, group); compute.dispatchWorkgroups(1); compute.end();
    encoder.copyBufferToBuffer(values, 0, readback, 0, 256);
    const render = encoder.beginRenderPass({ colorAttachments: [{ view: context.getCurrentTexture().createView(), clearValue: { r: .12, g: .48, b: .40, a: 1 }, loadOp: 'clear', storeOp: 'store' }] });
    render.end(); device.queue.submit([encoder.finish()]);
    await device.queue.onSubmittedWorkDone();
    await readback.mapAsync(GPUMapMode.READ);
    const numbers = new Uint32Array(readback.getMappedRange());
    for (let i = 0; i < 64; i++) if (numbers[i] !== i * 3 + 7) throw new Error(`WebGPU readback mismatch at ${i}`);
    readback.unmap();
    const validation = await device.popErrorScope();
    if (validation || errors.length) throw new Error(validation?.message || errors.join('; '));
    let shaderCompiler;
    const shaderFixture = await fetch('./shader-test.json', {method:'HEAD'});
    if (shaderFixture.ok) {
      const {testShaderCompiler} = await import('./shader-test.mjs');
      shaderCompiler = await testShaderCompiler(device);
    } else if (shaderFixture.status !== 404) throw new Error(`Shader fixture HTTP ${shaderFixture.status}`);
    let gameShaders;
    let gpuResources;
    let gpuTextures;
    let gpuSamplers;
    let gpuProgramSnapshots;
    let gpuRender;
    let gpuDepth;
    const resourceFixture=await fetch('./gpu-probe.mjs',{method:'HEAD'});
    if(resourceFixture.ok) {
      const {testGpuResources}=await import('./test-gpu-resources.mjs');
      gpuResources=await testGpuResources();
    } else if(resourceFixture.status!==404)throw new Error(`GPU resource fixture HTTP ${resourceFixture.status}`);
    const textureFixture=await fetch('./gpu-texture-probe.mjs',{method:'HEAD'});
    if(textureFixture.ok){
      const {testGpuResources}=await import('./test-gpu-resources.mjs');
      gpuTextures=await testGpuResources({withSdk:true});
    }else if(textureFixture.status!==404)throw new Error(`GPU texture fixture HTTP ${textureFixture.status}`);
    const samplerFixture=await fetch('./test-gpu-samplers.mjs',{method:'HEAD'});
    if(samplerFixture.ok){
      const {testGpuSamplers}=await import('./test-gpu-samplers.mjs');
      gpuSamplers=await testGpuSamplers(device);
    }else if(samplerFixture.status!==404)throw new Error(`Sampler fixture HTTP ${samplerFixture.status}`);
    const renderFixture=await fetch('./test-gpu-render.mjs',{method:'HEAD'});
    if(renderFixture.ok){
      const {testGpuRender}=await import('./test-gpu-render.mjs');
      gpuRender=await testGpuRender(device);
    }else if(renderFixture.status!==404)throw new Error(`Render fixture HTTP ${renderFixture.status}`);
    const depthFixture=await fetch('./test-gpu-depth.mjs',{method:'HEAD'});
    if(depthFixture.ok){
      const {testGpuDepth}=await import('./test-gpu-depth.mjs');
      gpuDepth=await testGpuDepth(device);
    }else if(depthFixture.status!==404)throw new Error(`Depth fixture HTTP ${depthFixture.status}`);
    const gameFixtures=await fetch('./maxwell-fixtures.json',{method:'HEAD'});
    if(gameFixtures.ok) {
      const {testGameShaders}=await import('./test-game-shaders.mjs');
      gameShaders=await testGameShaders(device);
      const snapshotFixture=await fetch('./test-gpu-program-snapshots.mjs',{method:'HEAD'});
      if(snapshotFixture.ok){
        const {testProgramSnapshots}=await import('./test-gpu-program-snapshots.mjs');
        gpuProgramSnapshots=await testProgramSnapshots(device);
      }else if(snapshotFixture.status!==404)throw new Error(`Program snapshot fixture HTTP ${snapshotFixture.status}`);
    } else if(gameFixtures.status!==404)throw new Error(`Maxwell fixtures HTTP ${gameFixtures.status}`);
    const infoAdapter = adapter.info;
    return { vendor: infoAdapter?.vendor, architecture: infoAdapter?.architecture,
      device: infoAdapter?.device, description: infoAdapter?.description,
      fallback: infoAdapter?.isFallbackAdapter ?? adapter.isFallbackAdapter ?? null,
      maxBufferSize: adapter.limits.maxBufferSize, readbackValuesChecked: 64, shaderCompiler, gameShaders, gpuResources, gpuTextures, gpuSamplers, gpuProgramSnapshots, gpuRender, gpuDepth, scope: 'compute/readback, optional NVN resources/commands/samplers and captured shader checks, render-target clear and depth/stencil readbacks; not a game draw' };
  } finally { device.destroy(); }
}
async function run() {
  button.disabled = true; tbody.replaceChildren(); log.textContent = ''; summary.textContent = 'Đang kiểm tra…';
  const report = { timestamp: new Date().toISOString(), userAgent: navigator.userAgent,
    hardwareConcurrency: navigator.hardwareConcurrency, crossOriginIsolated,
    scope: 'Runtime integration checks; game boot and gameplay are not implemented.', results: {}, errors: {} };
  window.__ssbuReport = report; window.__ssbuDone = false;
  try {
    try { report.manifest = await (await fetch('./manifest.json')).json(); } catch (e) { report.errors.manifest = String(e); }
    const isolated = crossOriginIsolated && typeof SharedArrayBuffer === 'function';
    row('Cross-origin isolation + SharedArrayBuffer', isolated ? 'pass' : 'fail', String(isolated));
    if (!isolated) report.errors.isolation = 'Pthreads requires COOP/COEP and SharedArrayBuffer';
    for (const target of report.manifest?.targetsBuiltThisInvocation ?? ['core32', 'core64', 'fiber32']) {
      if (target.startsWith('core') || target.startsWith('game')) {
        if (!isolated) { row(target, 'skip', 'Thiếu cross-origin isolation'); continue; }
      }
      try {
        const result = await core(target); report.results[target] = result;
        if (target.startsWith('game') && result.checks !== report.manifest.game?.cases) throw new Error('Số case game không khớp manifest');
        row(target, 'pass', result.functions ? `${result.functions} hàm từ dump 13.0.4; ${result.checks} trạng thái khớp AArch64; ${result.pointerBits}-bit` : result.checks ? `${result.checks} checks; pointers ${result.pointerBits}-bit` : `Nested fiber + longjmp; pointers ${result.pointerBits}-bit; chỉ cùng thread`);
      } catch (error) { report.errors[target] = String(error); row(target, 'fail', String(error)); }
    }
    try {
      const result = await timeout(gpuProbe(), 'WebGPU'); report.results.webgpu = result;
      row('WebGPU compute + present', 'pass', `${result.description || result.vendor || 'Adapter info unavailable'}; fallback=${result.fallback}`);
      if(result.shaderCompiler)row('Bộ dịch shader Wasm', 'pass', `SPIR-V → WGSL → compute: ${result.shaderCompiler.values.join(', ')}`);
      if(result.gpuResources) {
        const resources=result.gpuResources;
        row('Tài nguyên NVN → WebGPU',resources.ok?'pass':'fail',`${resources.checks} kiểm tra cấp phát, flush/invalidate và readback qua pthread/Worker.`);
        if(!resources.ok)report.errors.gpuResources=resources;
      }
      if(result.gameShaders) {
        const shaders=result.gameShaders;
        row('Shader Maxwell từ game',shaders.ok?'pass':'fail',`${shaders.passed}/${shaders.total} biên dịch bằng Wasm và WebGPU; chưa phải lệnh vẽ game.`);
        if(!shaders.ok)report.errors.gameShaders=shaders.results.filter(item=>!item.ok).map(item=>({name:item.name,error:item.error||item.errors}));
      }
      if(result.gpuSamplers){
        const samplers=result.gpuSamplers;
        row('Sampler NVN → WebGPU (fixture)',samplers.ok?'pass':'fail',`${samplers.checks} giá trị texture/depth khớp; chưa phải lệnh vẽ game.`);
        if(!samplers.ok)report.errors.gpuSamplers=samplers.errors;
      }
      if(result.gpuTextures){
        const textures=result.gpuTextures;
        row('Texture và lệnh NVN (fixture)',textures.ok?'pass':'fail',`${textures.checks} kiểm tra upload/copy/clear, trạng thái queue và readback; chưa phải lệnh từ game.`);
        if(!textures.ok)report.errors.gpuTextures=textures;
      }
      if(result.gpuProgramSnapshots){
        const snapshots=result.gpuProgramSnapshots;
        row('Giữ phiên bản shader (fixture)',snapshots.ok?'pass':'fail',`${snapshots.checks} kiểm tra cập nhật/giải phóng source và xác thực pipeline; chưa dispatch.`);
        if(!snapshots.ok)report.errors.gpuProgramSnapshots=snapshots;
      }
      if(result.gpuRender){
        const rendering=result.gpuRender;
        row('Xóa màu render target (fixture)',rendering.ok?'pass':'fail',`${rendering.checks} kiểm tra pixel, mặt nạ màu, scissor và mip/layer; chưa nhận lệnh từ game.`);
        if(!rendering.ok)report.errors.gpuRender=rendering;
      }
      if(result.gpuDepth){
        const depth=result.gpuDepth;
        row('Depth/stencil NVN → WebGPU (fixture)',depth.ok?'pass':'fail',`${depth.checks} kiểm tra upload/readback, aspect view và partial copy (${depth.formats.length} format); chưa nhận lệnh từ game.`);
        if(!depth.ok)report.errors.gpuDepth=depth;
      }
    } catch (error) { report.errors.webgpu = String(error); row('WebGPU', 'fail', String(error)); }
    report.ok = Object.keys(report.errors).length === 0;
    summary.textContent = report.ok ? report.manifest?.game ? 'Các hàm game đã chạy bằng Wasm và khớp AArch64. Chưa boot toàn bộ game.' : 'Các bài kiểm tra kỹ thuật đã qua — chưa chạy game.' : 'Có bài kiểm tra chưa qua. Xem chi tiết bên dưới.';
    try {
      const response = await fetch('/__report', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(report) });
      if (!response.ok) throw new Error(`HTTP ${response.status}`);
      report.saved = true;
    } catch (error) { report.saved = false; log.textContent += `Không lưu được report trên server: ${error}\n`; }
    if (reportURL) URL.revokeObjectURL(reportURL);
    reportURL = URL.createObjectURL(new Blob([JSON.stringify(report, null, 2)], { type: 'application/json' }));
    const link = document.querySelector('#download'); link.href = reportURL; link.download = 'ssbu-web-probes.json'; link.hidden = false;
  } finally { window.__ssbuDone = true; button.disabled = false; }
}
button.addEventListener('click', run);

function bindLocalFile(selector, kind) {
document.querySelector(selector).addEventListener('change', async (event) => {
  const file = event.target.files[0];
  if (!file) return;
  const status = document.querySelector(kind === 'elf' ? '#elf-status' : '#asset-status');
  const resultKey = kind === 'elf' ? '__ssbuElf' : '__ssbuAssets';
  event.target.disabled = true;
  window[`${resultKey}Done`] = false;
  delete window[`${resultKey}Error`];
  delete window[resultKey];
  let worker;
  try {
    const response = await fetch(kind === 'elf' ? './elf-manifest.json' : './asset-manifest.json');
    if (!response.ok) throw new Error('Chưa có bản build dữ liệu. Chạy prepare_game.py rồi build.py --with-game.');
    const manifest = await response.json();
    if (file.size !== manifest.size) throw new Error(`File cần có ${manifest.size.toLocaleString()} byte, file đã chọn có ${file.size.toLocaleString()} byte.`);
    status.textContent = 'Đang mở file trong worker…';
    worker = new Worker('./assets-worker.mjs', { type: 'module' });
    const result = await timeout(new Promise((resolve, reject) => {
      worker.onmessage = ({ data }) => {
        if (data.type === 'progress') status.textContent = `Đã đọc ${data.completed}/${data.total} đoạn…`;
        if (data.type === 'result') resolve(data.result);
        if (data.type === 'error') reject(new Error(data.message));
      };
      worker.onerror = (error) => reject(new Error(error.message));
      worker.postMessage({ file, manifest, kind });
    }), 'Đọc data.arc', 60000);
    window[resultKey] = result;
    const report = { results: { [kind]: result }, scope: result.scope, userAgent: navigator.userAgent, timestamp: new Date().toISOString() };
    const saved = await fetch(kind === 'elf' ? '/__elf_report' : '/__asset_report', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(report) });
    if (!saved.ok) throw new Error(`Không lưu được báo cáo đọc file: HTTP ${saved.status}`);
    if (!result.ok) throw new Error('Kiểm tra dữ liệu hoặc bộ nhớ không qua. Xem báo cáo.');
    status.textContent = kind === 'elf' ? `Đã nạp ${result.segments} segment, ${(result.bssBytes / 2 ** 20).toFixed(1)} MiB BSS bằng 0, ${result.pages.toLocaleString()} trang bộ nhớ. nninitStartup đã chạy, đăng ký allocator thành công. Chưa chạy constructors hoặc nnMain.` : `Đọc thành công file ${(Number(result.size) / 2 ** 30).toFixed(2)} GiB: ${result.samples.length} đoạn khớp, gồm vị trí vượt 4 GiB. Wasm dùng heap ${result.wasmHeapBytes / 2 ** 20} MiB; không nạp toàn bộ file.`;
  } catch (error) {
    window[`${resultKey}Error`] = String(error);
    status.textContent = `Lỗi: ${error}`;
  } finally {
    worker?.terminate(); event.target.disabled = false;
    window[`${resultKey}Done`] = true;
  }
});
}
bindLocalFile('#archive', 'assets');
bindLocalFile('#elf', 'elf');

let activeRuntime;
const runtimeControls=['#runtime','#boot','#elf','#romfs','#fonts','#run'].map(selector=>document.querySelector(selector));
const stopRuntime=document.querySelector('#stop-runtime');
stopRuntime.addEventListener('click',()=>activeRuntime?.cancel());
window.addEventListener('pagehide',()=>activeRuntime?.cancel());

async function startRuntime(event) {
  if(activeRuntime)return;
  const file = document.querySelector('#elf').files[0];
  const status = document.querySelector('#runtime-status');
  if (!file) { status.textContent = 'Chọn main.elf ở trên trước.'; return; }
  if(document.querySelector('#elf').disabled){status.textContent='Đang kiểm tra main.elf, chờ bước này hoàn tất.';return;}
  if(button.disabled){status.textContent='Đang chạy kiểm tra, chờ bước này hoàn tất.';return;}
  const fonts=Array.from(document.querySelector('#fonts').files);
  try { localFontInputs(fonts,{required:event.target.id==='boot'}); }
  catch(error){status.textContent=String(error.message??error);return;}
  const priorDisabled=runtimeControls.map(control=>control.disabled);
  for(const control of runtimeControls)control.disabled=true;
  stopRuntime.hidden=false;
  window.__ssbuRuntimeDone = false;
  delete window.__ssbuRuntimeError;
  delete window.__ssbuRuntimeDiagnostics;
  delete window.__ssbuRuntimeProgress;
  delete window.__ssbuRuntime;
  delete window.__ssbuDiagnosticMemory;
  delete window.__ssbuAllocationTimeline;
  // Keep a bounded history for opt-in heap/command-growth diagnosis. Retain the
  // first observation and recent samples; never retain full frame/GPU captures.
  const allocationTimeline=[];
  let lastAllocationSample=-Infinity;
  let worker;
  let input;
  let cancelled=false;
  let rejectRun;
  activeRuntime={cancel(){
    if(cancelled)return;
    cancelled=true;worker?.terminate();
    rejectRun?.(new DOMException('Đã dừng runtime.','AbortError'));
  }};
  try {
    const requestedTimeout=new URLSearchParams(location.search).get('timeout');
    const runtimeTimeout=requestedTimeout===null?(event.target.id==='boot'?0:900000):Number(requestedTimeout);
    if(!Number.isFinite(runtimeTimeout)||runtimeTimeout<0)throw new Error('Thời gian chờ runtime không hợp lệ.');
    worker = new Worker('./runtime-worker.mjs', {type:'module'});
    const result = await timeout(new Promise((resolve,reject) => {
      rejectRun=reject;
      worker.onmessage = ({data}) => {
        if (data.type === 'status') status.textContent = data.message;
        if (data.type === 'input-state') input?.bind(data.buffer,data.offset);
        if (data.type === 'memory-diagnostic') window.__ssbuDiagnosticMemory={buffer:data.buffer};
        if (data.type === 'progress') {
          window.__ssbuRuntimeProgress = data.progress;
          const p=data.progress;
          if(p.wasmHeapBytes!==undefined&&p.elapsedMs-lastAllocationSample>=10000){
            lastAllocationSample=p.elapsedMs;
            allocationTimeline.push({elapsedMs:p.elapsedMs,wasmHeapBytes:p.wasmHeapBytes,
              allocationInfo:p.allocationInfo,commandInfo:p.commandInfo,
              importCalls:p.importCalls,draws:p.gpuResources?.draws,framesPresented:p.gpuResources?.framesPresented});
            if(allocationTimeline.length>128)allocationTimeline.splice(1,1);
            window.__ssbuAllocationTimeline=allocationTimeline;
          }
          status.textContent = `Đang chạy (${Math.round(data.progress.elapsedMs / 1000)}s): ${data.progress.constructorsCompleted} ctors, ${data.progress.importCalls.toLocaleString()} imports, fn=${data.progress.fn}, lr=${data.progress.lr}`;
        }
        if (data.type === 'result') resolve(data.result);
        if (data.type === 'error') {window.__ssbuRuntimeDiagnostics=data.diagnostics;reject(new Error(data.message));}
      };
      worker.onerror = error => reject(new Error(error.message));
      const selected = Array.from(document.querySelector('#romfs').files);
      const romfs = selected.map(file => ({name:file.webkitRelativePath.split('/').slice(1).join('/') || (file.name.endsWith('.nrr') ? `.nrr/${file.name}` : file.name),data:file}));
      const requestedBuild=new URLSearchParams(location.search).get('runtime');
      const runtimeBuild=['debug','fibers','compact','fibers-compact'].includes(requestedBuild)?requestedBuild:'fibers-compact';
      const inspectPrograms=new URLSearchParams(location.search).get('inspectPrograms')==='1';
      const diagnosticMemory=new URLSearchParams(location.search).get('diagnosticMemory')==='1';
      const diagnosticFrames=new URLSearchParams(location.search).get('diagnosticFrames')==='1';
      const boot=event.target.id==='boot';
      let canvas;
      if(boot){
        const display=document.createElement('canvas');display.id='game';display.width=1280;display.height=720;
        display.setAttribute('aria-label','Hình ảnh game từ NVN');
        document.querySelector('#game')?.replaceWith(display);
        if(!display.isConnected)document.querySelector('#runtime-status').after(display);
        input=attachGameInput(display);
        canvas=display.transferControlToOffscreen();
      }
      worker.postMessage({file,romfs,fonts,boot,runtimeBuild,inspectPrograms,diagnosticMemory,diagnosticFrames,canvas},canvas?[canvas]:[]);
    }), 'Runtime', runtimeTimeout);
    window.__ssbuRuntime = result;
    if(allocationTimeline.length)result.allocationTimeline=allocationTimeline;
    const response = await fetch('/__runtime_report', {method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({results:{runtime:result},timestamp:new Date().toISOString()})});
    if (!response.ok) throw new Error(`Không lưu được báo cáo runtime: ${response.status}`);
    status.textContent = result.ok ? result.phase === 'nnMain' ? 'nnMain đã trả về. Chưa xác nhận menu/trận đấu.' : `Đã chạy ${result.constructorsCompleted}/${result.constructorsTotal} constructors. Chưa chạy nnMain hoặc renderer.` : `Đã chạy ${result.constructorsCompleted} constructors; ${result.phase || 'runtime'} dừng tại ${result.import || result.address}: ${result.error}.`;
  } catch (error) {
    window.__ssbuRuntimeError=String(error);
    const result={ok:false,...window.__ssbuRuntimeDiagnostics,
      phase:cancelled?'cancelled':window.__ssbuRuntimeDiagnostics?'workerError':'browserError',
      error:String(error),progress:window.__ssbuRuntimeProgress};
    if(allocationTimeline.length)result.allocationTimeline=allocationTimeline;
    window.__ssbuRuntime=result;
    try {
      const response=await fetch('/__runtime_report',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({results:{runtime:result},timestamp:new Date().toISOString()})});
      if(!response.ok)log.textContent+=`Không lưu được báo cáo lỗi runtime: ${response.status}\n`;
    }catch(saveError){log.textContent+=`Không lưu được báo cáo lỗi runtime: ${saveError}\n`;}
    status.textContent=cancelled?'Đã dừng runtime. Có thể chạy lại.':`Lỗi: ${error}`;
  }
  finally {
    input?.close();worker?.terminate();activeRuntime=undefined;stopRuntime.hidden=true;
    runtimeControls.forEach((control,index)=>{control.disabled=priorDisabled[index];});
    window.__ssbuRuntimeDone=true;
  }
}
document.querySelector('#runtime').addEventListener('click',startRuntime);
document.querySelector('#boot').addEventListener('click',startRuntime);
