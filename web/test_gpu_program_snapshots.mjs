/** Retained source revisions for recorded commands, including replacement and
 * close during async compilation. Mock compiler/device: no GPU pixel claim. */
import assert from 'node:assert/strict';
import {WebGpuPrograms} from './gpu-programs.mjs';
import {WebGpuBackend} from './gpu-backend.mjs';
function packet(tag){
  const bytes=new Uint8Array(12+0x720+0x80),v=new DataView(bytes.buffer);
  v.setUint32(0,1,true);v.setUint32(4,0x720,true);v.setUint32(8,0x80,true);
  v.setUint32(12,0x98761234,true);v.setUint32(12+0x6f8,0x50,true);
  v.setUint32(12+0x720,0x12345678,true);v.setUint32(12+0x720+0x30,1<<10,true);
  bytes[12+0x720+0x40]=tag;return bytes;
}
let shaderModules=0,finish;
const device={createShaderModule({code}){shaderModules++;return {code,getCompilationInfo:async()=>({messages:[]})};}};
const factory=async()=>({compile:(control,data)=>({code:`revision-${data[0x40]}`,metadata:{stage:0},layout:{}})});
const programs=new WebGpuPrograms(device,action=>action(),factory);
const original=packet(11),handle=programs.register(0,original),first=programs.retain(handle),second=programs.retain(handle);
original.fill(0);
programs.register(handle,packet(22));const replacement=programs.retain(handle);programs.release(handle);
assert.equal((await programs.compileSnapshot(first))[0].module.code,'revision-11');
assert.equal((await programs.compileSnapshot(replacement))[0].module.code,'revision-22');
const modulesBefore=shaderModules;
assert.equal((await programs.compileSnapshot(second))[0].module.code,'revision-11');assert.equal(shaderModules,modulesBefore);
await assert.rejects(programs.compile(handle),/stale/);assert.throws(()=>programs.retain(handle),/stale/);
programs.releaseSnapshot(first);await assert.rejects(programs.compileSnapshot(first),/stale/);
assert.equal((await programs.compileSnapshot(second))[0].module.code,'revision-11');
assert.throws(()=>programs.releaseSnapshot(first),/stale/);
programs.close();assert.equal(programs.snapshots.size,0);await assert.rejects(programs.compileSnapshot(second),/stale/);
assert.throws(()=>programs.register(0,packet(1)),/closed/);

const delayed=new WebGpuPrograms({createShaderModule:()=>({getCompilationInfo:()=>new Promise(resolve=>{finish=resolve;})})},action=>action(),factory);
const delayedHandle=delayed.register(0,packet(33)),delayedSnapshot=delayed.retain(delayedHandle);
const pending=delayed.compileSnapshot(delayedSnapshot);await new Promise(resolve=>setTimeout(resolve,0));
delayed.register(delayedHandle,packet(44));delayed.release(delayedHandle);finish({messages:[]});
assert.equal((await pending)[0].code,'revision-33');
const abandoned=delayed.compileSnapshot(delayedSnapshot,{topology:'triangles'});await new Promise(resolve=>setTimeout(resolve,0));
delayed.releaseSnapshot(delayedSnapshot);finish({messages:[]});await assert.rejects(abandoned,/released or replaced/);
assert.equal(delayed.compilations,1);delayed.close();

let finishFactory;
const late=new WebGpuPrograms(device,action=>action(),()=>new Promise(resolve=>{finishFactory=resolve;}));
const lateHandle=late.register(0,packet(55)),lateSnapshot=late.retain(lateHandle),count=shaderModules;
const lateCompilation=late.compileSnapshot(lateSnapshot);late.close();finishFactory(await factory());
await assert.rejects(lateCompilation,/released or replaced/);assert.equal(shaderModules,count);

const bounded=new WebGpuPrograms(device,action=>action(),factory,{snapshotLimit:2}),boundedHandle=bounded.register(0,packet(66));
const refs=Array.from({length:2},()=>bounded.retain(boundedHandle));assert.throws(()=>bounded.retain(boundedHandle),/exhausted/);
bounded.releaseSnapshot(refs[0]);assert.ok(bounded.retain(boundedHandle)>refs.at(-1));bounded.close();
assert.throws(()=>new WebGpuPrograms(device,action=>action(),factory,{snapshotLimit:0}),/Invalid/);

const backendDevice={...device,limits:new Proxy({},{get:()=>4096}),lost:new Promise(()=>{}),addEventListener(){},pushErrorScope(){},popErrorScope:async()=>null};
const backend=new WebGpuBackend(backendDevice);backend.programs.compilerFactory=factory;
const id=(await backend.request(12,{bytes:packet(77)})).value;
const retained=(await backend.request(20,{handle:id})).value;
await backend.request(12,{handle:id,bytes:packet(88)});await backend.request(13,{handle:id});
assert.equal((await backend.compileProgramSnapshot(retained))[0].code,'revision-77');
assert.equal(backend.statistics.livePrograms,0);assert.equal(backend.statistics.liveProgramSnapshots,1);
await backend.request(21,{handle:retained});assert.equal(backend.statistics.liveProgramSnapshots,0);
await assert.rejects(backend.compileProgramSnapshot(retained),/stale/);
const next=(await backend.request(12,{bytes:packet(99)})).value;
assert.equal((await backend.compileProgram(next))[0].code,'revision-99');backend.close();
console.log('Retained shader revisions across replace/finalize, shared variants, bounds, stale tokens, async close and owner queue: PASS (mock compiler/device)');
