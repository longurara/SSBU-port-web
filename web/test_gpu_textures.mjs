import assert from 'node:assert/strict';
import {textureFormat,parseTexturePacket,WebGpuTextures} from './gpu-textures.mjs';
const device={limits:{maxTextureDimension1D:8192,maxTextureDimension2D:8192,maxTextureDimension3D:2048,maxTextureArrayLayers:256},features:new Set(['texture-compression-bc'])};
function packet(nvn=0x25,width=2,height=2,pixels=new Uint8Array(width*height*4)) {
  const data=new Uint8Array(60+pixels.length),view=new DataView(data.buffer);
  [0x5854564e,nvn,1,width,height,1,1,0,1,0,0,width,height,1,pixels.length].forEach((value,i)=>view.setUint32(i*4,value,true));
  data.set(pixels,60);return data;
}
const bytes=packet(),parsed=parseTexturePacket(bytes,device);bytes.fill(255);
assert.equal(parsed.format.format,'rgba8unorm');assert.ok(parsed.subresources[0].bytes.every(value=>value===0));
const rgb=new Uint8Array(12),rgbView=new DataView(rgb.buffer);
for(const [i,value] of [0x3f800000,0xc0000000,0x7fc12345].entries())rgbView.setUint32(i*4,value,true);
const expanded=parseTexturePacket(packet(0x22,1,1,rgb),device);
assert.equal(expanded.format.format,'rgba32float');
assert.deepEqual(Array.from(new Uint32Array(expanded.subresources[0].bytes.buffer)),[0x3f800000,0xc0000000,0x7fc12345,0x3f800000]);
const integers=parseTexturePacket(packet(0x23,1,1,rgb),device);
assert.equal(integers.format.format,'rgba32uint');assert.equal(new DataView(integers.subresources[0].bytes.buffer).getUint32(12,true),1);
assert.equal(textureFormat(0x49).format,'bc4-r-unorm');assert.equal(textureFormat(0x50).format,'bc6h-rgb-ufloat');
assert.throws(()=>parseTexturePacket(packet(0x42,4,4,new Uint8Array(8)),{...device,features:new Set()}),/texture-compression-bc/);
assert.throws(()=>parseTexturePacket(packet(0x79,4,4,new Uint8Array(16)),device),/texture-compression-astc/);
const edgeBlocks=Uint8Array.from({length:16},(_,i)=>i+1);
const edge=parseTexturePacket(packet(0x42,5,4,edgeBlocks),device);
assert.equal(edge.width,5);assert.equal(edge.gpuWidth,8);assert.equal(edge.gpuHeight,4);
assert.deepEqual(edge.subresources[0].bytes,edgeBlocks);
assert.throws(()=>parseTexturePacket(packet(0x42,5,4,new Uint8Array(8)),device),/length/);
assert.throws(()=>parseTexturePacket(packet(0x25,0,1,new Uint8Array()),device),/dimensions/);
assert.throws(()=>parseTexturePacket(packet().subarray(0,63),device),/length/);
assert.throws(()=>parseTexturePacket(new Uint8Array([...packet(),0]),device),/Trailing/);
assert.equal(textureFormat(0x35).format,'depth24plus-stencil8');
assert.equal(textureFormat(0x35).depth,true);assert.equal(textureFormat(0x35).stencil,true);
assert.equal(textureFormat(0x36).feature,'depth32float-stencil8');
const packed=parseTexturePacket(packet(0x39,3,1,new Uint8Array([0x0f,0xf0,0x0f,0x0f,0xff,0x00])),device);
assert.deepEqual(Array.from(packed.subresources[0].bytes),[255,0,0,255,0,255,0,255,0,0,255,255]);
assert.throws(()=>textureFormat(0x100),/Unsupported/);
globalThis.GPUTextureUsage={COPY_SRC:1,COPY_DST:2,TEXTURE_BINDING:4,RENDER_ATTACHMENT:8};
let destroyed=0,invalid=true,writes=0;
const mock={...device,createTexture:()=>({destroy(){destroyed++;},createView:descriptor=>({descriptor})}),queue:{writeTexture(){writes++;},submit(){}}};
const textures=new WebGpuTextures(mock,async action=>{action();if(invalid)throw new Error('validation failed');});
await assert.rejects(textures.create(packet()),/validation/);assert.equal(destroyed,1);assert.equal(textures.records.size,0);
invalid=false;const handle=await textures.create(packet());assert.equal(textures.statistics.liveTextures,1);
function copyPacket(commands) {
  const length=4+commands.reduce((sum,command)=>sum+36+command.bytes.length,0),bytes=new Uint8Array(length),view=new DataView(bytes.buffer);
  view.setUint32(0,commands.length,true);let offset=4;
  for(const command of commands) {
    const fields=[handle,0,command.x??0,0,0,command.width??2,2,1,command.bytes.length];
    fields.forEach((value,i)=>view.setUint32(offset+i*4,value,true));offset+=36;bytes.set(command.bytes,offset);offset+=command.bytes.length;
  }
  return bytes;
}
const before=writes;
await assert.rejects(textures.submitCopies(copyPacket([{bytes:new Uint8Array(16)},{x:2,bytes:new Uint8Array(16)}])),/region/);
assert.equal(writes,before);assert.equal(textures.statistics.bufferTextureCopies,0);
await textures.submitCopies(copyPacket([{bytes:new Uint8Array(16)}]));
assert.equal(writes,before+1);assert.equal(textures.statistics.bufferTextureCopies,1);assert.equal(textures.statistics.commandSubmissions,1);
const native=new Uint8Array(18),nativeView=new DataView(native.buffer);native[0]=0x85;nativeView.setUint16(4,0x38,true);nativeView.setInt16(8,1,true);
const viewHandle=await textures.createView(handle,native);native.fill(0);
assert.equal(textures.getView(viewHandle).descriptor.format,'rgba8unorm-srgb');assert.equal(textures.getView(viewHandle).native[0],0x85);
const invalidView=new Uint8Array(18);invalidView[0]=0x81;new DataView(invalidView.buffer).setInt16(6,99,true);
await assert.rejects(textures.createView(handle,invalidView),/mip\/layer/);assert.equal(textures.statistics.liveTextureViews,1);
textures.destroy(handle);assert.throws(()=>textures.get(handle),/stale/);
assert.throws(()=>textures.getView(viewHandle),/stale/);textures.releaseView(viewHandle);assert.equal(textures.statistics.liveTextureViews,0);
await assert.rejects(textures.submitCopies(copyPacket([{bytes:new Uint8Array(16)}])),/stale/);
const nanDepth=new Uint8Array(4);new DataView(nanDepth.buffer).setUint32(0,0x7fc00000,true);
assert.throws(()=>textures.depth.validate(0x34,nanDepth),/finite \[0,1\]/);
const outOfRangeDepth=new Uint8Array(8);new DataView(outOfRangeDepth.buffer).setFloat32(0,1.25,true);
assert.throws(()=>textures.depth.validate(0x36,outOfRangeDepth),/finite \[0,1\]/);
textures.close();await assert.rejects(textures.create(packet()),/closed/);
console.log('Texture packet/format/precision/bounds/lifetime checks: PASS (CPU parser and mock device; not GPU pixel validation)');
