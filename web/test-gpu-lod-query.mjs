import {lowerSpirv} from './spirv-compat.mjs';
import {createShaderCompiler} from './shader-compiler.mjs';

/** Render and read back both query components, using actual GPU derivatives,
 * view dimensions/levels, and the frontend's sampler state. No guest assets. */
export async function testGpuLodQuery(device) {
  const {words}=await (await fetch('./lod-query-test.json')).json();
  const input=new Uint8Array(new Uint32Array(words).buffer),compiler=await createShaderCompiler();
  const vertex=device.createShaderModule({code:`@vertex fn main(@builtin(vertex_index) i:u32)->@builtin(position) vec4f {
    var p=array<vec2f,3>(vec2f(-1,-1),vec2f(3,-1),vec2f(-1,3));return vec4f(p[i],0,1);}`});
  const results=[];
  for(const c of [
    {name:'minification-x',rho:8,filter:5},
    {name:'magnification-negative-lambda',rho:0.25,filter:5},
    {name:'minification-y',rho:4,filter:4,y:true},
    {name:'sheared-footprint',rho:4,filter:5,shear:true},
    {name:'lower-clamp-independent-y',rho:0.25,min:2,filter:5},
    {name:'upper-clamp-independent-y',rho:8,max:1,filter:5},
    {name:'available-level-clamp',rho:64,levels:3,filter:5},
    {name:'nonzero-base-mip',rho:4,base:2,levels:3,filter:5},
    ...[0,1,2,3,4,5].map(filter=>({name:`filter-${filter}`,rho:2**1.4,filter})),
    {name:'nearest-below-half',rho:2**1.49,filter:2},
    {name:'nearest-above-half',rho:2**1.51,filter:2},
  ]) {
    const owned=[];device.pushErrorScope('validation');
    try {
      const base=c.base??0,levels=c.levels??5,width=256>>base,height=64>>base;
      const state=new Uint8Array(48),v=new DataView(state.buffer);state[0]=c.filter;state[1]=1;state.set([7,7,7],2);
      v.setFloat32(8,c.min??-1000,true);v.setFloat32(12,c.max??1000,true);v.setFloat32(20,1,true);
      const lowered=lowerSpirv(input,{lodQuerySamplers:{'1/41':Array.from(state)}});
      const module=device.createShaderModule({code:compiler.compile(lowered.bytes)});
      const info=await module.getCompilationInfo();if(info.messages.some(m=>m.type==='error'))throw new Error(JSON.stringify(info.messages));
      const pipeline=await device.createRenderPipelineAsync({layout:'auto',vertex:{module:vertex,entryPoint:'main'},fragment:{module,entryPoint:'main',targets:[{format:'rgba32float'}]},primitive:{topology:'triangle-list'}});
      const texture=device.createTexture({size:[256,64],mipLevelCount:base+levels,format:'rgba8unorm',usage:GPUTextureUsage.TEXTURE_BINDING});owned.push(texture);
      const output=device.createTexture({size:[16,16],format:'rgba32float',usage:GPUTextureUsage.RENDER_ATTACHMENT|GPUTextureUsage.COPY_SRC});owned.push(output);
      const uniform=device.createBuffer({size:16,usage:GPUBufferUsage.UNIFORM|GPUBufferUsage.COPY_DST});owned.push(uniform);
      const gradient=c.shear?[c.rho/width/Math.SQRT2,c.rho/height/Math.SQRT2,0,c.rho/height/2]:c.y?[c.rho/width/2,0,0,c.rho/height]:[c.rho/width,0,0,c.rho/height/2];
      device.queue.writeBuffer(uniform,0,new Float32Array(gradient));
      const read=device.createBuffer({size:4096,usage:GPUBufferUsage.COPY_DST|GPUBufferUsage.MAP_READ});owned.push(read);
      const encoder=device.createCommandEncoder(),pass=encoder.beginRenderPass({colorAttachments:[{view:output.createView(),loadOp:'clear',storeOp:'store',clearValue:[0,0,0,0]}]});
      pass.setPipeline(pipeline);
      pass.setBindGroup(0,device.createBindGroup({layout:pipeline.getBindGroupLayout(0),entries:[{binding:0,resource:{buffer:uniform}}]}));
      pass.setBindGroup(1,device.createBindGroup({layout:pipeline.getBindGroupLayout(1),entries:[{binding:41,resource:texture.createView({baseMipLevel:base,mipLevelCount:levels})}]}));
      pass.draw(3);pass.end();encoder.copyTextureToBuffer({texture:output},{buffer:read,bytesPerRow:256},[16,16]);device.queue.submit([encoder.finish()]);
      await read.mapAsync(GPUMapMode.READ);const data=new Float32Array(read.getMappedRange());
      const lambda=Math.log2(c.rho),bounded=Math.min(levels-1,Math.max(0,Math.min(c.max??1000,Math.max(c.min??-1000,lambda))));
      const expected=[c.filter<2?0:c.filter<4?Math.ceil(bounded+0.5)-1:bounded,lambda];
      let maxError=0;
      for(let y=2;y<14;y++)for(let x=2;x<14;x++)for(let i=0;i<2;i++){
        const actual=data[y*64+x*4+i],error=Math.abs(actual-expected[i]);if(!Number.isFinite(error)||error>0.0001)throw new Error(`${c.name}: pixel ${x},${y} component ${i}: ${actual}, expected ${expected[i]}`);maxError=Math.max(maxError,error);
      }
      read.unmap();results.push({name:c.name,expected,maxError,pixels:144});
    } finally {for(const resource of owned)resource.destroy();const error=await device.popErrorScope();if(error)throw new Error(error.message);}
  }
  return {ok:true,cases:results.length,results,scope:'Analytic isotropic footprints on real WebGPU; both query components. Native Maxwell pixel parity remains unverified.'};
}
