import {WebGpuBackend,dispatchGpuRequest} from './gpu-backend.mjs';
import {localFontInputs} from './font-input.mjs';
import {MovieBackend} from './movie-backend.mjs';

self.onmessage = async ({data}) => {
  let module;
  let device;
  let backend;
  let movies;
  try {
    const fonts=localFontInputs(data.fonts,{required:data.boot===true});
    const runtimeModules={debug:'./runtime-debug.mjs',fibers:'./runtime-fibers.mjs',compact:'./runtime-debug-compact.mjs','fibers-compact':'./runtime-fibers-compact.mjs'};
    const runtimeBuild=Object.hasOwn(runtimeModules,data.runtimeBuild)?data.runtimeBuild:'fibers-compact';
    self.postMessage({type:'status',message:'Đang tải và biên dịch WebAssembly, lần đầu có thể mất một lúc…'});
    const {default:create}=await import(runtimeModules[runtimeBuild]);
    let completed,failed;
    const done = new Promise((resolve,reject) => {completed=resolve;failed=reject;});
    done.catch(()=>{});
    if (data.boot && navigator.gpu) {
      const adapter = await navigator.gpu.requestAdapter({powerPreference:'high-performance'});
      if (adapter) {
        const requiredFeatures=['texture-compression-bc','texture-compression-astc','texture-formats-tier1','rg11b10ufloat-renderable','shader-f16','float32-filterable','depth-clip-control','depth32float-stencil8','indirect-first-instance','clip-distances'].filter(feature=>adapter.features.has(feature));
        device=await adapter.requestDevice({requiredFeatures});
      }
    }
    if(device)backend=new WebGpuBackend(device,{canvas:data.canvas,diagnosticFrames:data.diagnosticFrames===true});
    // Opt-in access for the local browser diagnostic harness. Never expose a
    // running backend in the normal game flow; clear the reference on shutdown.
    if(data.diagnosticFrames===true)self.__ssbuDiagnosticGpu=backend;
    movies=new MovieBackend({notify:(owner,mask)=>module?._web_movie_notify(owner,mask)});
    module = await create({webgpuDevice:device,webgpuBackend:backend,
      webmovieDispatch:request=>movies.dispatch(module,request).catch(failed),
      webgpuDispatch:request=>dispatchGpuRequest(module,request).catch(failed),onRuntimeReport:text => completed(JSON.parse(text))});
    // The local diagnostic harness can inspect exports and bounded guest data
    // directly, without heap-wide DevTools object queries on a large live Wasm.
    if(data.diagnosticFrames===true)self.__ssbuDiagnosticModule=module;
    if (!(data.file instanceof File)) throw new Error('Chưa chọn main.elf');
    module.FS.mkdir('/program');
    module.FS.mount(module.WORKERFS, {blobs:[{name:'main.elf',data:data.file}]}, '/program');
    if (data.romfs?.length) {
      module.FS.mkdir('/romfs');
      module.FS.mount(module.WORKERFS,{blobs:data.romfs},'/romfs');
    }
    if(fonts.length){module.FS.mkdir('/fonts');module.FS.mount(module.WORKERFS,{blobs:fonts},'/fonts');}
    self.postMessage({type:'status',message:'Đang nạp chương trình…'});
    if (!module.ccall('elf_load','number',['string'],['/program/main.elf'])) throw new Error(module.ccall('elf_info','string',[],[]));
    if(data.boot&&typeof module._web_input_pointer==='function')self.postMessage({type:'input-state',buffer:module.HEAPU8.buffer,offset:module._web_input_pointer()>>>0});
    self.postMessage({type:'status',message:'Đang khởi tạo runtime và chạy constructors…'});
    const infoBase = module._runtime_info() >>> 0;
    if (module._runtime_start(data.boot ? 1 : 0)) throw new Error('Cannot start runtime pthread');
    const startedAt = Date.now();
    const progressTimer = setInterval(() => {
      try {
        const view = new DataView(module.HEAPU8.buffer);
        const hex64 = (off) => `0x${view.getBigUint64(infoBase + off, true).toString(16)}`;
        const progress = {
          elapsedMs: Date.now() - startedAt,
          wasmHeapBytes: data.diagnosticFrames===true?module.HEAPU8.buffer.byteLength:undefined,
          allocationInfo: data.diagnosticFrames===true&&typeof module._web_allocation_info==='function'?JSON.parse(module.ccall('web_allocation_info','string',[],[])):undefined,
          commandInfo: data.diagnosticFrames===true&&typeof module._web_gpu_command_info==='function'?JSON.parse(module.ccall('web_gpu_command_info','string',[],[])):undefined,
          importCalls: view.getUint32(infoBase + 0x400, true),
          constructorsCompleted: view.getUint32(infoBase + 0x404, true),
          threadFailed: view.getInt32(infoBase + 0x760, true),
          fn: hex64(0x748),
          lr: hex64(0x508),
          sp: hex64(0x510),
          x0: hex64(0x418),
          x8: hex64(0x458),
          x19: hex64(0x4b0),
          x20: hex64(0x4b8),
          x21: hex64(0x4c0),
          sdkLayoutQueries: typeof module._web_nvn_layout_query_count === 'function' ? module._web_nvn_layout_query_count() : 0,
          mutexes: typeof module._web_mutex_live_count === 'function' ? {live: module._web_mutex_live_count(), peak: module._web_mutex_peak_count()} : undefined,
          semaphores: typeof module._web_semaphore_live_count === 'function' ? {live: module._web_semaphore_live_count()} : undefined,
          gpuResources: backend ? {...backend.statistics} : undefined,
          frameDiagnostics: backend?.render?.frameDiagnostics,
        };
        self.postMessage({type:'progress',progress});
      } catch {}
    }, 2000);
    let result;
    try {
      result = await done;
    } finally {
      clearInterval(progressTimer);
    }
    result.runtimeBuild=runtimeBuild;
    if(data.diagnosticFrames===true)result.wasmHeapBytes=module.HEAPU8.buffer.byteLength;
    if(data.diagnosticFrames===true&&typeof module._web_allocation_info==='function')result.allocationInfo=JSON.parse(module.ccall('web_allocation_info','string',[],[]));
    if(data.diagnosticFrames===true&&typeof module._web_gpu_command_info==='function')result.commandInfo=JSON.parse(module.ccall('web_gpu_command_info','string',[],[]));
    if(typeof module._web_mutex_live_count==='function')result.mutexes={live:module._web_mutex_live_count(),peak:module._web_mutex_peak_count()};
    if(typeof module._web_semaphore_live_count==='function')result.semaphores={live:module._web_semaphore_live_count()};
    result.sdkLayoutQueries=module._web_nvn_layout_query_count();
    result.sdkLayout=JSON.parse(module.ccall('web_nvn_layout_info','string',[],[]));
    if(backend&&data.inspectPrograms){
      self.postMessage({type:'status',message:'Đang kiểm tra shader đã đăng ký…'});
      const {inspectGpuPrograms}=await import('./gpu-program-inspect.mjs');
      result.diagnosticProgramInspection=await inspectGpuPrograms(backend,{topology:'triangles',integerAttributes:0,signedAttributes:0,integerTextures:0,signedTextures:0,forceOutputs:0});
    }
    if(backend){result.gpuResources={...backend.statistics};result.gpuCapabilities=Array.from(backend.capabilities);}
    if(backend?.render?.frameDiagnostics)result.frameDiagnostics=backend.render.frameDiagnostics;
    if(backend?.draw?.diagnosticDraws)result.drawDiagnostics=backend.draw.diagnosticDraws;
    if(backend?.draw?.lastPacket&&!result.ok)result.lastGraphicsPacket=backend.draw.lastPacket;
    if(backend?.programs?.lastFailure)result.lastShaderFailure=backend.programs.lastFailure;
    if(module.webgpuError)result.gpuError=module.webgpuError;
    if (result.phase !== 'guestThread') module._runtime_join();
    if(!result.ok&&data.diagnosticMemory)self.postMessage({type:'memory-diagnostic',buffer:module.HEAPU8.buffer});
    self.postMessage({type:'result',result});
  } catch (error) {
    self.postMessage({type:'error',message:String(error),diagnostics:{
      phase:'workerError',runtimeBuild:data.runtimeBuild,
      gpuResources:backend?{...backend.statistics}:undefined,
      frameDiagnostics:backend?.render?.frameDiagnostics,
      drawDiagnostics:backend?.draw?.diagnosticDraws,
      lastGraphicsPacket:backend?.draw?.lastPacket,
      lastShaderFailure:backend?.programs?.lastFailure,
      gpuError:module?.webgpuError,
      scope:'Runtime worker failure snapshot; a recorded draw packet does not prove GPU submission or game rendering'
    }});
  }
  finally { delete self.__ssbuDiagnosticModule;delete self.__ssbuDiagnosticGpu;await movies?.close();if(module){module.webgpuBridgeClosed=true;module.PThread.terminateAllThreads();}backend?.close();device?.destroy();self.close(); }
};
