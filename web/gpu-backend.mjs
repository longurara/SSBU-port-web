import {WebGpuPrograms} from './gpu-programs.mjs';
import {WebGpuTextures} from './gpu-textures.mjs';
import {WebGpuSamplers} from './gpu-samplers.mjs';
import {WebGpuRender} from './gpu-render.mjs';
import {WebGpuDraw} from './gpu-draw.mjs';
import {WebGpuCompute} from './gpu-compute.mjs';
import {stageInterface,diagnosticVertexLayout} from './wgsl-interface.mjs';
/** WebGPU resources owned by the runtime Worker. NVN guest addresses never
 * become raw Wasm pointers here; transfers use bounded copied staging data. */
export function deviceCapabilities(device) {
  const l=device.limits;
  // ABI sizes/alignment describe the compatibility frontend's opaque objects,
  // descriptors and command storage, not the physical GPU's architecture.
  const values=[53,312,l.maxUniformBuffersPerShaderStage,l.maxUniformBufferBindingSize,l.minUniformBufferOffsetAlignment,
    l.maxColorAttachments,l.maxVertexBuffers,0,l.maxStorageBuffersPerShaderStage,l.maxSampledTexturesPerShaderStage,
    16,4,4,4,l.maxVertexAttributes,32,32,256,256,4,8,512,256,2,256,4096,131072,16,
    l.maxComputeWorkgroupSizeX,l.maxComputeWorkgroupSizeY,l.maxComputeWorkgroupSizeZ,l.maxComputeInvocationsPerWorkgroup,
    l.maxComputeWorkgroupsPerDimension,l.maxComputeWorkgroupsPerDimension,l.maxComputeWorkgroupsPerDimension,
    l.maxStorageTexturesPerShaderStage,1048576,4096,1,4096,0,1,0,0,
    Math.floor(Math.log2(l.maxTextureDimension2D))+1,l.maxTextureArrayLayers,1,1,15,5,
    0,8,0,4,32,128,32,128,65536,0,4,l.maxTextureDimension2D,0,l.maxTextureDimension3D,
    l.maxTextureDimension2D,l.maxTextureDimension2D,0,0,0,0,1,0,0,0,0,0,0,0,1024,0,
    4096,65536,65536,256,16384,262144,4096,0,4,2,0,4096,16384,4096,
    l.maxSampledTexturesPerShaderStage,l.maxSamplersPerShaderStage,65535,0];
  if(values.length!==0x62 || values.some(value=>!Number.isSafeInteger(value)||value<0||value>0x7fffffff))
    throw new Error('Invalid NVN/WebGPU capability table');
  return new Int32Array(values);
}

/** Complete each C request once. Closing the owner disables all late writes
 * and callbacks before pthread stacks are terminated. */
export async function dispatchGpuRequest(module,{token,operation,handle,offset,size,pointer,length}) {
  if(module.webgpuBridgeClosed)return;
  let reply;
  try {
    const backend=module.webgpuBackend;
    if(!backend)throw new Error('WebGPU backend not attached');
    let bytes;
    if(operation===2||operation===9||operation===12||operation===14||operation===17||operation===18||operation===22||operation===23||operation===24||operation===25||operation===26) {
      if(pointer+length>module.HEAPU8.length)throw new Error('GPU staging input range exceeds Wasm memory');
      bytes=module.HEAPU8.slice(pointer,pointer+length);
    }
    reply=await backend.request(operation,{handle,offset,size,bytes});
    if(module.webgpuBridgeClosed)return;
    if(reply.bytes) {
      if(!(reply.bytes instanceof Uint8Array)||reply.bytes.length!==length||pointer+length>module.HEAPU8.length)
        throw new Error('GPU readback storage mismatch');
      module.HEAPU8.set(reply.bytes,pointer);
    }
  }catch(error) {
    if(!module.webgpuBridgeClosed){module.webgpuError=String(error);module._gpu_complete(token,1,0);}
    return;
  }
  // Keep completion outside the catch: a failing C callback must never cause
  // a second completion against a possibly released request frame.
  if(!module.webgpuBridgeClosed)module._gpu_complete(token,0,reply.value);
}

export class WebGpuBackend {
  constructor(device,{canvas,diagnosticFrames=false}={}) {
    this.device=device;this.capabilities=deviceCapabilities(device);
    this.buffers=new Map();this.nextHandle=1;this.closed=false;
    this.tail=Promise.resolve();
    this.fences=new Map();this.nextFence=1;
    this.programs=new WebGpuPrograms(device,action=>this.checked(action));
    this.textures=new WebGpuTextures(device,action=>this.checked(action));
    this.samplers=new WebGpuSamplers(device,action=>this.checked(action));
    this.render=new WebGpuRender(device,this.textures,action=>this.checked(action),{canvas,diagnosticFrames});
    this.draw=new WebGpuDraw(this);
    this.compute=new WebGpuCompute(this);
    this.statistics={created:0,destroyed:0,writes:0,reads:0,bytesWritten:0,bytesRead:0,liveBytes:0,flushes:0,finishes:0,
      fencesCreated:0,fencesReleased:0,liveFences:0,syncWaits:0,serverWaits:0,
      programsRegistered:0,livePrograms:0,shadersCompiled:0,programSnapshotsCreated:0,liveProgramSnapshots:0,...this.textures.statistics,...this.samplers.statistics,...this.render.statistics,...this.draw.statistics,...this.compute.statistics};
    this.device.lost.then(info=>{this.lost=info;});
    this.device.addEventListener('uncapturederror',event=>{this.failure=event.error;});
  }
  buffer(handle) {
    const record=this.buffers.get(handle);
    if(!record)throw new Error('Invalid or stale GPU buffer handle');
    return record;
  }
  range(record,offset,size) {
    if(!Number.isSafeInteger(offset)||!Number.isSafeInteger(size)||offset<0||size<0||offset%4||size%4||offset>record.size||size>record.size-offset)
      throw new Error('GPU transfer range must be aligned and within the buffer');
  }
  async checked(action) {
    this.device.pushErrorScope('out-of-memory');this.device.pushErrorScope('validation');
    let result,error;
    try {result=await action();}catch(cause){error=cause;}
    const validation=await this.device.popErrorScope(),memory=await this.device.popErrorScope();
    if(error||validation||memory)throw error||new Error(validation?.message||memory.message);
    return result;
  }
  request(operation,arguments_={}) {
    if(operation===9||operation===6) {
      // Order the wait's snapshot after earlier requests, but do not hold the
      // submission queue while a CPU thread waits for GPU progress.
      const snapshot=this.tail.then(()=>({completion:operation===9?this.waitFence(arguments_):this.perform(6,arguments_)}));
      this.tail=snapshot.then(()=>{},()=>{});
      return snapshot.then(({completion})=>completion);
    }
    // WebGPU error scopes form one stack per device. Serialize requests from
    // different guest pthreads so errors and queue completion belong to the
    // correct caller, and a rejected request does not poison later work.
    const task=this.tail.then(()=>this.perform(operation,arguments_));
    this.tail=task.catch(()=>{});return task;
  }
  compileProgram(handle,options) {
    const task=this.tail.then(async()=> {
      if(this.closed)throw new Error('GPU backend closed');
      const stages=await this.programs.compile(handle,options);
      this.statistics.shadersCompiled=this.programs.compilations;return stages;
    });
    this.tail=task.catch(()=>{});return task;
  }
  materializeSampler(bytes,{mipLevelCount}={}) {
    if(!(bytes instanceof Uint8Array))return Promise.reject(new Error('Sampler requires copied frontend bytes'));
    const snapshot=bytes.slice();
    const task=this.tail.then(async()=>{
      if(this.closed)throw new Error('GPU backend closed');
      try{
        const sampler=await this.samplers.get(snapshot,{mipLevelCount});
        if(this.closed)throw new Error('GPU backend closed during sampler creation');
        return sampler;
      }
      finally{Object.assign(this.statistics,this.samplers.statistics);}
    });
    this.tail=task.catch(()=>{});return task;
  }
  compileProgramSnapshot(snapshot,options) {
    const task=this.tail.then(async()=>{
      if(this.closed)throw new Error('GPU backend closed');
      const stages=await this.programs.compileSnapshot(snapshot,options);
      this.statistics.shadersCompiled=this.programs.compilations;return stages;
    });
    this.tail=task.catch(()=>{});return task;
  }
  checkComputeProgram(handle,options) {
    const task=this.tail.then(async()=>{
      if(this.closed)throw new Error('GPU backend closed');
      const stages=await this.programs.compile(handle,options);
      if(stages.length!==1||stages[0].metadata.stage!==5)throw new Error('Expected one compute shader');
      const stage=stages[0],{entryPoint}=stageInterface(stage.code,'compute');
      await this.checked(()=>this.device.createComputePipelineAsync({layout:'auto',compute:{module:stage.module,entryPoint}}));
      if(this.closed)throw new Error('GPU backend closed during pipeline validation');
      this.statistics.diagnosticComputePipelines=(this.statistics.diagnosticComputePipelines||0)+1;
      return {entryPoint};
    });
    this.tail=task.catch(()=>{});return task;
  }
  checkGraphicsPrograms(vertexHandle,fragmentHandle,options) {
    const task=this.tail.then(async()=>{
      if(this.closed)throw new Error('GPU backend closed');
      const vertex=await this.programs.compile(vertexHandle,options),fragment=await this.programs.compile(fragmentHandle,options);
      if(vertex.length!==1||vertex[0].metadata.stage!==0||fragment.length!==1||fragment[0].metadata.stage!==1)throw new Error('Expected separate vertex and fragment programs');
      const layout=diagnosticVertexLayout(vertex[0].code),fragmentEntry=stageInterface(fragment[0].code,'fragment').entryPoint;
      await this.checked(()=>this.device.createRenderPipelineAsync({layout:'auto',
        vertex:{module:vertex[0].module,...layout},fragment:{module:fragment[0].module,entryPoint:fragmentEntry,targets:[{format:'rgba8unorm'}]},
        primitive:{topology:'triangle-list'}}));
      if(this.closed)throw new Error('GPU backend closed during pipeline validation');
      this.statistics.diagnosticGraphicsPipelines=(this.statistics.diagnosticGraphicsPipelines||0)+1;
      return {vertexHandle,fragmentHandle,vertexEntry:layout.entryPoint,fragmentEntry,vertexBuffers:layout.buffers,targetFormat:'rgba8unorm',syntheticVertexLayout:true};
    });
    this.tail=task.catch(()=>{});return task;
  }
  async perform(operation,{handle=0,offset=0,size=0,bytes}={}) {
    if(this.closed)throw new Error('GPU backend closed');
    if(operation===12) {
      const value=this.programs.register(handle,bytes);this.statistics.programsRegistered++;
      this.statistics.livePrograms=this.programs.records.size;return {value};
    }
    if(operation===13){this.programs.release(handle);this.statistics.livePrograms=this.programs.records.size;return {value:0};}
    if(operation===20){
      const value=this.programs.retain(handle);this.statistics.programSnapshotsCreated++;
      this.statistics.liveProgramSnapshots=this.programs.snapshots.size;return {value};
    }
    if(operation===21){this.programs.releaseSnapshot(handle);this.statistics.liveProgramSnapshots=this.programs.snapshots.size;return {value:0};}
    if(operation===14) {
      const value=await this.textures.create(bytes);Object.assign(this.statistics,this.textures.statistics);return {value};
    }
    if(operation===15){this.textures.destroy(handle);Object.assign(this.statistics,this.textures.statistics);return {value:0};}
    if(operation===16) {
      const bytes=await this.textures.read(handle,offset,size);Object.assign(this.statistics,this.textures.statistics);return {value:0,bytes};
    }
    if(operation===17){await this.textures.submitCopies(bytes);Object.assign(this.statistics,this.textures.statistics);return {value:0};}
    if(operation===18){const value=await this.textures.createView(handle,bytes);Object.assign(this.statistics,this.textures.statistics);return {value};}
    if(operation===19){this.textures.releaseView(handle);Object.assign(this.statistics,this.textures.statistics);return {value:0};}
    if(operation===22){await this.render.clearColors(bytes);Object.assign(this.statistics,this.render.statistics);return {value:0};}
    if(operation===23){await this.render.present(handle,bytes);Object.assign(this.statistics,this.render.statistics);return {value:0};}
    if(operation===25){await this.render.clearDepthStencil(bytes);Object.assign(this.statistics,this.render.statistics);return {value:0};}
    if(operation===24){await this.draw.draw(bytes);return {value:0};}
    if(operation===26)return this.compute.dispatch(bytes);
    if(operation===5) {
      await this.checked(()=>this.device.queue.submit([]));this.statistics.flushes++;return {value:0};
    }
    if(operation===6) {
      await this.device.queue.onSubmittedWorkDone();this.statistics.finishes++;return {value:0};
    }
    if(operation===7)return {value:this.lost||this.failure?1:0};
    if(operation===8) {
      if(this.nextFence>0x7fffffff)throw new Error('GPU fence handle space exhausted');
      // Submit pending queue writes before taking the completion snapshot.
      await this.checked(()=>this.device.queue.submit([]));
      const fence={done:false,failed:false};
      fence.promise=new Promise(resolve=>{fence.resolve=resolve;});
      this.device.queue.onSubmittedWorkDone().then(()=>{fence.done=true;fence.resolve();},()=>{fence.failed=true;fence.resolve();});
      const handle=this.nextFence++;this.fences.set(handle,fence);
      this.statistics.fencesCreated++;this.statistics.liveFences++;return {value:handle};
    }
    if(operation===10) {
      if(!this.fences.delete(handle))throw new Error('Invalid GPU fence handle');
      this.statistics.fencesReleased++;this.statistics.liveFences--;return {value:0};
    }
    if(operation===11) {
      const fence=this.fences.get(handle);this.statistics.serverWaits++;
      // All logical NVN queues share one physical WebGPU queue. Work submitted
      // after this snapshot is already ordered after the fenced work, so a
      // server wait needs no CPU stall or manufactured completion signal.
      return {value:fence&&!fence.failed&&!this.lost?1:0};
    }
    if(operation===1) {
      if(!Number.isSafeInteger(size)||size<=0||size%4||size>this.device.limits.maxBufferSize)
        throw new Error('Invalid GPU buffer size');
      if(this.nextHandle>0x7fffffff)throw new Error('GPU buffer handle space exhausted');
      let buffer;
      try {
        await this.checked(()=>{buffer=this.device.createBuffer({size,usage:
          GPUBufferUsage.COPY_SRC|GPUBufferUsage.COPY_DST|GPUBufferUsage.VERTEX|GPUBufferUsage.INDEX|
          GPUBufferUsage.UNIFORM|GPUBufferUsage.STORAGE|GPUBufferUsage.INDIRECT});});
      } catch(error){buffer?.destroy();throw error;}
      if(this.closed){buffer.destroy();throw new Error('GPU backend closed during allocation');}
      handle=this.nextHandle++;this.buffers.set(handle,{buffer,size});
      this.statistics.created++;this.statistics.liveBytes+=size;return {value:handle};
    }
    const record=this.buffer(handle);
    if(operation===4) {
      record.buffer.destroy();this.buffers.delete(handle);
      this.statistics.destroyed++;this.statistics.liveBytes-=record.size;return {value:0};
    }
    this.range(record,offset,size);
    if(operation===2) {
      if(!(bytes instanceof Uint8Array)||bytes.length!==size)throw new Error('GPU upload size mismatch');
      if(this.lost||this.failure)throw new Error(this.failure?.message||this.lost?.message||'GPU device lost');
      this.device.queue.writeBuffer(record.buffer,offset,bytes);
      this.statistics.writes++;this.statistics.bytesWritten+=size;return {value:0};
    }
    if(operation===3) {
      if(!size)return {value:0,bytes:new Uint8Array()};
      const staging=this.device.createBuffer({size,usage:GPUBufferUsage.COPY_DST|GPUBufferUsage.MAP_READ});
      try {
        await this.checked(()=> {
          const encoder=this.device.createCommandEncoder();encoder.copyBufferToBuffer(record.buffer,offset,staging,0,size);
          this.device.queue.submit([encoder.finish()]);
        });
        await staging.mapAsync(GPUMapMode.READ);
        const bytes=new Uint8Array(staging.getMappedRange()).slice();staging.unmap();
        this.statistics.reads++;this.statistics.bytesRead+=size;return {value:0,bytes};
      } finally {staging.destroy();}
    }
    throw new Error(`Unknown GPU resource operation ${operation}`);
  }
  async waitFence({handle,bytes}) {
    if(this.closed)throw new Error('GPU backend closed');
    if(!(bytes instanceof Uint8Array)||bytes.length!==8)throw new Error('Fence timeout must be a uint64 nanosecond value');
    this.statistics.syncWaits++;
    const fence=this.fences.get(handle);
    if(!fence||fence.failed||this.lost)return {value:3};
    if(fence.done)return {value:0};
    const timeout=new DataView(bytes.buffer,bytes.byteOffset,8).getBigUint64(0,true);
    if(timeout===0n)return {value:2};
    if(timeout===0xffffffffffffffffn) {await fence.promise;return {value:fence.failed?3:1};}
    const deadline=performance.now()+Number(timeout)/1e6;
    let timer;
    const expired=new Promise(resolve=> {
      function poll() {
        const remaining=deadline-performance.now();
        if(remaining<=0)resolve(false);
        else timer=setTimeout(poll,Math.min(Math.ceil(remaining),0x7fffffff));
      }
      poll();
    });
    try {
      const completed=await Promise.race([fence.promise.then(()=>true),expired]);
      return {value:fence.failed?3:completed?1:2};
    } finally {clearTimeout(timer);}
  }
  close() {
    this.compute.close();
    this.draw.close();
    this.render.close();
    this.programs.close();
    this.samplers.close();Object.assign(this.statistics,this.samplers.statistics);
    this.textures.close();Object.assign(this.statistics,this.textures.statistics);
    this.statistics.livePrograms=0;this.statistics.liveProgramSnapshots=0;
    for(const {buffer} of this.buffers.values())buffer.destroy();this.buffers.clear();
    for(const fence of this.fences.values()){fence.failed=true;fence.resolve();}this.fences.clear();
    this.statistics.liveBytes=0;this.statistics.liveFences=0;this.closed=true;
  }
}
