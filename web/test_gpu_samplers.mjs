/** Descriptor, unsupported-mode and cache lifetime checks; GPU pixels are
 * verified separately by test-gpu-samplers.mjs in a browser. */
import assert from 'node:assert/strict';
import {samplerDescriptor,WebGpuSamplers} from './gpu-samplers.mjs';
import {WebGpuBackend} from './gpu-backend.mjs';
function value(change=()=>{}){
  const bytes=new Uint8Array(48),v=new DataView(bytes.buffer);
  bytes.set([5,1,7,7,7,0,2,0]);v.setFloat32(12,1000,true);v.setFloat32(20,1,true);change(v,bytes);return bytes;
}
const context={mipLevelCount:5};
for(const [low,high,expectedLow,expectedHigh]of[[-1000,1000,0,1000],[-0.25,0.5,0,0.5]]){
  const descriptor=samplerDescriptor(value(v=>{v.setFloat32(8,low,true);v.setFloat32(12,high,true);}),context);
  assert.equal(descriptor.lodMinClamp,expectedLow);assert.equal(descriptor.lodMaxClamp,expectedHigh);
}
assert.deepEqual(samplerDescriptor(value(),context),{addressModeU:'clamp-to-edge',addressModeV:'clamp-to-edge',addressModeW:'clamp-to-edge',
  minFilter:'linear',magFilter:'linear',mipmapFilter:'linear',lodMinClamp:0,lodMaxClamp:1000,maxAnisotropy:1});
for(const [filter,min,mip]of[[0,'nearest','nearest'],[1,'linear','nearest'],[2,'nearest','nearest'],[3,'linear','nearest'],[4,'nearest','linear'],[5,'linear','linear']]){
  const bytes=value((v,b)=>{b[0]=filter;});
  const result=samplerDescriptor(bytes,{mipLevelCount:1});assert.equal(result.minFilter,min);assert.equal(result.mipmapFilter,mip);
  if(filter<2)assert.throws(()=>samplerDescriptor(bytes,context),/multi-level/);
}
for(const [mode,address]of[[1,'repeat'],[6,'mirror-repeat'],[7,'clamp-to-edge']]){
  const result=samplerDescriptor(value((v,b)=>{b[2]=b[3]=b[4]=mode;}),context);
  assert.equal(result.addressModeU,address);assert.equal(result.addressModeV,address);assert.equal(result.addressModeW,address);
}
for(const [code,name]of['never','less','equal','less-equal','greater','not-equal','greater-equal','always'].entries())
  assert.equal(samplerDescriptor(value((v,b)=>{b[5]=1;b[6]=code+1;}),context).compare,name);
for(const [change,pattern]of[
  [(v,b)=>{b[0]=6;},/enum/],[(v,b)=>{b[1]=2;},/enum/],[(v,b)=>{b[5]=2;},/enum/],[(v,b)=>{b[5]=1;b[6]=0;},/enum/],
  ...[0,2,3,4,5,8,255].map(mode=>[(v,b)=>{b[2]=mode;},/border\/mirror/]),
  [(v)=>{v.setFloat32(8,-1,true);v.setFloat32(12,-2,true);},/LOD range/],[(v)=>v.setFloat32(8,1001,true),/LOD range/],
  [(v)=>{v.setFloat32(8,-1000,true);v.setFloat32(12,-1,true);},/LOD range/],
  [(v)=>v.setFloat32(8,-Infinity,true),/LOD range/],[(v)=>v.setFloat32(8,NaN,true),/LOD range/],
  [(v)=>v.setFloat32(12,NaN,true),/LOD range/],[(v)=>v.setFloat32(16,1,true),/bias/],[(v)=>v.setFloat32(44,.25,true),/snap/],
  [(v,b)=>{b[7]=1;},/reduction/],[(v)=>v.setFloat32(20,1.5,true),/anisotropy/],[(v)=>v.setFloat32(20,17,true),/anisotropy/],
  [(v,b)=>{v.setFloat32(20,2,true);b[0]=4;},/non-linear/]
])assert.throws(()=>samplerDescriptor(value(change),context),pattern);
assert.throws(()=>samplerDescriptor(new Uint8Array(47),context),/48 bytes/);
assert.throws(()=>samplerDescriptor(value()),/mip count/);
const padded=new Uint8Array(56);padded.set(value(),4);assert.deepEqual(samplerDescriptor(padded.subarray(4,52),context),samplerDescriptor(value(),context));

let serial=0,reject=false,release;
const device={createSampler(descriptor){if(reject)throw new Error('GPU rejected sampler');return {id:++serial,descriptor};}};
const cache=new WebGpuSamplers(device,async action=>action(),{capacity:2});
const first=await cache.get(value(),context);assert.equal(await cache.get(value(),context),first);
assert.equal(await cache.get(value(v=>v.setFloat32(8,-1000,true)),context),first);
const second=value((v,b)=>{b[2]=1;});await cache.get(second,context);
// A hit makes the first descriptor most recently used; the second is evicted.
await cache.get(value(),context);await cache.get(value((v,b)=>{b[2]=6;}),context);
assert.equal(await cache.get(value(),context),first);const before=serial;await cache.get(second,context);assert.equal(serial,before+1);
assert.equal(cache.cache.size,2);assert.equal(cache.statistics.samplerCacheEvictions,2);
reject=true;const failing=value(v=>v.setFloat32(8,2,true));await assert.rejects(cache.get(failing,context),/GPU rejected/);
reject=false;assert.ok(await cache.get(failing,context));
cache.close();assert.equal(cache.statistics.cachedSamplers,0);await assert.rejects(cache.get(value(),context),/closed/);
const late=new WebGpuSamplers(device,action=>new Promise(resolve=>{release=()=>resolve(action());}));
const pending=late.get(value(),context);late.close();release();await assert.rejects(pending,/closed/);assert.equal(late.statistics.samplersCreated,0);
console.log('NVN sampler descriptors, mode guards, bounded LRU, failure recovery and late close: PASS (mock device)');

let scopes=0,unblock,hold=true,submissions=0;
const queuedDevice={limits:new Proxy({},{get:()=>4096}),lost:new Promise(()=>{}),addEventListener(){},
  pushErrorScope(){scopes++;},popErrorScope(){
    if(hold&&!unblock)return new Promise(resolve=>{unblock=()=>{scopes--;resolve(null);};});
    scopes--;return Promise.resolve(null);
  },createSampler(descriptor){return {descriptor};},queue:{submit(){submissions++;}}};
const backend=new WebGpuBackend(queuedDevice),mutable=value();
const creation=backend.materializeSampler(mutable,context);mutable[0]=255;
const flush=backend.request(5);
await new Promise(resolve=>setTimeout(resolve,0));assert.equal(submissions,0);assert.equal(scopes,2);
hold=false;unblock();assert.equal((await creation).descriptor.minFilter,'linear');await flush;assert.equal(submissions,1);assert.equal(scopes,0);
await assert.rejects(backend.materializeSampler(mutable,context),/enum/);
assert.equal((await backend.materializeSampler(value(),context)).descriptor.minFilter,'linear');
backend.close();await assert.rejects(backend.materializeSampler(value(),context),/closed/);
assert.equal(backend.statistics.cachedSamplers,0);
console.log('Sampler owner queue, input snapshot, failed request recovery and close: PASS (mock device)');
