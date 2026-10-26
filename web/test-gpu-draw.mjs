import {WebGpuBackend} from './gpu-backend.mjs';

/** Self-authored WGSL injected only into this fixture's compiler instance.
 * Production draws compile retained Maxwell bytes; these pixels verify the
 * real pipeline/buffer/bind-group/scissor/blending/index path independently. */
export async function createFixtureGraphicsCompiler() {
  return {compile(control,data){
    const vertex=(new DataView(data.buffer,data.byteOffset).getUint32(0x30,true)>>>10)===1;
    const variant=new DataView(control.buffer,control.byteOffset).getUint32(0x704,true),flat=variant===0x464c4154,textured=!vertex&&variant===0x54455854,noOutput=!vertex&&variant===0x4e4f5554;
    const flatVertex='@group(3) @binding(0) var<uniform> params:vec4<u32>; struct Out{@builtin(position) p:vec4<f32>,@location(0) @interpolate(flat) shade:vec4<f32>}; @vertex fn vertex_main(@location(0) p:vec4<f32>,@builtin(vertex_index) i:u32)->Out{let colors=array<vec4<f32>,3>(vec4<f32>(1,0,0,1),vec4<f32>(0,1,0,1),vec4<f32>(0,0,1,1));return Out(vec4<f32>(p.xyz,p.w*bitcast<f32>(params.z)),colors[i%3]);}';
    const flatFragment='@fragment fn fragment_main(@location(0) @interpolate(flat) shade:vec4<f32>)->@location(0) vec4<f32>{return shade;}';
    return {code:noOutput?'@fragment fn fragment_main(){}':flat?(vertex?flatVertex:flatFragment):vertex?'@group(3) @binding(0) var<uniform> params:vec4<u32>; @vertex fn vertex_main(@location(0) p:vec4<f32>)->@builtin(position) vec4<f32>{return vec4<f32>(p.xyz,p.w*bitcast<f32>(params.z));}':
      '@group(0) @binding(42) var<storage,read> unused:array<u32>; @group(0) @binding(43) var<storage,read> color:array<f32>;'+(textured?'@group(1) @binding(40) var image:texture_2d<f32>; @group(1) @binding(296) var smp:sampler;':'')+
      '@fragment fn fragment_main(@builtin(position) p:vec4<f32>)->@location(0) vec4<f32>{return '+(textured?'textureSampleLevel(image,smp,p.xy/vec2<f32>(textureDimensions(image)),0.0)*':'')+'vec4<f32>(color[0],color[1],color[2],color[3]);}',
      activeBindings:vertex?[[3,0]]:flat||noOutput?[]:[[0,43],...(textured?[[1,40],[1,296]]:[])],
      metadata:{stage:vertex?0:1,inputAttributes:vertex||flat?1:0,cbufs:vertex||flat||noOutput?0:12,ssbos:0,ssbosWritten:0,images:0,textures:textured?1:0},layout:{pushConstant:vertex?{group:3,binding:0}:null,samplers:textured?[{group:1,imageBinding:40,samplerBinding:296}]:[]}};
  }};
}
export async function testGpuDraw(device,{shaderLowering=false}={}) {
  const backend=new WebGpuBackend(device,{diagnosticFrames:true});let checks=0;
  const check=(value,message)=>{checks++;if(!value)throw new Error(message);};
  backend.programs.compilerFactory=createFixtureGraphicsCompiler;
  const program=type=>{
    const bytes=new Uint8Array(4+8+0x720+0x80),v=new DataView(bytes.buffer);v.setUint32(0,1,true);v.setUint32(4,0x720,true);v.setUint32(8,0x80,true);
    v.setUint32(12,0x98761234,true);v.setUint32(12+0x6f8,0x50,true);v.setUint32(12+0x720,0x12345678,true);v.setUint32(12+0x720+0x30,type<<10,true);return bytes;
  };
  const buffer=async data=>{const size=Math.ceil(data.byteLength/4)*4,bytes=new Uint8Array(size);bytes.set(new Uint8Array(data.buffer,data.byteOffset,data.byteLength));const {value:handle}=await backend.request(1,{size});await backend.request(2,{handle,offset:0,size,bytes});return handle;};
  const texture=new Uint8Array(60+64),t=new DataView(texture.buffer);[0x5854564e,0x25,1,4,4,1,1,0,1,0,0,4,4,1,64].forEach((n,i)=>t.setUint32(i*4,n,true));
  try {
    const snapshots=[];for(const type of [1,5]){const {value:h}=await backend.request(12,{bytes:program(type)});snapshots.push((await backend.request(20,{handle:h})).value);}
    const vertices=await buffer(new Float32Array([-1,-1,0.25,1,3,-1,0.25,1,-1,3,0.25,1]));
    const uniform=await buffer(new Float32Array([0,0.2,0.4,0.6,1,0,0,0]));
    const indices=await buffer(new Uint16Array([0,1,2,0]));
    const {value:target}=await backend.request(14,{bytes:texture});
    const p={version:1,programs:snapshots.slice(),primitive:[4,-1,3,0,0,1,0,0],attributes:Array(32).fill(0),streams:Array.from({length:32},()=>[0,0,0,0,0]),
      ubo:Array.from({length:2},()=>Array.from({length:15},()=>[0,0,0])),ssbo:Array.from({length:2},()=>Array.from({length:16},()=>[0,0,0])),textures:Array.from({length:2},()=>Array(32).fill(null)),
      blend:Array.from({length:8},(_,i)=>[i,2,1,2,1,1,1,0]),color:[0,3,8,0],polygon:[0,1,2,0],ds:Array(8).fill(0),stencil:[0,0,255,255,255,255],
      viewport:[0,0,4,4,0,1],scissor:[0,0,4,4],blendColor:[0,0,0,0],colors:[{handle:target,mip:0,layer:0,format:0,layers:1},...Array(7).fill(null)],depth:null,
      channelMask:0xffffffff,sampleMask:0xffffffff,alphaRef:0,pointSize:1,flags:[0,0,0,0,0]};
    p.attributes[0]=0x2e;p.streams[0]=[16,0,vertices,0,48];p.ubo[1][0]=[uniform,4,16];
    const draw=()=>backend.request(24,{bytes:new TextEncoder().encode(JSON.stringify(p))});
    await draw();let read=(await backend.request(16,{handle:target,offset:0,size:0})).bytes;
    for(let pixel=0;pixel<16;pixel++)for(let c=0;c<4;c++)check(Math.abs(read[pixel*4+c]-[51,102,153,255][c])<=1,'Triangle/uniform pixels differ');
    check(backend.statistics.draws===1&&backend.statistics.drawBufferCopies===1,'Draw or unaligned storage staging was not executed');
    await backend.request(2,{handle:uniform,offset:4,size:16,bytes:new Uint8Array(new Float32Array([1,0,0,0.5]).buffer)});
    p.primitive=[4,1,3,0,0,1,indices,0];p.scissor=[1,1,2,2];p.color[0]=1;p.blend[0]=[0,5,6,2,6,1,1,0];
    await draw();read=(await backend.request(16,{handle:target,offset:0,size:0})).bytes;
    for(let y=0;y<4;y++)for(let x=0;x<4;x++)for(let c=0;c<4;c++)check(Math.abs(read[(y*4+x)*4+c]-(x>=1&&x<3&&y>=1&&y<3?[153,51,77,255]:[51,102,153,255])[c])<=1,'Indexed blend/scissor pixels differ');
    check(backend.statistics.indexedDraws===1,'Index draw was skipped');
    p.streams[0][2]=999999;
    let rejected=false;try{await draw();}catch{rejected=true;}check(rejected,'Stale vertex buffer was accepted');
    check(backend.statistics.draws===2,'Failed draw advanced counters');
    const unchanged=(await backend.request(16,{handle:target,offset:0,size:0})).bytes;check(unchanged.every((n,i)=>n===read[i]),'Rejected draw changed target pixels');
    const textured=program(5);new DataView(textured.buffer).setUint32(12+0x704,0x54455854,true);
    const {value:fp}=await backend.request(12,{bytes:textured});p.programs[1]=(await backend.request(20,{handle:fp})).value;
    const source=texture.slice();for(let i=0;i<16;i++)source.set([255,128,64,255],60+i*4);
    const {value:sourceHandle}=await backend.request(14,{bytes:source});
    const {value:view}=await backend.request(18,{handle:sourceHandle,bytes:new Uint8Array(18)});
    const sampler=new Uint8Array(48),sv=new DataView(sampler.buffer);sampler.set([0,0,7,7,7,0,0,0]);sv.setFloat32(12,1,true);sv.setFloat32(20,1,true);
    p.textures[1][0]={texture:sourceHandle,view,sampler:Array.from(sampler)};p.streams[0][2]=vertices;p.color[0]=0;p.scissor=[0,0,4,4];p.primitive=[4,-1,3,0,0,1,0,0];
    await backend.request(2,{handle:uniform,offset:4,size:16,bytes:new Uint8Array(new Float32Array([1,1,1,1]).buffer)});
    await draw();read=(await backend.request(16,{handle:target,offset:0,size:0})).bytes;
    for(let pixel=0;pixel<16;pixel++)for(let c=0;c<4;c++)check(Math.abs(read[pixel*4+c]-[255,128,64,255][c])<=1,'Textured draw pixels differ');
    check(backend.statistics.samplersCreated===1,'Draw sampler was not materialized');
    const halfwordIndices=await buffer(new Uint16Array([42,0,1,2]));p.primitive=[4,1,3,0,0,1,halfwordIndices,2];
    await backend.request(2,{handle:uniform,offset:4,size:16,bytes:new Uint8Array(new Float32Array([0.5,1,1,1]).buffer)});
    await draw();read=(await backend.request(16,{handle:target,offset:0,size:0})).bytes;
    for(let pixel=0;pixel<16;pixel++)for(let c=0;c<4;c++)check(Math.abs(read[pixel*4+c]-[128,128,64,255][c])<=1,'16-bit index offset was not realigned correctly');
    check(backend.statistics.convertedIndexDraws===1,'Halfword index range did not use conversion');
    for(const [stage,type]of [1,5].entries()){
      const code=program(type);new DataView(code.buffer).setUint32(12+0x704,0x464c4154,true);
      const {value:handle}=await backend.request(12,{bytes:code});p.programs[stage]=(await backend.request(20,{handle})).value;
    }
    const clear=async()=>{const bytes=new Uint8Array(64),v=new DataView(bytes.buffer);v.setUint32(0,1,true);v.setUint32(4,target,true);v.setUint32(20,15,true);await backend.request(22,{bytes});};
    p.primitive=[4,-1,3,0,0,1,0,0];await clear();await draw();
    read=(await backend.request(16,{handle:target,offset:0,size:0})).bytes;
    for(let pixel=0;pixel<16;pixel++)check(read[pixel*4]===0&&read[pixel*4+2]===255,'Flat color did not come from NVN last vertex');
    const stripVertices=await buffer(new Float32Array([-1,-1,0.25,1,1,-1,0.25,1,-1,1,0.25,1,1,1,0.25,1]));
    p.streams[0]=[16,0,stripVertices,0,64];p.primitive=[5,-1,4,0,0,1,0,0];p.polygon[0]=2;await clear();await draw();
    read=(await backend.request(16,{handle:target,offset:0,size:0})).bytes;
    check(read[(3*4)*4+2]===255&&read[3*4]===255,'Strip parity/culling or last flat vertex differs');
    const restartIndices=await buffer(new Uint8Array([0,1,2,255,1,3,2,255]));
    p.primitive=[5,0,7,0,0,1,restartIndices,0];p.flags[2]=1;await clear();await draw();
    read=(await backend.request(16,{handle:target,offset:0,size:0})).bytes;
    for(let pixel=0;pixel<16;pixel++)check(read[pixel*4]===0&&read[pixel*4+2]===255,'8-bit strip restart was not reset correctly');
    const highVertexData=new Float32Array(256*4);highVertexData.set([-1,-1,0.25,1,3,-1,0.25,1]);highVertexData.set([-1,3,0.25,1],255*4);
    const highVertices=await buffer(highVertexData),byteIndices=await buffer(new Uint8Array([42,0,1,255]));
    p.streams[0]=[16,0,highVertices,0,highVertexData.byteLength];p.primitive=[4,0,3,0,0,1,byteIndices,1];p.flags[2]=0;await clear();await draw();
    read=(await backend.request(16,{handle:target,offset:0,size:0})).bytes;
    for(let pixel=0;pixel<16;pixel++)check(read[pixel*4]===255&&read[pixel*4+2]===0,'Disabled restart treated vertex 255 as a sentinel');
    // Scalar/three-channel fetches retain the implicit w=1, including native
    // scaled integers and half floats which WebGPU cannot bind as float16x3.
    const packedBytes=new Uint8Array(12);packedBytes.set([255,255,0,3,255,0,255,3,0],1);
    const fetchCases=[
      {format:0x22,stride:12,data:new Float32Array([-1,-1,0,3,-1,0,-1,3,0])},
      {format:0x1d,stride:6,data:new Uint16Array([0xbc00,0xbc00,0,0x4200,0xbc00,0,0xbc00,0x4200,0])},
      {format:0x5e,stride:3,offset:1,size:9,data:packedBytes}
    ];
    p.polygon[0]=0;p.primitive=[4,-1,3,0,0,1,0,0];
    for(const test of fetchCases){const handle=await buffer(test.data);p.attributes[0]=test.format;p.streams[0]=[test.stride,0,handle,test.offset||0,test.size||test.data.byteLength];
      await clear();await draw();read=(await backend.request(16,{handle:target,offset:0,size:0})).bytes;
      for(let pixel=0;pixel<16;pixel++)check(read[pixel*4]===0&&read[pixel*4+2]===255,`NVN fetch 0x${test.format.toString(16)} pixels differ`);}
    check(backend.statistics.convertedVertexStreams===2,'Half/scaled fetches did not execute conversion');
    const normalized=await buffer(new Int16Array([-32768,-32768,0,32767,-32768,0,-32768,32767,0]));
    p.attributes[0]=0x1f;p.streams[0]=[6,0,normalized,0,18];await clear();await draw();
    read=(await backend.request(16,{handle:target,offset:0,size:0})).bytes;
    check(read[(3*4)*4+2]===255&&read[3*4+2]===0,'Signed normalized 16-bit fetch/default w differs');
    const prefixed=new Uint8Array(16);prefixed.set(packedBytes.subarray(1,10),3);
    const prefixedHandle=await buffer(prefixed);p.attributes[0]=0x5e;p.streams[0]=[3,0,prefixedHandle,0,12];p.primitive=[4,1,3,1,0,1,indices,0];
    await clear();await draw();read=(await backend.request(16,{handle:target,offset:0,size:0})).bytes;
    // vertex_index includes the base vertex: last index 2 + base 1 => red.
    for(let pixel=0;pixel<16;pixel++)check(read[pixel*4]===255&&read[pixel*4+2]===0,'Converted fetch lost indexed base vertex');
    p.streams[0]=[3,1,prefixedHandle,0,12];p.primitive=[4,-1,3,0,1,1,0,0];
    // A constant instance position cannot form a triangle; validation still
    // exercises firstInstance bounds without rebasing the source stream.
    await clear();await draw();read=(await backend.request(16,{handle:target,offset:0,size:0})).bytes;
    check(read.every(n=>n===0),'Instance fetch unexpectedly used per-vertex positions');
    const noOutput=program(5);new DataView(noOutput.buffer).setUint32(12+0x704,0x4e4f5554,true);
    const {value:noOutputHandle}=await backend.request(12,{bytes:noOutput});p.programs=[snapshots[0],(await backend.request(20,{handle:noOutputHandle})).value];
    p.attributes[0]=0x2e;p.streams[0]=[16,0,vertices,0,48];p.primitive=[4,-1,3,0,0,1,0,0];
    const beforeDraw=backend.statistics.draws;await clear();await draw();read=(await backend.request(16,{handle:target,offset:0,size:0})).bytes;
    check(backend.statistics.draws===beforeDraw+1&&read.every(n=>n===0),'Fragment without color outputs must preserve a bound color target');
    p.programs=snapshots.slice();p.ds=[0x17,0x11,0x11,0x11,0x11,0,0,0];
    await clear();await draw();read=(await backend.request(16,{handle:target,offset:0,size:0})).bytes;
    for(let pixel=0;pixel<16;pixel++)for(let c=0;c<4;c++)check(Math.abs(read[pixel*4+c]-[128,255,255,255][c])<=1,'Unattached depth/stencil tests must pass even with NEVER functions');
    const capture=backend.draw.diagnosticDraws,first=capture.records[0];
    check(!capture.lastError&&capture.records.length>=4,'Actual draw input capture failed');
    check(JSON.stringify(capture).length<=capture.byteLimit,'Draw input diagnostics exceeded report budget');
    check(first.draw===1&&first.packet.primitive[1]===-1&&first.shaders.length===2,'First successful draw capture differs');
    check(first.shaders.every(s=>s.control&&s.data&&s.options.singleInstanceBase===0),'Retained shader source/options missing from draw capture');
    const decode=text=>Uint8Array.from(atob(text),c=>c.charCodeAt(0));
    const input=first.buffers.find(b=>b.kind==='vertex'),constant=first.buffers.find(b=>b.kind==='uniform');
    check(input.complete&&input.capturedBytes===48&&new Float32Array(decode(input.data).buffer)[4]===3,'Captured vertex data differs from GPU buffer');
    check(constant.complete&&constant.range[1]===4&&Math.abs(new Float32Array(decode(constant.data).buffer)[0]-0.2)<1e-6,'Unaligned uniform GPU capture differs');
    const prefixPacket=structuredClone(first.packet),large=new Uint8Array(8192);large[0]=73;large[8191]=91;
    const largeBuffer=await buffer(large);prefixPacket.streams[0]=[16,0,largeBuffer,0,8192];
    await backend.draw.captureInputs('fixture-prefix',prefixPacket,first.shaders,first.shaders.map(s=>s.options));
    const prefix=capture.records.at(-1).buffers.find(b=>b.kind==='vertex');
    check(!prefix.complete&&prefix.range[2]===8192&&prefix.capturedBytes===4096&&decode(prefix.data)[0]===73,'Large range capture must explicitly retain only its bounded prefix');
    const priorCount=capture.records.length,priorLimit=capture.byteLimit;capture.byteLimit=1;
    await backend.draw.captureInputs('fixture-budget',first.packet,first.shaders,first.shaders.map(s=>s.options));
    check(capture.records.length===priorCount&&capture.omitted.includes('limit'),'Draw capture report budget was not enforced');capture.byteLimit=priorLimit;
    const perform=backend.perform;backend.perform=async()=>{throw new Error('injected draw capture failure');};
    await backend.draw.captureInputs('fixture-error',first.packet,first.shaders,first.shaders.map(s=>s.options),new Error('fixture draw failed'));
    check(capture.lastError.includes('injected draw capture failure'),'Diagnostic read failure was not retained separately');backend.perform=perform;
    if(device.features.has('float32-filterable')&&device.features.has('texture-compression-bc')){
      const compressed=new Uint8Array(68),v=new DataView(compressed.buffer);[0x5854564e,0x42,1,3,3,1,1,0,1,0,0,3,3,1,8].forEach((n,i)=>v.setUint32(i*4,n,true));v.setUint16(60,0xf800,true);v.setUint16(62,0xf800,true);
      const {value:image}=await backend.request(14,{bytes:compressed}),{value:imageView}=await backend.request(18,{handle:image,bytes:new Uint8Array(18)});
      p.programs=[snapshots[0],(await backend.request(20,{handle:fp})).value];p.attributes[0]=0x2e;p.streams[0]=[16,0,vertices,0,48];p.primitive=[4,-1,3,0,0,1,0,0];p.ds.fill(0);
      p.textures[1][0]={texture:image,view:imageView,sampler:Array.from(sampler)};
      await clear();await draw();read=(await backend.request(16,{handle:target,offset:0,size:0})).bytes;
      for(let pixel=0;pixel<16;pixel++)for(let c=0;c<4;c++)check(Math.abs(read[pixel*4+c]-[128,0,0,255][c])<=1,'Logical compressed view did not reach the actual draw/sampler');
      check(backend.draw.statistics.logicalSamplingCopies===1,'Logical copy was not counted through draw submission');
    }
    if(shaderLowering){
    const {lowerSpirv}=await import('./spirv-compat.mjs'),{createShaderCompiler}=await import('./shader-compiler.mjs');
    // Real SPIR-V lowering behind the actual immutable packet/variant path.
    // This fixture shader replaces only the fixture compiler's TEXT marker.
    const originalCompiler=backend.programs.compiler,wgsl=await createShaderCompiler(),fixture=await(await fetch('./swizzle-array.json')).json();
    let seenSwizzle=false;
    backend.programs.compiler={compile(control,data,options){if(options.textureSwizzles&&new DataView(control.buffer,control.byteOffset).getUint32(0x704,true)===0x54455854){
      seenSwizzle=JSON.stringify(options.textureSwizzles['1/40'])==='[1,1,1,2]';
      const lowered=lowerSpirv(new Uint32Array(fixture.words).buffer,options),generated=wgsl.compileWithBindings(lowered.bytes);
      return {code:generated.code,activeBindings:generated.activeBindings,layout:lowered.layout,metadata:{stage:1,inputAttributes:0,cbufs:0,ssbos:0,ssbosWritten:0,images:0,textures:1}};
    }return originalCompiler.compile(control,data,options);}};
    const maskSource=texture.slice();for(let pixel=0;pixel<16;pixel++)maskSource.set([64,128,192,32],60+pixel*4);
    const {value:mask}=await backend.request(14,{bytes:maskSource}),{value:maskView}=await backend.request(18,{handle:mask,bytes:new Uint8Array(18)});
    p.programs=[snapshots[0],(await backend.request(20,{handle:fp})).value];p.attributes[0]=0x2e;p.streams[0]=[16,0,vertices,0,48];p.primitive=[4,-1,3,0,0,1,0,0];p.ds.fill(0);
    p.textures[1][0]={texture:mask,view:maskView,sampler:Array.from(sampler),swizzle:[1,1,1,2]};
    await clear();await draw();read=(await backend.request(16,{handle:target,offset:0,size:0})).bytes;
    check(backend.draw.statistics.arraySamplingViews===1,'Shader array declaration did not promote the single-layer 2D view');
    check(seenSwizzle,'Actual draw did not forward immutable descriptor swizzle to compiler variant');
    for(let pixel=0;pixel<16;pixel++)for(let c=0;c<4;c++)check(Math.abs(read[pixel*4+c]-[255,255,255,64][c])<=1,'Actual shader-swizzled mask draw differs');
    // Promote only the selected native 2D slice: mip/layer rebasing must not
    // accidentally sample the atlas's first layer or base mip.
    const pieces=[],header=new Uint32Array([0x5854564e,1,4,4,4,2,2,0,4]);pieces.push(new Uint8Array(header.buffer));
    for(let layer=0;layer<2;layer++)for(let mip=0;mip<2;mip++){const extent=4>>mip,value=layer===1&&mip===1?192:11+layer*32+mip*16;pieces.push(new Uint8Array(new Uint32Array([mip,layer,extent,extent,1,extent*extent]).buffer),new Uint8Array(extent*extent).fill(value));}
    const atlasBytes=new Uint8Array(pieces.reduce((n,p)=>n+p.length,0));let atlasOffset=0;for(const piece of pieces){atlasBytes.set(piece,atlasOffset);atlasOffset+=piece.length;}
    const {value:atlas}=await backend.request(14,{bytes:atlasBytes}),slice=new Uint8Array(18),sliceData=new DataView(slice.buffer);slice[0]=0xa3;slice[1]=1;sliceData.setInt16(6,1,true);sliceData.setInt16(8,1,true);sliceData.setInt16(10,1,true);sliceData.setInt16(12,1,true);
    const {value:sliceView}=await backend.request(18,{handle:atlas,bytes:slice});p.textures[1][0]={texture:atlas,view:sliceView,sampler:Array.from(sampler),swizzle:[1,1,1,2]};
    await clear();await draw();read=(await backend.request(16,{handle:target,offset:0,size:0})).bytes;
    check(backend.draw.statistics.arraySamplingViews===2,'Selected mip/layer did not use the single-layer array view');
    for(let pixel=0;pixel<16;pixel++)for(let c=0;c<4;c++)check(Math.abs(read[pixel*4+c]-[255,255,255,192][c])<=1,'Array promotion lost the native selected mip/layer');
    p.textures[1][0]={texture:mask,view:maskView,sampler:Array.from(sampler),swizzle:[2,3,4,5]};
    await backend.request(2,{handle:uniform,offset:4,size:16,bytes:new Uint8Array(new Float32Array([1,1,1,1]).buffer)});
    await clear();await draw();read=(await backend.request(16,{handle:target,offset:0,size:0})).bytes;
    for(let pixel=0;pixel<16;pixel++)for(let c=0;c<4;c++)check(Math.abs(read[pixel*4+c]-[64,128,192,32][c])<=1,'Identity draw reused a swizzled shader variant');
    p.textures[1][0].swizzle=[1,1,1,9];let invalidSwizzle=false;try{await draw();}catch(error){invalidSwizzle=String(error).includes('Invalid NVN texture swizzle');}check(invalidSwizzle,'Invalid descriptor swizzle was accepted');
    }
    return {checks,failures:0,...backend.draw.statistics};
  }finally{backend.close();}
}
