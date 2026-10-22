import {WebGpuBackend} from './gpu-backend.mjs';

/** Analytic texture samples on the actual device. Inputs use the frontend's
 * 48-byte encoding. This is not a captured NVN draw/pixel equivalence test. */
export async function testGpuSamplers(device) {
  const resources=[],errors=[];
  const checked=async action=>{
    device.pushErrorScope('validation');let result,error;
    try{result=await action();}catch(cause){error=cause;}
    const validation=await device.popErrorScope();
    if(error||validation)throw error||new Error(validation.message);
    return result;
  };
  const backend=new WebGpuBackend(device);
  const keep=resource=>(resources.push(resource),resource);
  let checks=0;
  function value({min=5,mag=1,wrap=7,compare=0,low=0,high=1000,aniso=1}={}){
    const bytes=new Uint8Array(48),v=new DataView(bytes.buffer);
    bytes.set([min,mag,wrap,wrap,wrap,compare?1:0,compare||2,0]);
    v.setFloat32(8,low,true);v.setFloat32(12,high,true);v.setFloat32(20,aniso,true);return bytes;
  }
  async function texture(levels){
    return checked(()=>{
      const texture=keep(device.createTexture({size:[2,2],mipLevelCount:levels,format:'rgba8unorm',usage:GPUTextureUsage.TEXTURE_BINDING|GPUTextureUsage.COPY_DST}));
      device.queue.writeTexture({texture},new Uint8Array([0,0,0,255,64,0,0,255,128,0,0,255,255,0,0,255]),{bytesPerRow:8},[2,2]);
      if(levels===2)device.queue.writeTexture({texture,mipLevel:1},new Uint8Array([192,0,0,255]),{bytesPerRow:4},[1,1]);
      return texture;
    });
  }
  async function pipeline(comparison){
    return checked(async()=>{
      const module=device.createShaderModule({code:`
        @group(0) @binding(0) var image: ${comparison?'texture_depth_2d':'texture_2d<f32>'};
        @group(0) @binding(1) var sampling: ${comparison?'sampler_comparison':'sampler'};
        @group(0) @binding(2) var<storage,read> probes: array<vec4<f32>>;
        @group(0) @binding(3) var<storage,read_write> results: array<vec4<f32>>;
        @compute @workgroup_size(1) fn sample_main(@builtin(global_invocation_id) id: vec3<u32>) {
          let probe=probes[id.x];
          results[id.x]=${comparison?'vec4<f32>(textureSampleCompareLevel(image,sampling,probe.xy,probe.z),0.0,0.0,1.0)':'textureSampleLevel(image,sampling,probe.xy,probe.z)'};
        }`});
      const info=await module.getCompilationInfo();
      if(info.messages.some(message=>message.type==='error'))throw new Error('Sampler probe WGSL failed');
      return device.createComputePipelineAsync({layout:'auto',compute:{module,entryPoint:'sample_main'}});
    });
  }
  async function sample(texture,levels,pipeline,params,probes){
    const sampler=await backend.materializeSampler(value(params),{mipLevelCount:levels});
    const floats=new Float32Array(probes.flatMap(probe=>[...probe.slice(0,3),0]));
    const size=floats.byteLength;
    const input=keep(device.createBuffer({size,usage:GPUBufferUsage.STORAGE|GPUBufferUsage.COPY_DST}));
    const output=keep(device.createBuffer({size,usage:GPUBufferUsage.STORAGE|GPUBufferUsage.COPY_SRC}));
    const read=keep(device.createBuffer({size,usage:GPUBufferUsage.MAP_READ|GPUBufferUsage.COPY_DST}));
    await checked(()=>{
      device.queue.writeBuffer(input,0,floats);
      const group=device.createBindGroup({layout:pipeline.getBindGroupLayout(0),entries:[
        {binding:0,resource:texture.createView()},{binding:1,resource:sampler},
        {binding:2,resource:{buffer:input}},{binding:3,resource:{buffer:output}}]});
      const encoder=device.createCommandEncoder(),pass=encoder.beginComputePass();
      pass.setPipeline(pipeline);pass.setBindGroup(0,group);pass.dispatchWorkgroups(probes.length);pass.end();
      encoder.copyBufferToBuffer(output,0,read,0,size);device.queue.submit([encoder.finish()]);
    });
    await read.mapAsync(GPUMapMode.READ);
    try{
      const actual=new Float32Array(read.getMappedRange());
      for(let i=0;i<probes.length;i++)for(let channel=0;channel<4;channel++){
        const expected=channel===0?probes[i][3]:channel===3?1:0;
        if(!Number.isFinite(actual[i*4+channel])||Math.abs(actual[i*4+channel]-expected)>1e-4)
          errors.push({params,probe:probes[i],channel,actual:actual[i*4+channel],expected});
        checks++;
      }
    }finally{read.unmap();}
  }
  try{
    const single=await texture(1),mips=await texture(2),color=await pipeline(false);
    await sample(single,1,color,{min:0,mag:0},[[.25,.25,0,0],[.75,.25,2,64/255],[.25,.75,-2,128/255]]);
    await sample(single,1,color,{min:1},[[.5,.5,0,111.75/255],[.5,.5,2,111.75/255]]);
    await sample(mips,2,color,{min:2,mag:0},[[.25,.25,.25,0],[.25,.25,.75,192/255]]);
    await sample(mips,2,color,{min:3},[[.5,.5,.25,111.75/255],[.5,.5,.75,192/255]]);
    await sample(mips,2,color,{min:4,mag:0},[[.25,.25,.25,48/255],[.25,.25,.75,144/255]]);
    await sample(mips,2,color,{min:5},[[.5,.5,.25,131.8125/255],[.5,.5,.75,171.9375/255]]);
    for(const [wrap,positive,negative,corner]of[[1,0,64,0],[6,64,0,255],[7,64,0,255]])
      await sample(mips,2,color,{min:2,mag:0,wrap},[[1.25,.25,0,positive/255],[-.25,.25,0,negative/255],[1.25,1.25,0,corner/255]]);
    await sample(mips,2,color,{min:2,mag:0,low:1,high:1},[[.25,.25,0,192/255]]);
    await sample(mips,2,color,{min:2,mag:0,high:0},[[.25,.25,1,0]]);
    await sample(mips,2,color,{min:5,aniso:16},[[.25,.25,2,192/255]]);
    // Captured game state: linear min/mag, nearest mip, [-1000,1000].
    await sample(mips,2,color,{min:3,low:-1000,high:1000},[
      [.5,.5,-1000,111.75/255],[.5,.5,-2,111.75/255],[.5,.5,0,111.75/255],
      [.5,.5,.25,111.75/255],[.5,.5,.75,192/255],[.5,.5,1000,192/255]]);
    // Different min/mag filters must retain the crossover, not force one.
    await sample(mips,2,color,{min:2,mag:1,low:-1000},[
      [.5,.5,-2,111.75/255],[.5,.5,0,111.75/255],[.5,.5,.25,1],[.5,.5,.75,192/255]]);
    await sample(mips,2,color,{min:2,mag:1,low:-0.25,high:.75},[
      [.5,.5,-1000,111.75/255],[.5,.5,-2,111.75/255],[.5,.5,0,111.75/255],[.5,.5,1000,192/255]]);
    const depth=await checked(()=>{
      const texture=keep(device.createTexture({size:[2,2],format:'depth32float',usage:GPUTextureUsage.TEXTURE_BINDING|GPUTextureUsage.RENDER_ATTACHMENT}));
      const encoder=device.createCommandEncoder(),pass=encoder.beginRenderPass({colorAttachments:[],depthStencilAttachment:{view:texture.createView(),depthClearValue:.5,depthLoadOp:'clear',depthStoreOp:'store'}});
      pass.end();device.queue.submit([encoder.finish()]);return texture;
    });
    const comparison=await pipeline(true);
    for(const [compare,ref,expected]of[[1,.25,0],[2,.25,1],[2,.75,0],[3,.5,1],[3,.25,0],[4,.5,1],[4,.75,0],
      [5,.75,1],[5,.25,0],[6,.25,1],[6,.5,0],[7,.5,1],[7,.25,0],[8,.75,1]])
      await sample(depth,1,comparison,{min:0,mag:0,compare},[[.25,.25,ref,expected]]);
    return {ok:errors.length===0,checks,errors,statistics:{...backend.samplers.statistics},
      scope:'Hand-encoded frontend sampler values, actual WebGPU nearest/linear/mip/wrap/LOD/depth-comparison samples with analytic texels; not a native NVN draw or anisotropic pixel-equivalence test'};
  }finally{backend.close();for(const resource of resources)resource.destroy();}
}
