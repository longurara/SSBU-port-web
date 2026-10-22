import {lowerSpirv} from './spirv-compat.mjs';
import {createShaderCompiler} from './shader-compiler.mjs';

export async function testGather(device,{constant=false,width=8,height=8}={}){
  const fixture=await(await fetch(constant?'./gather-constant-test.json':'./gather-test.json')).json(),compiler=await createShaderCompiler();
  const source=new Uint32Array(fixture.words);let capability=false;
  for(let i=5;i<source.length;i+=source[i]>>>16)if((source[i]&65535)===17&&source[i+1]===25)capability=true;
  if(!constant&&!capability)throw new Error('Dynamic fixture must declare ImageGatherExtended');
  // Exercise the gate even when a glslang version omits the redundant cap.
  const words=capability?source:new Uint32Array([...source.subarray(0,5),(2<<16)|17,25,...source.subarray(5)]);
  const lowered=lowerSpirv(words.buffer),{code,activeBindings}=compiler.compileWithBindings(lowered.bytes);
  const resources=[];device.pushErrorScope('validation');
  try{
    const image=device.createTexture({size:[width,height],format:'rgba8unorm',usage:GPUTextureUsage.COPY_DST|GPUTextureUsage.TEXTURE_BINDING});resources.push(image);
    const data=new Uint8Array(width*height*4);for(let i=0;i<width*height;i++)data.set([100,150,200,i+1],i*4);device.queue.writeTexture({texture:image},data,{bytesPerRow:width*4},[width,height]);
    const target=device.createTexture({size:[4,4],format:'rgba8unorm',usage:GPUTextureUsage.RENDER_ATTACHMENT|GPUTextureUsage.COPY_SRC});resources.push(target);
    const vertex=device.createShaderModule({code:'@vertex fn vs(@builtin(vertex_index) i:u32)->@builtin(position) vec4<f32>{let p=array<vec2<f32>,3>(vec2<f32>(-1,-1),vec2<f32>(3,-1),vec2<f32>(-1,3));return vec4<f32>(p[i],0,1);}'});
    const fragment=device.createShaderModule({code}),entry=code.match(/@fragment\s+fn\s+(\w+)/)?.[1];
    const pipeline=await device.createRenderPipelineAsync({layout:'auto',vertex:{module:vertex,entryPoint:'vs'},fragment:{module:fragment,entryPoint:entry,targets:[{format:'rgba8unorm'}]},primitive:{topology:'triangle-list'}});
    const group=device.createBindGroup({layout:pipeline.getBindGroupLayout(1),entries:[{binding:0,resource:image.createView()},{binding:256,resource:device.createSampler()}]});
    let offsetBuffer,offsetGroup;
    if(!constant){offsetBuffer=device.createBuffer({size:16,usage:GPUBufferUsage.UNIFORM|GPUBufferUsage.COPY_DST});resources.push(offsetBuffer);offsetGroup=device.createBindGroup({layout:pipeline.getBindGroupLayout(0),entries:[{binding:0,resource:{buffer:offsetBuffer}}]});}
    const readback=device.createBuffer({size:1024,usage:GPUBufferUsage.MAP_READ|GPUBufferUsage.COPY_DST});resources.push(readback);
    let checks=0;
    for(const [dx,dy]of constant?[[1,-1]]:[[1,-1],[-10,9],[0,0],[31,-32]]){
      if(offsetBuffer)device.queue.writeBuffer(offsetBuffer,0,new Int32Array([dx,dy,0,0]));
      const encoder=device.createCommandEncoder(),pass=encoder.beginRenderPass({colorAttachments:[{view:target.createView(),loadOp:'clear',storeOp:'store'}]});pass.setPipeline(pipeline);if(offsetGroup)pass.setBindGroup(0,offsetGroup);pass.setBindGroup(1,group);pass.draw(3);pass.end();encoder.copyTextureToBuffer({texture:target},{buffer:readback,bytesPerRow:256},[4,4]);device.queue.submit([encoder.finish()]);
      await readback.mapAsync(GPUMapMode.READ);const bytes=new Uint8Array(readback.getMappedRange()).slice();readback.unmap();
      const x0=Math.floor(width*0.5-0.5)+dx,y0=Math.floor(height*0.5-0.5)+dy,texel=(x,y)=>Math.max(0,Math.min(height-1,y))*width+Math.max(0,Math.min(width-1,x))+1;
      const expected=[texel(x0,y0+1),texel(x0+1,y0+1),texel(x0+1,y0),texel(x0,y0)];
      for(let y=0;y<4;y++)for(let x=0;x<4;x++)for(let c=0;c<4;c++){checks++;if(Math.abs(bytes[y*256+x*4+c]-expected[c])>1)throw new Error(`Gather offset/component/order mismatch (${dx},${dy}): ${bytes.slice(y*256+x*4,y*256+x*4+4)} expected ${expected}`);}
    }
    const error=await device.popErrorScope();if(error)throw new Error(error.message);
    return {ok:true,checks,constant,width,height,activeBindings,originalCapability:capability,scope:'Self-authored GLSL/SPIR-V constant/dynamic alpha gather with signed offsets and clamp-to-edge -> Wasm Naga -> actual WebGPU pixels; not game pixels'};
  }finally{for(const resource of resources)resource.destroy();}
}
