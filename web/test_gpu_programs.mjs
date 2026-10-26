import assert from 'node:assert/strict';
import {WebGpuPrograms} from './gpu-programs.mjs';
function packet() {
  const bytes=new Uint8Array(12+0x720+0x80),view=new DataView(bytes.buffer);
  view.setUint32(0,1,true);view.setUint32(4,0x720,true);view.setUint32(8,0x80,true);
  view.setUint32(12,0x98761234,true);view.setUint32(12+0x6f8,0x50,true);
  view.setUint32(12+0x720,0x12345678,true);view.setUint32(12+0x720+0x30,1<<10,true);return bytes;
}
let compiled=0,lastOptions;
const programs=new WebGpuPrograms({createShaderModule:()=>({getCompilationInfo:async()=>({messages:[]})})},action=>action(),
  async()=>({compile(control,data,options){compiled++;lastOptions=options;assert.equal(control[0],0x34);return {code:'test',metadata:{stage:0},layout:{}};}}));
const bytes=packet(),handle=programs.register(0,bytes);bytes.fill(0);
assert.equal(programs.get(handle).stages[0].control[0],0x34);
assert.equal(programs.get(handle).stages[0].data[0],0x78);
const first=await programs.compile(handle,{topology:'triangles',integerAttributes:1});
assert.equal(compiled,1);assert.deepEqual(lastOptions,{topology:'triangles',integerAttributes:1});
assert.equal(await programs.compile(handle,{integerAttributes:1,topology:'triangles'}),first);
assert.equal(compiled,1);
await programs.compile(handle,{topology:'triangles',integerAttributes:2});assert.equal(compiled,2);
assert.throws(()=>programs.register(handle,new Uint8Array([1,2,3])),/packet/);
assert.equal(programs.get(handle).variants.size,2);
const trailing=new Uint8Array(packet().length+1);trailing.set(packet());
assert.throws(()=>programs.register(handle,trailing),/Trailing/);
assert.equal(programs.register(handle,packet()),handle);assert.equal(programs.get(handle).variants.size,0);
programs.release(handle);assert.throws(()=>programs.get(handle),/stale/);
assert.throws(()=>programs.register(handle,packet()),/stale/);
const replacement=programs.register(0,packet());assert.ok(replacement>handle);
programs.close();assert.throws(()=>programs.get(replacement),/stale/);
let invalid=true,finish;
const validation=new WebGpuPrograms({createShaderModule:()=>({getCompilationInfo:async()=>({messages:invalid?[{type:'error',message:'invalid WGSL'}]:[]})})},action=>action(),
  async()=>({compile:()=>({code:'test'})}));
const validHandle=validation.register(0,packet());
await assert.rejects(validation.compile(validHandle),/invalid WGSL/);
assert.equal(validation.get(validHandle).variants.size,0);invalid=false;
await validation.compile(validHandle);assert.equal(validation.compilations,1);
validation.device.createShaderModule=()=>({getCompilationInfo:()=>new Promise(resolve=>{finish=resolve;})});
const pending=validation.compile(validHandle,{topology:'triangles'});
await new Promise(resolve=>setTimeout(resolve,0));validation.close();finish({messages:[]});
await assert.rejects(pending,/released or replaced/);
console.log('Program source ownership/cache/replacement/lifetime: PASS (mock compiler/device; not GPU validation)');

// The same immutable program/snapshot can own both stages. Their per-draw
// resource state is different, and compiling the unused fragment with vertex
// samplers reproduces the real post-reward LOD failure.
const combined=new Uint8Array(4+2*(packet().length-4));new DataView(combined.buffer).setUint32(0,2,true);
combined.set(packet().subarray(4),4);const fragmentPacket=packet();new DataView(fragmentPacket.buffer).setUint32(12+0x720+0x30,5<<10,true);
combined.set(fragmentPacket.subarray(4),packet().length);
const calls=[];
const stages=new WebGpuPrograms({createShaderModule:()=>({getCompilationInfo:async()=>({messages:[]})})},action=>action(),async()=>({compile(control,data,options){
  const stage=(new DataView(data.buffer,data.byteOffset).getUint32(0x30,true)>>>10)===1?0:1;
  assert.equal(options.resourceStage,stage,'Stage compiled using another stage\'s resource state');calls.push(stage);return {code:'test',metadata:{stage},layout:{}};
}}));
const both=stages.register(0,combined),snapshot=stages.retain(both);
for(const stage of [1,0])assert.equal((await stages.compileSnapshot(snapshot,{requestedStage:stage,resourceStage:stage}))[0].metadata.stage,stage);
assert.deepEqual(calls,[1,0]);
await stages.compileSnapshot(snapshot,{requestedStage:1,resourceStage:1});assert.deepEqual(calls,[1,0]);
await stages.compileSnapshot(snapshot,{requestedStage:1,resourceStage:1,variant:2});assert.deepEqual(calls,[1,0,1]);
await assert.rejects(stages.compileSnapshot(snapshot,{requestedStage:2}),/Invalid requested/);
stages.close();console.log('Combined graphics program stage/resource selection and independent variant cache: PASS');
