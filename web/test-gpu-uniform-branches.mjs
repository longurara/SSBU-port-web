import {createGameShaderCompiler} from './game-shaders.mjs';
import {WebGpuBackend} from './gpu-backend.mjs';

/** Pixel equivalence using the actual captured game fragment shader. The
 * fullscreen vertex fixture only supplies its three texture coordinates. */
export async function testGpuUniformBranches(device,record){
  const compiler=await createGameShaderCompiler(),source=record.shaders.find(s=>s.stage===1),decode=s=>Uint8Array.from(atob(s),c=>c.charCodeAt(0));
  const control=decode(source.control),data=decode(source.data),captured=decode(record.buffers.find(b=>b.kind==='uniform'&&b.stage===1&&b.slot===0).data);
  const originalWords=Array.from(new Uint32Array(captured.buffer));let checks=0;
  const check=(condition,message)=>{checks++;if(!condition)throw new Error(message);};
  const base={...source.options};delete base.uniformWords;
  for(const [bits,absent]of [[0,true],[0x80000000,true],[0x3f800000,false],[0xbf800000,false],[0x7fc00000,false],[0x7f800000,false]]){
    const words=originalWords.slice();words[1]=bits;
    const result=compiler.compile(control,data,{...base,uniformWords:{'0/43':words}});
    check(result.activeBindings.some(([g,b])=>g===1&&b===42)!==absent,`Incorrect missing-texture proof for bits ${bits.toString(16)}`);
    check((result.layout.uniformSpecialization?.branches??0)===(absent?8:0),'Unexpected number of proven selections');
    if(absent)check(JSON.stringify(result.layout.uniformSpecialization.reads)==='["0/43/1"]','Proof must identify the actual controlling uniform word');
  }
  for(const uniformWords of [{},{'0/44':originalWords},{'0/43':[]}]){
    const result=compiler.compile(control,data,{...base,uniformWords});
    check(result.activeBindings.some(([g,b])=>g===1&&b===42),'Unknown/absent constant-buffer data must preserve texture access');
  }
  const resources=[];
  const buffer=(bytes,usage)=>{const b=device.createBuffer({size:Math.max(16,bytes.byteLength),usage:usage|GPUBufferUsage.COPY_DST});device.queue.writeBuffer(b,0,bytes);resources.push(b);return b;};
  const texture=(value)=>{const t=device.createTexture({size:[1,1],format:'r8unorm',usage:GPUTextureUsage.TEXTURE_BINDING|GPUTextureUsage.COPY_DST});device.queue.writeTexture({texture:t},new Uint8Array([value]),{},[1,1]);resources.push(t);return t.createView();};
  const views=[texture(128),texture(64),texture(255)],sampler=device.createSampler();
  const target=device.createTexture({size:[4,4],format:'rgba8unorm',usage:GPUTextureUsage.RENDER_ATTACHMENT|GPUTextureUsage.COPY_SRC});resources.push(target);
  const readback=device.createBuffer({size:1024,usage:GPUBufferUsage.COPY_DST|GPUBufferUsage.MAP_READ});resources.push(readback);
  const pc=buffer(new Uint32Array([0,0,0x3f800000,0]),GPUBufferUsage.UNIFORM);
  const vertex=device.createShaderModule({code:'struct Out{@builtin(position) p:vec4<f32>,@location(1) a:vec4<f32>,@location(2) b:vec4<f32>,@location(3) c:vec4<f32>}; @vertex fn main(@builtin(vertex_index) i:u32)->Out{let p=array<vec2<f32>,3>(vec2<f32>(-1,-1),vec2<f32>(3,-1),vec2<f32>(-1,3));return Out(vec4<f32>(p[i],0,1),vec4<f32>(0.5),vec4<f32>(0.5),vec4<f32>(0.5));}'});
  const render=async(words,specialize)=>{
    const result=compiler.compile(control,data,{...base,...(specialize?{uniformWords:{'0/43':words}}:{})});
    const fragment=device.createShaderModule({code:result.code}),info=await fragment.getCompilationInfo();
    check(!info.messages.some(m=>m.type==='error'),'Actual game fragment specialization failed WebGPU validation');
    const pipeline=await device.createRenderPipelineAsync({layout:'auto',vertex:{module:vertex,entryPoint:'main'},fragment:{module:fragment,entryPoint:'main',targets:[{format:'rgba8unorm'}]},primitive:{topology:'triangle-list'}});
    const ub=buffer(new Uint32Array(words),GPUBufferUsage.STORAGE);
    const groups=[device.createBindGroup({layout:pipeline.getBindGroupLayout(0),entries:[{binding:43,resource:{buffer:ub}}]}),device.createBindGroup({layout:pipeline.getBindGroupLayout(1),entries:result.activeBindings.filter(([g])=>g===1).map(([,binding])=>({binding,resource:binding>=256?sampler:views[binding-40]}))}),device.createBindGroup({layout:pipeline.getBindGroupLayout(3),entries:[{binding:0,resource:{buffer:pc}}]})];
    const encoder=device.createCommandEncoder(),pass=encoder.beginRenderPass({colorAttachments:[{view:target.createView(),loadOp:'clear',storeOp:'store',clearValue:[0,0,0,0]}]});
    pass.setPipeline(pipeline);pass.setBindGroup(0,groups[0]);pass.setBindGroup(1,groups[1]);pass.setBindGroup(3,groups[2]);pass.draw(3);pass.end();encoder.copyTextureToBuffer({texture:target},{buffer:readback,bytesPerRow:256},[4,4]);device.queue.submit([encoder.finish()]);await readback.mapAsync(GPUMapMode.READ);const bytes=new Uint8Array(readback.getMappedRange()).slice();readback.unmap();return bytes;
  };
  try{
    // Readback parity includes nonblack color and independent alpha. Both
    // positive and negative zero disable the optional texture on the GPU.
    for(const bits of [0,0x80000000,0x3e4ccccd]){
      const words=originalWords.slice();words[1]=bits;words[8]=0x3e4ccccd;words[9]=0x3ecccccd;words[10]=0x3f19999a;
      const reference=await render(words,false),specialized=await render(words,true);
      for(let y=0;y<4;y++)for(let x=0;x<4;x++)for(let c=0;c<4;c++)check(reference[y*256+x*4+c]===specialized[y*256+x*4+c],'Specialization changed an actual game shader pixel');
      check(reference[0]===51&&reference[1]===102&&reference[2]===153,'Reference draw must produce the configured nonblack color');
      check(Math.abs(reference[3]-(bits===0x3e4ccccd?122:32))<=1,'Actual shader alpha does not match its texture expression');
    }
    // Exercise the production packet -> readback -> specialization path, with
    // no texture at slot2. Enabling the effect must fail rather than reuse the
    // zero-uniform variant; restoring zero must safely reuse it.
    const backend=new WebGpuBackend(device);
    try{
      const vertexCode='struct Out{@builtin(position) p:vec4<f32>,@location(1) a:vec4<f32>,@location(2) b:vec4<f32>,@location(3) c:vec4<f32>}; @vertex fn main(@builtin(vertex_index) i:u32)->Out{let p=array<vec2<f32>,3>(vec2<f32>(-1,-1),vec2<f32>(3,-1),vec2<f32>(-1,3));return Out(vec4<f32>(p[i],0,1),vec4<f32>(0.5),vec4<f32>(0.5),vec4<f32>(0.5));}';
      backend.programs.compilerFactory=async()=>({compile(c,d,o){return ((new DataView(d.buffer,d.byteOffset).getUint32(0x30,true)>>>10)&15)===1?{code:vertexCode,activeBindings:[],metadata:{stage:0,inputAttributes:0,cbufs:0,ssbos:0,ssbosWritten:0,images:0},layout:{samplers:[]}}:compiler.compile(c,d,o);}});
      const snapshots=[];
      for(const stage of [0,1]){
        const s=record.shaders.find(s=>s.stage===stage),c=decode(s.control),d=decode(s.data),packet=new Uint8Array(12+c.length+d.length),v=new DataView(packet.buffer);v.setUint32(0,1,true);v.setUint32(4,c.length,true);v.setUint32(8,d.length,true);packet.set(c,12);packet.set(d,12+c.length);
        const {value:handle}=await backend.request(12,{bytes:packet});snapshots.push((await backend.request(20,{handle})).value);
      }
      const image=async(value)=>{const bytes=new Uint8Array(124),v=new DataView(bytes.buffer);[0x5854564e,0x25,1,4,4,1,1,0,1,0,0,4,4,1,64].forEach((n,i)=>v.setUint32(i*4,n,true));for(let i=0;i<16;i++)bytes.set([value,0,0,255],60+i*4);const {value:handle}=await backend.request(14,{bytes});return {handle,view:(await backend.request(18,{handle,bytes:new Uint8Array(18)})).value};};
      const target=await image(0),images=[await image(128),await image(64),await image(255)],words=originalWords.slice();words[8]=0x3e4ccccd;words[9]=0x3ecccccd;words[10]=0x3f19999a;
      const {value:ub}=await backend.request(1,{size:words.length*4});
      const p=structuredClone(record.packet);p.programs=snapshots;p.primitive=[4,-1,3,0,0,1,0,0];p.attributes.fill(0);p.streams=p.streams.map(()=>[0,0,0,0,0]);p.ubo=p.ubo.map(a=>a.map(()=>[0,0,0]));p.ubo[1][0]=[ub,0,words.length*4];p.ssbo=p.ssbo.map(a=>a.map(()=>[0,0,0]));p.textures=p.textures.map(a=>a.map(()=>null));
      for(let i=0;i<2;i++)p.textures[1][i]={...record.packet.textures[1][i],texture:images[i].handle,view:images[i].view};
      p.colors=[{handle:target.handle,mip:0,layer:0,format:0,layers:1},...Array(7).fill(null)];p.depth=null;p.ds.fill(0);p.color=[0,3,8,0];p.viewport=[0,0,4,4,0,1];p.scissor=[0,0,4,4];
      const update=()=>backend.request(2,{handle:ub,offset:0,size:words.length*4,bytes:new Uint8Array(new Uint32Array(words).buffer)});
      const draw=()=>backend.request(24,{bytes:new TextEncoder().encode(JSON.stringify(p))});
      await update();await draw();
      let pixels=(await backend.request(16,{handle:target.handle,offset:0,size:0})).bytes;
      for(let i=0;i<16;i++)for(let c=0;c<4;c++)check(Math.abs(pixels[i*4+c]-[51,102,153,32][c])<=1,'Missing optional slot did not render correctly through production draw');
      words[1]=0x3e4ccccd;await update();let rejected=false;try{await draw();}catch(e){rejected=String(e).includes('Unbound shader texture 1/2');}
      check(rejected&&backend.statistics.draws===1,'Changed nonzero uniform reused an unsafe missing-texture variant');
      const unchanged=(await backend.request(16,{handle:target.handle,offset:0,size:0})).bytes;check(unchanged.every((n,i)=>n===pixels[i]),'Rejected active missing texture modified target');
      words[1]=0;await update();await draw();check(backend.statistics.draws===2,'Restoring zero did not restore the proved specialization');
      words[1]=0x3e4ccccd;await update();p.textures[1][2]={...p.textures[1][0],texture:images[2].handle,view:images[2].view,swizzle:[2,3,4,5]};await draw();pixels=(await backend.request(16,{handle:target.handle,offset:0,size:0})).bytes;
      check(backend.statistics.draws===3&&Math.abs(pixels[3]-122)<=1,'Bound optional texture must execute the original enabled effect');
    }finally{backend.close();}
    return {ok:true,checks,scope:'Actual captured game fragment: GPU pixel parity and production missing-slot specialization; zero/negative zero remove only proved branches, changed/unknown/NaN values retain the texture requirement'};
  }finally{for(const resource of resources)resource.destroy();}
}
