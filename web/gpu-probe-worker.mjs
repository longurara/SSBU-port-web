import {WebGpuBackend,dispatchGpuRequest} from './gpu-backend.mjs';
self.onmessage=async({data})=> {
  let module,device,backend;
  try {
    const adapter=await navigator.gpu?.requestAdapter();
    if(!adapter)throw new Error('No WebGPU adapter for resource integration fixture');
    const requiredFeatures=['texture-compression-bc','depth32float-stencil8'].filter(feature=>adapter.features.has(feature));
    device=await adapter.requestDevice({requiredFeatures});backend=new WebGpuBackend(device,{canvas:new OffscreenCanvas(4,4)});
    if(data.graphicsFixture)backend.programs.compilerFactory=(await import('./test-gpu-draw.mjs')).createFixtureGraphicsCompiler;
    const {default:create}=await import(data.withSdk?'./gpu-texture-probe.mjs':'./gpu-probe.mjs');
    let complete,failed;const done=new Promise((resolve,reject)=>{complete=resolve;failed=reject;});
    module=await create({webgpuDevice:device,webgpuBackend:backend,graphicsFixture:!!data.graphicsFixture,
      webgpuDispatch:request=>dispatchGpuRequest(module,request).catch(failed),onGpuProbeReport:text=>complete(JSON.parse(text))});
    if(module._gpu_probe_start())throw new Error('Cannot start resource fixture pthread');
    const result=await done;
    module._gpu_probe_join();
    if(typeof module._web_gpu_command_info==='function'){
      const pointer=module._web_gpu_command_info()>>>0;
      result.commandInfo=JSON.parse(new TextDecoder().decode(module.HEAPU8.slice(pointer,module.HEAPU8.indexOf(0,pointer))));
    }
    if(backend.programs.snapshots.size)throw new Error('Program command snapshots leaked after queue finalization');
    result.statistics={...backend.statistics};
    result.scope='Self-authored guest NVN calls -> pthread/owner-Worker bridge -> actual WebGPU upload/readback; not game draws';
    result.withSdk=!!data.withSdk;
    if(data.graphicsFixture)result.graphicsFixture='Self-authored WGSL compiler injection in fixture Worker only; real C recording/bridge/GPU draws; not Maxwell translation or game pixels';
    self.postMessage({result});
  }catch(error){self.postMessage({error:String(error)});}
  finally{if(module){module.webgpuBridgeClosed=true;module.PThread.terminateAllThreads();}backend?.close();device?.destroy();self.close();}
};
