/** Controlled queue promises test scheduling/timeout edge cases. Actual GPU
 * completion and the C bridge are exercised separately by gpu_probe.c. */
import assert from 'node:assert/strict';
import {WebGpuBackend} from './gpu-backend.mjs';
let complete;
let pending=new Promise(resolve=>{complete=resolve;});
const device={limits:new Proxy({},{get:()=>4096}),lost:new Promise(()=>{}),addEventListener(){},
  pushErrorScope(){},async popErrorScope(){return null;},queue:{submit(){},onSubmittedWorkDone:()=>pending}};
const backend=new WebGpuBackend(device);
function timeout(value) {const bytes=new Uint8Array(8);new DataView(bytes.buffer).setBigUint64(0,value,true);return bytes;}
async function bounded(promise) {
  let timer;
  try{return await Promise.race([promise,new Promise((_,reject)=>{timer=setTimeout(()=>reject(new Error('request queue stalled behind CPU fence wait')),200);})]);}
  finally{clearTimeout(timer);}
}
const first=(await backend.request(8)).value;
assert.equal((await backend.request(9,{handle:first,bytes:timeout(0n)})).value,2);
const start=performance.now();
assert.equal((await backend.request(9,{handle:first,bytes:timeout(2_000_000n)})).value,2);
assert.ok(performance.now()-start>=2);
const infinite=backend.request(9,{handle:first,bytes:timeout(0xffffffffffffffffn)});
const finish=backend.request(6);
assert.equal((await bounded(backend.request(7))).value,0);
assert.equal((await bounded(backend.request(11,{handle:first}))).value,1);
complete();assert.equal((await bounded(infinite)).value,1);assert.equal((await bounded(finish)).value,0);
assert.equal((await backend.request(9,{handle:first,bytes:timeout(0n)})).value,0);
await backend.request(10,{handle:first});
assert.equal((await backend.request(9,{handle:first,bytes:timeout(0n)})).value,3);

pending=new Promise(resolve=>{complete=resolve;});
const second=(await backend.request(8)).value;assert.ok(second>first);
// A high uint64 timeout must not be truncated to zero/32 bits or fed to the
// signed 32-bit setTimeout API as one overflowing delay.
const large=backend.request(9,{handle:second,bytes:timeout(1n<<63n)});
await bounded(backend.request(7));complete();assert.equal((await bounded(large)).value,1);
await backend.request(10,{handle:second});

pending=new Promise(()=>{});
const third=(await backend.request(8)).value;
const cancelled=backend.request(9,{handle:third,bytes:timeout(0xffffffffffffffffn)});
await bounded(backend.request(7));backend.close();
assert.equal((await bounded(cancelled)).value,3);
assert.equal(backend.statistics.liveFences,0);
console.log('GPU fence scheduling: PASS (poll/finite/uint64/infinite waits, unrelated request progress, stale handles, close)');
