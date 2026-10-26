import {stageInterface} from './wgsl-interface.mjs';

const uint=(n,label)=>{if(!Number.isSafeInteger(n)||n<0||n>0xffffffff)throw new Error(`Invalid compute ${label}`);return n;};
const array=(a,n,label)=>{if(!Array.isArray(a)||a.length!==n)throw new Error(`Invalid compute ${label}`);return a;};

/** Dispatch actual retained Maxwell compute programs. Written storage lands in
 * the physical resource before the C caller copies its bytes to guest memory. */
export class WebGpuCompute {
  constructor(backend){this.backend=backend;this.cache=new WeakMap();this.closed=false;this.statistics={computeDispatches:0,computeWorkgroups:0,computeStorageCopies:0,computeWrittenBuffers:0};}
  async dispatch(bytes){
    if(this.closed)throw new Error('Compute backend closed');
    if(!(bytes instanceof Uint8Array)||bytes.length>131072)throw new Error('Invalid compute packet length');
    const p=JSON.parse(new TextDecoder('utf-8',{fatal:true}).decode(bytes));this.lastPacket=p;
    if(p.version!==1)throw new Error('Unknown compute packet version');
    uint(p.program,'program');array(p.groups,3,'workgroups');array(p.ubo,15,'uniform buffers');array(p.ssbo,16,'storage buffers');array(p.textures,32,'textures');
    const b=this.backend,d=b.device;
    for(const n of p.groups)if(uint(n,'workgroup count')>d.limits.maxComputeWorkgroupsPerDimension)throw new Error('Compute workgroup count exceeds device limit');
    if(p.groups.some(n=>n===0))return {value:0};
    let integerTextures=0,signedTextures=0;const textureSwizzles={},lodQuerySamplers={};
    for(let i=0;i<32;i++)if(p.textures[i]){
      const t=p.textures[i],format=b.textures.get(t.texture).format.format;
      if(format.endsWith('uint')||format.endsWith('sint'))integerTextures=(integerTextures|1<<i)>>>0;
      if(format.endsWith('sint'))signedTextures=(signedTextures|1<<i)>>>0;
      lodQuerySamplers[`1/${i}`]=t.sampler;
      if(t.swizzle){array(t.swizzle,4,'swizzle');if(t.swizzle.some(n=>!Number.isInteger(n)||n<0||n>5))throw new Error('Invalid compute texture swizzle');textureSwizzles[`1/${i}`]=t.swizzle;}
    }
    const [shader]=await b.programs.compileSnapshot(p.program,{requestedStage:5,integerTextures,signedTextures,textureSwizzles,lodQuerySamplers});
    if(!shader||shader.metadata.stage!==5)throw new Error('Dispatch lacks a retained compute shader');
    b.statistics.shadersCompiled=b.programs.compilations;
    const meta=shader.metadata,used=(group,binding)=>!shader.activeBindings||shader.activeBindings.some(v=>v[0]===group&&v[1]===binding);
    if(meta.images)throw new Error('Compute storage images need texture writeback support');
    const source=b.programs.snapshots.get(p.program)?.stages.find(s=>s.type===6);if(!source)throw new Error('Missing retained compute source');
    let pipeline=this.cache.get(shader.module);
    if(!pipeline){const {entryPoint}=stageInterface(shader.code,'compute');pipeline=await b.checked(()=>d.createComputePipelineAsync({layout:'auto',compute:{module:shader.module,entryPoint}}));this.cache.set(shader.module,pipeline);}
    const temporary=[],bindings=[],ranges=[],copybacks=[];let maskBuffer;
    const add=(group,binding,resource)=>{bindings[group]??=[];bindings[group].push({binding,resource:resource.buffer?{buffer:resource.buffer,offset:resource.offset??0,size:resource.size}:resource});};
    const allocate=async(data)=>{
      const padded=new Uint8Array(Math.max(4,Math.ceil(data.byteLength/4)*4));padded.set(new Uint8Array(data.buffer,data.byteOffset,data.byteLength));
      const buffer=await b.checked(()=>d.createBuffer({size:padded.length,usage:GPUBufferUsage.STORAGE|GPUBufferUsage.COPY_DST|GPUBufferUsage.COPY_SRC}));temporary.push(buffer);d.queue.writeBuffer(buffer,0,padded);return {buffer,offset:0,size:padded.length};
    };
    const range=async(value,writable,slot)=>{
      array(value,3,'buffer range');const [handle,offset,size]=value;uint(handle,'buffer');uint(offset,'offset');uint(size,'size');
      if(!handle||!size)throw new Error('Unbound compute buffer');const record=b.buffer(handle);b.range(record,offset,size);
      if(size>d.limits.maxStorageBufferBindingSize)throw new Error('Compute buffer binding exceeds device limit');
      for(const old of ranges)if(old.handle===handle&&(old.writable||writable)&&offset<old.offset+old.size&&old.offset<offset+size)throw new Error('Overlapping writable compute bindings need alias lowering');
      ranges.push({handle,offset,size,writable});
      if(offset%d.limits.minStorageBufferOffsetAlignment===0)return {buffer:record.buffer,offset,size};
      const buffer=await b.checked(()=>d.createBuffer({size,usage:GPUBufferUsage.STORAGE|GPUBufferUsage.COPY_SRC|GPUBufferUsage.COPY_DST}));temporary.push(buffer);
      await b.checked(()=>{const e=d.createCommandEncoder();e.copyBufferToBuffer(record.buffer,offset,buffer,0,size);d.queue.submit([e.finish()]);});
      if(writable)copybacks.push({slot,buffer,target:record.buffer,offset,size});this.statistics.computeStorageCopies++;return {buffer,offset:0,size};
    };
    try{
      for(let n=0;n<18;n++)if(((meta.cbufs>>>n)&1)&&used(0,n)){
        let resource;
        if(n===1){const ctl=new DataView(source.control.buffer,source.control.byteOffset,source.control.byteLength),size=ctl.getUint32(0x6fc,true),offset=ctl.getUint32(0x700,true);if(!size||offset>source.data.length||size>source.data.length-offset)throw new Error('Compute program constants unavailable');resource=await allocate(source.data.subarray(offset,offset+size));}
        else if(n===2){const words=new Uint32Array(224);for(let i=0;i<32;i++)if(p.textures[i]){const t=p.textures[i],tex=b.textures.get(t.texture),view=b.textures.getView(t.view).descriptor,mip=view.baseMipLevel;words.set([Math.max(1,tex.width>>mip),Math.max(1,tex.height>>mip),tex.dimension==='3d'?Math.max(1,tex.depth>>mip):view.arrayLayerCount/(view.dimension==='cube-array'?6:1),view.mipLevelCount|((tex.dimension==='3d'?1:0)<<16)],96+i*4);}resource=await allocate(words);}
        else if(n>=3)resource=await range(p.ubo[n-3],false,-1);
        else throw new Error('Dynamic compute constant buffer zero needs lowering');
        add(0,n,resource);
      }
      for(let n=0;n<16;n++)if(((meta.ssbos>>>n)&1)&&used(0,20+n))add(0,20+n,await range(p.ssbo[n],!!(meta.ssbosWritten&(1<<n)),n));
      if(meta.ssbosWritten&&used(0,38)){const resource=await allocate(new Uint32Array([0]));maskBuffer=resource.buffer;add(0,38,resource);}
      else if(meta.ssbosWritten)throw new Error('Compute stores lack their actual write mask');
      for(const sampler of shader.layout.samplers){
        const imageUsed=used(sampler.group,sampler.imageBinding),samplerUsed=used(sampler.group,sampler.samplerBinding);if(!imageUsed&&!samplerUsed)continue;
        const t=p.textures[sampler.imageBinding];if(!t)throw new Error('Unbound compute texture');
        const tex=b.textures.get(t.texture),view=b.textures.getView(t.view);if(view.texture!==t.texture)throw new Error('Compute texture/view mismatch');
        if(tex.width!==tex.gpuWidth||tex.height!==tex.gpuHeight||tex.format.depth&&!sampler.depth)throw new Error('Compute logical/depth sampling needs explicit view lowering');
        const dimension=shader.layout.textureDimensions?.[`${sampler.group}/${sampler.imageBinding}`];let sampleView=view.view;
        if(dimension&&dimension!==view.descriptor.dimension){if(dimension!=='2d-array'||view.descriptor.dimension!=='2d'||view.descriptor.arrayLayerCount!==1)throw new Error('Compute texture dimensionality needs lowering');sampleView=await b.checked(()=>tex.texture.createView({...view.descriptor,dimension:'2d-array'}));}
        if(imageUsed)add(sampler.group,sampler.imageBinding,sampleView);
        if(samplerUsed)add(sampler.group,sampler.samplerBinding,await b.samplers.get(Uint8Array.from(t.sampler),{mipLevelCount:view.descriptor.mipLevelCount}));
      }
      if(shader.layout.pushConstant&&used(3,0)||shader.layout.drawParameters&&used(3,1))throw new Error('Compute stage unexpectedly reads graphics parameters');
      const groups=[];for(let i=0;i<bindings.length;i++)if(bindings[i]?.length)groups.push([i,await b.checked(()=>d.createBindGroup({layout:pipeline.getBindGroupLayout(i),entries:bindings[i]}))]);
      // Validate every binding before encoding any dispatch that could write.
      await b.checked(()=>{const e=d.createCommandEncoder(),pass=e.beginComputePass();pass.setPipeline(pipeline);for(const [i,g]of groups)pass.setBindGroup(i,g);pass.dispatchWorkgroups(...p.groups);pass.end();d.queue.submit([e.finish()]);});
      let written=0;
      if(maskBuffer){const read=d.createBuffer({size:4,usage:GPUBufferUsage.COPY_DST|GPUBufferUsage.MAP_READ});temporary.push(read);await b.checked(()=>{const e=d.createCommandEncoder();e.copyBufferToBuffer(maskBuffer,0,read,0,4);d.queue.submit([e.finish()]);});await read.mapAsync(GPUMapMode.READ);written=new DataView(read.getMappedRange()).getUint32(0,true);read.unmap();if((written&~meta.ssbosWritten)!==0)throw new Error('Compute write mask exceeds translated storage bindings');}
      if(copybacks.some(c=>written&(1<<c.slot)))await b.checked(()=>{const e=d.createCommandEncoder();for(const c of copybacks)if(written&(1<<c.slot))e.copyBufferToBuffer(c.buffer,0,c.target,c.offset,c.size);d.queue.submit([e.finish()]);});
      this.statistics.computeDispatches++;this.statistics.computeWorkgroups+=p.groups.reduce((a,n)=>a*n,1);for(let n=0;n<16;n++)if(written&(1<<n))this.statistics.computeWrittenBuffers++;
      return {value:written>>>0};
    }finally{for(const resource of temporary)resource.destroy();Object.assign(b.statistics,this.statistics,b.samplers.statistics);}
  }
  close(){this.closed=true;this.cache=new WeakMap();}
}
