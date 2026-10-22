import {lowerSpirv} from './spirv-compat.mjs';
import {createShaderCompiler} from './shader-compiler.mjs';

export async function testViewport(device){
  const normal=await(await fetch('./viewport-test.json')).json(),nonzero=await(await fetch('./viewport-nonzero-test.json')).json();
  const bytes=value=>new Uint32Array(value.words).buffer,compiler=await createShaderCompiler();let checks=0;
  for(const [input,options]of [[normal,{topology:'triangles'}],[nonzero,{topology:'triangles',singleInstanceBase:0}]]){
    let rejected=false;try{lowerSpirv(bytes(input),options);}catch(error){rejected=/unproven viewport/.test(error.message);}checks++;if(!rejected)throw new Error('Unproven/nonzero viewport was not rejected');
  }
  const resources=[];device.pushErrorScope('validation');
  try{
    const target=device.createTexture({size:[4,4],format:'rgba8unorm',usage:GPUTextureUsage.RENDER_ATTACHMENT|GPUTextureUsage.COPY_SRC});resources.push(target);
    const readback=device.createBuffer({size:1024,usage:GPUBufferUsage.MAP_READ|GPUBufferUsage.COPY_DST});resources.push(readback);
    const uniform=device.createBuffer({size:16,usage:GPUBufferUsage.UNIFORM|GPUBufferUsage.COPY_DST});resources.push(uniform);
    const fragment=device.createShaderModule({code:'@fragment fn fs(@location(0) @interpolate(flat) shade:vec4<f32>)->@location(0) vec4<f32>{return shade;}'});
    for(const base of [0,9]){
      const lowered=lowerSpirv(bytes(normal),{topology:'triangles',singleInstanceBase:base}),compiled=compiler.compileWithBindings(lowered.bytes);
      checks++;if(JSON.stringify(lowered.layout.singleViewport?.outputs)!=='[10]')throw new Error('Viewport proof missing');
      const entry=compiled.code.match(/@vertex\s+fn\s+(\w+)/)?.[1],module=device.createShaderModule({code:compiled.code});
      const pipeline=await device.createRenderPipelineAsync({layout:'auto',vertex:{module,entryPoint:entry},fragment:{module:fragment,entryPoint:'fs',targets:[{format:'rgba8unorm'}]},primitive:{topology:'triangle-list'}});
      device.queue.writeBuffer(uniform,0,new Uint32Array([0,base,0,0]));
      const group=device.createBindGroup({layout:pipeline.getBindGroupLayout(3),entries:[{binding:1,resource:{buffer:uniform}}]});
      const encoder=device.createCommandEncoder(),pass=encoder.beginRenderPass({colorAttachments:[{view:target.createView(),loadOp:'clear',storeOp:'store'}]});pass.setPipeline(pipeline);pass.setBindGroup(3,group);pass.draw(3,1,0,base);pass.end();encoder.copyTextureToBuffer({texture:target},{buffer:readback,bytesPerRow:256},[4,4]);device.queue.submit([encoder.finish()]);
      await readback.mapAsync(GPUMapMode.READ);const data=new Uint8Array(readback.getMappedRange()).slice();readback.unmap();
      for(let y=0;y<4;y++)for(let x=0;x<4;x++)for(let c=0;c<4;c++){checks++;if(data[y*256+x*4+c]!==[0,255,0,255][c])throw new Error(`Viewport/relative-instance pixel mismatch with firstInstance ${base}`);}
    }
    const error=await device.popErrorScope();if(error)throw new Error(error.message);
    return {ok:true,checks,scope:'Self-authored GLSL/SPIR-V viewport-zero proof and real GPU pixels at firstInstance 0/9; nonzero/unproven outputs rejected; no game frame proof'};
  }finally{for(const resource of resources)resource.destroy();}
}
