import assert from 'node:assert/strict';
import {MovieBackend,MovieOp as OP,MovieStatus as ST} from './movie-backend.mjs';

// Transport/lifetime fixture: deterministic codec callbacks make all 48 output
// slots fill, then all eight SDK inputs wait for releases. Real media decoding
// is checked separately in the browser, never replaced by this test codec.
globalThis.EncodedVideoChunk=class{constructor(value){Object.assign(this,value);}};
globalThis.VideoDecoder=class{
  static async isConfigSupported(config){return {supported:true,config};}
  constructor(callbacks){this.callbacks=callbacks;this.state='configured';this.decodeQueueSize=0;}
  configure(){} addEventListener(){}
  decode(chunk){this.callbacks.output({format:'NV12',timestamp:chunk.timestamp,visibleRect:{x:0,y:0,width:2,height:2},async copyTo(bytes){bytes.fill(chunk.timestamp);},close(){}});}
  async flush(){} close(){this.state='closed';}
};
const events=[],backend=new MovieBackend({notify:(...event)=>events.push(event)});
const track={number:1,type:1,codecId:'V_VP9',codecPrivate:new Uint8Array(),width:2,height:2};
const source=backend.add({kind:'extractor',demux:{tracks:[track]},keys:new Map([[1,[{timestamp:0}]]]),eof:false});
const call=(operation,handle,args={})=>backend.request({operation,handle,...args});
const decoder=(await call(OP.CONFIGURE,source,{flags:123,time:8})).value;
const d=backend.entries.get(decoder);
const submit=(i,time)=>call(OP.SUBMIT,decoder,{index:i,time,bytes:Uint8Array.of(0x82,0)});
await assert.rejects(()=>call(OP.EOS,decoder),e=>e.status===ST.INVALID);
for(let i=0;i<48;i++){
  assert.equal((await submit(i%8,i)).status,0);
  assert.equal((await call(OP.INPUT_AVAILABLE,decoder)).value,1<<(i%8));
}
await d.decoder.chain;assert.equal(d.decoder.occupied,48);
for(let i=0;i<8;i++)assert.equal((await submit(i,48+i)).status,0);
assert.equal(d.inputs.length,8);assert.equal(d.busyInputs,255);
assert.equal((await call(OP.INPUT_AVAILABLE,decoder)).value,0);
await assert.rejects(()=>submit(0,56),e=>e.status===ST.INVALID);
await assert.rejects(()=>submit(8,56),e=>e.status===ST.INVALID);
backend.entries.get(source).eof=true;
assert.equal((await call(OP.EOS,decoder)).status,0);
assert.equal(d.eos,false);assert.equal(d.draining,false); // Drain cannot bypass queued media.
await assert.rejects(()=>submit(0,56),e=>e.status===ST.INVALID);
let count=0,mask=0;
while(count<56){
  await d.decoder.chain;
  const reply=await call(OP.ACQUIRE,decoder);assert.equal(reply.status,0);
  const v=new DataView(reply.bytes.buffer);assert.equal(Number(v.getBigInt64(8,true)),count);
  const frame=(await call(OP.COPY,decoder,{index:reply.value})).bytes;
  assert.ok(frame.every(byte=>byte===count));
  if(count===0){assert.equal(d.inputs.length,8);assert.equal(d.decoder.occupied,48);}
  await call(OP.RELEASE,decoder,{index:reply.value});
  mask|=(await call(OP.INPUT_AVAILABLE,decoder)).value;count++;
}
await d.decoder.chain;await Promise.resolve();
assert.equal(mask,255);assert.equal(d.inputs.length,0);assert.equal(d.decoder.occupied,0);
assert.equal(d.eos,true);
const eos=await call(OP.ACQUIRE,decoder);assert.equal(eos.status,0);assert.equal(eos.value,0);assert.equal(new DataView(eos.bytes.buffer).getUint32(20,true),4);
assert.equal((await call(OP.ACQUIRE,decoder)).status,ST.NO_OUTPUT);
await call(OP.RELEASE,decoder,{index:0});
assert.ok(events.some(([owner,bits])=>owner===123&&(bits&1)));
assert.ok(events.some(([owner,bits])=>owner===123&&(bits&2)));
await call(OP.RESET,decoder);assert.equal(d.eos,false);assert.equal(d.busyInputs,0);assert.equal(d.returnedInputs,0);
assert.equal((await call(OP.STATUS,decoder)).status,0);
const old=d.decoder;old.fail(Error('Actual codec failure'));assert.equal((await call(OP.STATUS,decoder)).status,ST.FAILED);
assert.ok(events.some(([owner,bits])=>owner===123&&(bits&8)));
await call(OP.RESET,decoder);assert.equal((await call(OP.STATUS,decoder)).status,0);
const eventsBefore=events.length;old.onChange();assert.equal(events.length,eventsBefore);
// Reset must invalidate pending packets and old output leases, including a
// drain promise that finishes after the replacement decoder is configured.
for(let i=0;i<48;i++){await submit(i%8,i);await call(OP.INPUT_AVAILABLE,decoder);}
for(let i=0;i<8;i++)await submit(i,48+i);
await d.decoder.chain;
const staleLease=(await call(OP.ACQUIRE,decoder)).value;
const full=d.decoder;await call(OP.RESET,decoder);
assert.equal(full.closed,true);assert.equal(full.occupied,0);assert.equal(d.inputs.length,0);
assert.equal(d.held.size,0);assert.equal(d.busyInputs,0);
await assert.rejects(()=>call(OP.RELEASE,decoder,{index:staleLease}),e=>e.status===ST.INVALID);
assert.equal((await call(OP.ACQUIRE,decoder)).status,ST.NO_OUTPUT);
let finishDrain;const previous=d.decoder;
previous.decoder.flush=()=>new Promise(resolve=>{finishDrain=resolve;});
await call(OP.EOS,decoder);assert.equal(d.draining,true);
await call(OP.RESET,decoder);finishDrain();await previous.chain;await Promise.resolve();await Promise.resolve();
assert.equal(d.eos,false);assert.equal(d.draining,false);
const beforeClose=events.length;
await call(OP.DECODER_CLOSE,decoder);d.decoder.onChange();assert.equal(events.length,beforeClose);
await call(OP.CLOSE,source);assert.equal(backend.entries.size,0);await backend.close();
console.log('PASS: eight SDK input slots, 48 retained outputs, nonblocking EOS drain, reset and stale notification suppression');
