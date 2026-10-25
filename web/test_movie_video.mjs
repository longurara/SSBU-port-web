import assert from 'node:assert/strict';
import {MovieVideoDecoder,videoPacketOutputs} from './movie-video.mjs';

const codec='vp09.00.10.08',packet=(...bytes)=>Uint8Array.of(...bytes);
assert.equal(videoPacketOutputs('vp8',packet(0x10,0,0)),1);
assert.equal(videoPacketOutputs('vp8',packet(0,0,0)),0);
assert.equal(videoPacketOutputs(codec,packet(0x82,0)),1);
assert.equal(videoPacketOutputs(codec,packet(0x84,0)),0); // Invisible reference.
assert.equal(videoPacketOutputs(codec,packet(0x88)),1); // Show existing frame.
assert.equal(videoPacketOutputs(codec,packet(0x84,0,0x82,0,0xc1,2,2,0xc1)),1);
assert.equal(videoPacketOutputs(codec,packet(0x82,0,0x88,0xc1,2,1,0xc1)),2);
assert.equal(videoPacketOutputs(codec,packet(0x84,0,0xc0,2,0xc0)),0);
for(const data of [packet(),packet(0),packet(0xb2),packet(0x82,0xc1),packet(0x82,0,0xc0,9,0xc0),packet(0x82,0,0xc0,0,0xc0),packet(0x82,0,0xc0,1,0xc0)]){
  assert.throws(()=>videoPacketOutputs(codec,data));
}
// Simulate the difficult WebCodecs scheduling case: dequeue occurs before
// delayed output callbacks. Queue size zero must not restore output capacity.
let frameCloses=0;
globalThis.EncodedVideoChunk=class{constructor(value){Object.assign(this,value);}};
globalThis.VideoDecoder=class{
  constructor(callbacks){this.callbacks=callbacks;this.state='configured';this.decodeQueueSize=0;this.chunks=[];}
  configure(){} addEventListener(){}
  decode(chunk){if(this.reject)throw Error('Decode rejected');this.chunks.push(chunk);}
  emit(timestamp){this.callbacks.output({format:'NV12',timestamp,visibleRect:{x:0,y:0,width:2,height:2},async copyTo(bytes){bytes.fill(timestamp);},close(){frameCloses++;}});}
  async flush(){} close(){this.state='closed';}
};
const decoder=new MovieVideoDecoder({codec},{capacity:2});
const sample={data:packet(0x82,0),timestamp:1,key:true};
decoder.submit(sample);decoder.submit({...sample,timestamp:2});
assert.equal(decoder.decoder.decodeQueueSize,0);assert.equal(decoder.occupied,2);
assert.equal(decoder.available,false);assert.equal(decoder.canSubmit(sample),false);
// Hidden references need no new display slot, even while visible slots are full.
assert.equal(decoder.canSubmit({...sample,data:packet(0x84,0)}),true);
decoder.submit({...sample,data:packet(0x84,0)});assert.equal(decoder.occupied,2);
decoder.decoder.emit(1);decoder.decoder.emit(2);await decoder.chain;
assert.equal(decoder.frames.length,2);assert.equal(decoder.awaitingOutput,0);
const lease=decoder.take({retain:true});assert.equal(decoder.occupied,2);
assert.equal(decoder.canSubmit(sample),false); // Acquiring is not releasing.
decoder.release(lease);assert.equal(decoder.occupied,1);assert.equal(decoder.canSubmit(sample),true);
assert.throws(()=>decoder.release(lease));
decoder.decoder.reject=true;assert.throws(()=>decoder.submit(sample));assert.equal(decoder.occupied,1);assert.equal(decoder.awaitingOutput,0);
assert.equal(decoder.take().timestamp,2);assert.equal(decoder.occupied,0);
await decoder.flush();assert.equal(frameCloses,2);decoder.close();
assert.equal(decoder.canSubmit(sample),false);
const missing=new MovieVideoDecoder({codec},{capacity:2});missing.submit(sample);
await assert.rejects(()=>missing.flush(),/all visible frames/);missing.close();
const partial=new MovieVideoDecoder({codec},{capacity:2});let decoded=0;
partial.decoder.decode=()=>{if(decoded++)throw Error('Second coded frame rejected');};
assert.throws(()=>partial.submit({...sample,data:packet(0x84,0,0x82,0,0xc1,2,2,0xc1)}));
assert.equal(partial.occupied,0);assert.equal(partial.awaitingOutput,0);assert.ok(partial.failure);partial.close();
const extra=new MovieVideoDecoder({codec},{capacity:2});extra.decoder.emit(3);
assert.match(String(extra.failure),/reserved storage/);assert.equal(frameCloses,3);extra.close();
console.log('PASS: VP8/VP9 visible-frame accounting, delayed output backpressure, retained leases and decode failures');
