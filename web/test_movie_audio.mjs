import assert from 'node:assert/strict';
import {audioTrackConfig,MovieAudioDecoder} from './movie-audio.mjs';

const identification=new Uint8Array(30);identification.set([1,118,111,114,98,105,115]);identification[11]=2;new DataView(identification.buffer).setUint32(12,48000,true);identification[28]=0xb8;identification[29]=1;
const comment=Uint8Array.of(3,118,111,114,98,105,115),setup=Uint8Array.of(5,118,111,114,98,105,115);
const codecPrivate=Uint8Array.from([2,30,7,...identification,...comment,...setup]);
const track={type:2,codecId:'A_VORBIS',sampleRate:48000,channels:2,codecPrivate};
assert.equal(audioTrackConfig(track).codec,'vorbis');
for(const bad of [{...track,channels:1},{...track,sampleRate:44100},{...track,codecId:'A_OPUS'},{...track,codecPrivate:codecPrivate.slice(0,12)}])assert.throws(()=>audioTrackConfig(bad));
const invalid=codecPrivate.slice();invalid[3+29]=0;assert.throws(()=>audioTrackConfig({...track,codecPrivate:invalid}));

let closed=0;
globalThis.EncodedAudioChunk=class{constructor(value){Object.assign(this,value);}};
globalThis.AudioDecoder=class{
  static async isConfigSupported(config){return {supported:true,config};}
  constructor(callbacks){this.callbacks=callbacks;this.state='configured';this.decodeQueueSize=0;this.chunks=[];}
  configure(){} addEventListener(){}
  decode(chunk){if(this.reject)throw Error('Decode refused');this.chunks.push(chunk);}
  emit(timestamp,channels=2){this.callbacks.output({timestamp,sampleRate:48000,numberOfChannels:channels,numberOfFrames:16,duration:333,allocationSize(options){assert.deepEqual(options,{format:'s16',planeIndex:0});return 16*channels*2;},copyTo(bytes){bytes.fill(17);},close(){closed++;}});}
  async flush(){} close(){this.state='closed';}
};
const decoder=await MovieAudioDecoder.create(track,{capacity:2}),sample={timestamp:0,data:Uint8Array.of(0)};
decoder.submit(sample);assert.equal(decoder.occupied,0); // Actual Vorbis priming, no invented output.
decoder.submit({...sample,timestamp:1});decoder.submit({...sample,timestamp:2});
assert.equal(decoder.decoder.decodeQueueSize,0);assert.equal(decoder.occupied,2);assert.equal(decoder.canSubmit(sample),false);
decoder.decoder.emit(0);decoder.decoder.emit(333);assert.equal(decoder.frames.length,2);
const held=decoder.take({retain:true});assert.equal(held.data.length,64);assert.equal(decoder.canSubmit(sample),false);
decoder.release(held);assert.equal(decoder.canSubmit(sample),true);assert.throws(()=>decoder.release(held));
decoder.decoder.reject=true;assert.throws(()=>decoder.submit(sample));assert.equal(decoder.occupied,1);assert.equal(decoder.awaitingOutput,0);
assert.equal(decoder.take().timestamp,333);assert.equal(decoder.occupied,0);await decoder.flush();decoder.close();
decoder.decoder.emit(3);assert.equal(closed,3);assert.equal(decoder.frames.length,0);
const missing=await MovieAudioDecoder.create(track);missing.submit(sample);missing.submit(sample);await assert.rejects(()=>missing.flush(),/expected PCM/);missing.close();
const changed=await MovieAudioDecoder.create(track);changed.submit(sample);changed.submit(sample);changed.decoder.emit(0,1);assert.match(String(changed.failure),/format changed/);changed.close();
console.log('PASS: Vorbis headers, actual priming contract, delayed output backpressure, PCM leases, bounds, format and error lifetimes');
