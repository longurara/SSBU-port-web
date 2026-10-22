import {lowerSpirv} from './spirv-compat.mjs';
import {createShaderCompiler} from './shader-compiler.mjs';
import {WebGpuBackend} from './gpu-backend.mjs';
import {WebGpuDepthSamples} from './gpu-depth-sampling.mjs';

export async function testDepthSampling(device,format='depth32float') {
  const fixture=await(await fetch('./depth-sampling-test.json')).json();
  const compiler=await createShaderCompiler(),lowered=lowerSpirv(new Uint32Array(fixture.words).buffer);
  const {code,activeBindings}=compiler.compileWithBindings(lowered.bytes);
  if(/texture_depth_2d/.test(code)||!/texture_2d<f32>/.test(code))throw new Error('Ordinary sampling must retain the original floating image type');
  const resources=[],backend=new WebGpuBackend(device),copies=new WebGpuDepthSamples(backend);device.pushErrorScope('validation');
  const texture=descriptor=>{const resource=device.createTexture(descriptor);resources.push(resource);return resource;};
  try {
    const depth=texture({size:[4,4],format,usage:GPUTextureUsage.RENDER_ATTACHMENT|GPUTextureUsage.TEXTURE_BINDING});
    const color=texture({size:[1,1],format:'rgba8unorm',usage:GPUTextureUsage.COPY_DST|GPUTextureUsage.TEXTURE_BINDING});
    device.queue.writeTexture({texture:color},new Uint8Array([64,0,0,255]),{bytesPerRow:4},[1,1]);
    const target=texture({size:[4,4],format:'rgba8unorm',usage:GPUTextureUsage.RENDER_ATTACHMENT|GPUTextureUsage.COPY_SRC});
    const vertex=device.createShaderModule({code:'@vertex fn vs(@builtin(vertex_index) i:u32)->@builtin(position) vec4<f32>{let p=array<vec2<f32>,3>(vec2<f32>(-1,-1),vec2<f32>(3,-1),vec2<f32>(-1,3));return vec4<f32>(p[i],0,1);}'});
    const fragment=device.createShaderModule({code}),diagnostics=await fragment.getCompilationInfo();
    const errors=diagnostics.messages.filter(message=>message.type==='error');if(errors.length)throw new Error(errors.map(m=>m.message).join('\n'));
    const entry=code.match(/@fragment\s+fn\s+(\w+)/)?.[1];if(!entry)throw new Error('Missing depth fixture entry');
    const pipeline=await device.createRenderPipelineAsync({layout:'auto',vertex:{module:vertex,entryPoint:'vs'},fragment:{module:fragment,entryPoint:entry,targets:[{format:'rgba8unorm'}]},primitive:{topology:'triangle-list'}});
    const sampler=device.createSampler({minFilter:'linear',magFilter:'linear'});
    const readback=device.createBuffer({size:1024,usage:GPUBufferUsage.COPY_DST|GPUBufferUsage.MAP_READ});resources.push(readback);
    const depthEncoder=device.createCommandEncoder();
    const depthFragment=device.createShaderModule({code:'@fragment fn fs()->@builtin(frag_depth) f32{return 0.25;}'});
    const depthPipeline=await device.createRenderPipelineAsync({layout:'auto',vertex:{module:vertex,entryPoint:'vs'},fragment:{module:depthFragment,entryPoint:'fs',targets:[]},depthStencil:{format,depthWriteEnabled:true,depthCompare:'always'},primitive:{topology:'triangle-list'}});
    const clear=depthEncoder.beginRenderPass({colorAttachments:[],depthStencilAttachment:{view:depth.createView(),depthLoadOp:'clear',depthStoreOp:'store',depthClearValue:0.75,...(format.includes('stencil')?{stencilLoadOp:'clear',stencilStoreOp:'store',stencilClearValue:0}:{})}});
    clear.setPipeline(depthPipeline);clear.setScissorRect(0,0,2,4);clear.draw(3);clear.end();device.queue.submit([depthEncoder.finish()]);
    const snapshot=await copies.create({texture:depth,width:4,height:4,dimension:'2d'},{descriptor:{format:format.includes('stencil')?'depth24plus':format,dimension:'2d',aspect:'depth-only',baseMipLevel:0,mipLevelCount:1,baseArrayLayer:0,arrayLayerCount:1}});resources.push(snapshot.texture);
    const group=device.createBindGroup({layout:pipeline.getBindGroupLayout(1),entries:[{binding:0,resource:color.createView()},{binding:2,resource:snapshot.view},{binding:256,resource:sampler},{binding:258,resource:sampler}]});
    const encoder=device.createCommandEncoder();
    const pass=encoder.beginRenderPass({colorAttachments:[{view:target.createView(),loadOp:'clear',storeOp:'store'}]});pass.setPipeline(pipeline);pass.setBindGroup(1,group);pass.draw(3);pass.end();
    encoder.copyTextureToBuffer({texture:target},{buffer:readback,bytesPerRow:256},[4,4]);device.queue.submit([encoder.finish()]);await readback.mapAsync(GPUMapMode.READ);
    const bytes=new Uint8Array(readback.getMappedRange()).slice();readback.unmap();
    const error=await device.popErrorScope();if(error)throw new Error(error.message);
    let checks=2;for(let y=0;y<4;y++)for(let x=0;x<4;x++)for(let c=0;c<4;c++){checks++;if(Math.abs(bytes[y*256+x*4+c]-[64,128,0,255][c])>1)throw new Error('Color/depth linear interpolation pixel mismatch');}
    return {ok:true,checks,format,activeBindings,...copies.statistics,scope:'Self-authored GLSL/SPIR-V and Wasm Naga with real GPU depth -> r32float copies and unchanged filtering sampler; no game pixel parity claim'};
  }finally{backend.close();for(const resource of resources)resource.destroy();}
}
