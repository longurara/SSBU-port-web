import {lowerSpirv} from './spirv-compat.mjs';
import {createShaderCompiler} from './shader-compiler.mjs';
export async function testSwizzle(device){
 const compiler=await createShaderCompiler(),resources=[];let checks=0,variants=0;
 const check=(v,m)=>{checks++;if(!v)throw new Error(m);};
 device.pushErrorScope('validation');
 try{
  const target=device.createTexture({size:[4,4],format:'rgba32float',usage:GPUTextureUsage.RENDER_ATTACHMENT|GPUTextureUsage.COPY_SRC});resources.push(target);
  const read=device.createBuffer({size:1024,usage:GPUBufferUsage.COPY_DST|GPUBufferUsage.MAP_READ});resources.push(read);
  const vertex=device.createShaderModule({code:'@vertex fn vs(@builtin(vertex_index) i:u32)->@builtin(position) vec4<f32>{let p=array<vec2<f32>,3>(vec2<f32>(-1,-1),vec2<f32>(3,-1),vec2<f32>(-1,3));return vec4<f32>(p[i],0,1);}'});
  const mappings=[[1,1,1,2],[5,4,3,2],[0,1,2,5],[2,3,4,5]];
  for(const mode of ['sample','fetch','gather','uint','sint']){
   const fixture=await(await fetch('./swizzle-'+mode+'.json')).json(),integer=mode==='uint'||mode==='sint',signed=mode==='sint';
   const pixels=Array.from({length:4},(_,i)=>signed?[-8-i,16+i,-32-i,64+i]:[32+i*16,64+i*16,96+i*16,128+i*16]);
   const bytes=integer&&signed?new Int8Array(pixels.flat()):new Uint8Array(pixels.flat());
   const image=device.createTexture({size:[2,2],format:integer?(signed?'rgba8sint':'rgba8uint'):'rgba8unorm',usage:GPUTextureUsage.TEXTURE_BINDING|GPUTextureUsage.COPY_DST});resources.push(image);device.queue.writeTexture({texture:image},bytes,{bytesPerRow:8},[2,2]);
   const list=mode==='gather'?Array.from({length:6},(_,i)=>[2,3,4,i]):mappings;
   for(const mapping of list){
    const lowered=lowerSpirv(new Uint32Array(fixture.words).buffer,{textureSwizzles:{'1/40':mapping}}),generated=compiler.compileWithBindings(lowered.bytes),code=generated.code;
    const fragment=device.createShaderModule({code}),entry=code.match(/@fragment\s+fn\s+(\w+)/)?.[1];
    const errors=(await fragment.getCompilationInfo()).messages.filter(m=>m.type==='error');check(!errors.length,'Swizzle WGSL failed: '+errors.map(m=>m.message).join(';'));
    const pipeline=await device.createRenderPipelineAsync({layout:'auto',vertex:{module:vertex,entryPoint:'vs'},fragment:{module:fragment,entryPoint:entry,targets:[{format:'rgba32float'}]},primitive:{topology:'triangle-list'}});
    const entries=[];if(generated.activeBindings.some(b=>b[0]===1&&b[1]===40))entries.push({binding:40,resource:image.createView()});if(generated.activeBindings.some(b=>b[0]===1&&b[1]===296))entries.push({binding:296,resource:device.createSampler({minFilter:'linear',magFilter:'linear'})});
    const encoder=device.createCommandEncoder(),pass=encoder.beginRenderPass({colorAttachments:[{view:target.createView(),loadOp:'clear',storeOp:'store'}]});pass.setPipeline(pipeline);if(entries.length)pass.setBindGroup(1,device.createBindGroup({layout:pipeline.getBindGroupLayout(1),entries}));pass.draw(3);pass.end();encoder.copyTextureToBuffer({texture:target},{buffer:read,bytesPerRow:256},[4,4]);device.queue.submit([encoder.finish()]);
    await read.mapAsync(GPUMapMode.READ);const values=new Float32Array(read.getMappedRange()).slice();read.unmap();
    const normalized=pixels.map(p=>p.map(n=>integer?n:n/255)),base=mode==='sample'?Array.from({length:4},(_,c)=>normalized.reduce((sum,p)=>sum+p[c],0)/4):normalized[0];
    const expected=mode==='gather'?[2,3,1,0].map(i=>mapping[3]<2?mapping[3]:normalized[i][mapping[3]-2]):mapping.map(n=>n<2?n:base[n-2]);
    for(let y=0;y<4;y++)for(let x=0;x<4;x++)for(let c=0;c<4;c++)check(Math.abs(values[y*64+x*4+c]-expected[c])<(integer?1e-6:0.005),`${mode} mapping ${mapping} channel${c}: ${values[y*64+x*4+c]} != ${expected[c]}`);
    variants++;
   }
  }
  const error=await device.popErrorScope();if(error)throw new Error(error.message);
  return {ok:true,checks,variants,scope:'Actual SPIR-V to Wasm Naga to GPU pixels: linear samples, fetch, gather channel selection/constants, unsigned and signed integers; not game pixels'};
 }finally{for(const r of resources)r.destroy();}
}
