import assert from 'node:assert/strict';
import {WebmDemuxer} from './movie-webm.mjs';
import {videoFrameNv12,videoTrackConfig} from './movie-video.mjs';

const cat=(...arrays)=>Uint8Array.from(arrays.flatMap(a=>[...a]));
const bytes=(...n)=>Uint8Array.from(n),utf=s=>new TextEncoder().encode(s);
function size(n){for(let width=1;width<=8;width++)if(BigInt(n)<(1n<<BigInt(width*7))-1n){let value=BigInt(n)|(1n<<BigInt(width*7));const out=new Uint8Array(width);for(let i=width-1;i>=0;i--){out[i]=Number(value&255n);value>>=8n;}return out;}throw Error('size');}
function element(id,data){const hex=id.toString(16).padStart(Math.ceil(id.toString(16).length/2)*2,'0');return cat(Uint8Array.from(hex.match(/../g),s=>parseInt(s,16)),size(data.length),data);}
const u=(id,n)=>element(id,n<256?bytes(n):bytes(n>>8,n&255));
const header=element(0x1a45dfa3,element(0x4282,utf('webm')));
const info=element(0x1549a966,element(0x2ad7b1,bytes(0x0f,0x42,0x40)));
const tracks=element(0x1654ae6b,element(0xae,cat(u(0xd7,1),u(0x83,1),element(0x86,utf('V_VP9')),element(0x23e383,bytes(0x00,0x98,0x96,0x80)),element(0xe0,cat(u(0xb0,4),u(0xba,2))))));
const cluster=data=>element(0x1f43b675,cat(u(0xe7,100),data));
const file=data=>cat(header,element(0x18538067,cat(info,tracks,data)));
const simple=(flags,data)=>element(0xa3,cat(bytes(0x81,0xff,0xfe,flags),data));
const collect=async source=>{const d=await new WebmDemuxer(source).open();const result=[];for await(const s of d.samples())result.push(s);return result;};

// Signed block timestamps, true key/delta flags, and caller-owned sample bytes.
let source=file(cluster(cat(simple(0x80,bytes(1,2,3)),simple(0,bytes(4,5)))));
let samples=await collect(source);
assert.deepEqual(samples.map(s=>[s.timestamp,s.key,[...s.data]]),[[98000,true,[1,2,3]],[98000,false,[4,5]]]);
assert.equal(samples[0].duration,10000);source.fill(0);assert.deepEqual([...samples[0].data],[1,2,3]);
source=file(cluster(simple(0x80,bytes(6))));
assert.equal((await collect(new Blob([source])))[0].data[0],6);
assert.equal((await collect({size:source.length,async read(offset,length){return source.slice(offset,offset+length);}}))[0].data[0],6);
await assert.rejects(()=>collect({size:source.length,async read(){return bytes();}}),/Short/);

// Each lacing mode has a different independent byte layout and expected sizes.
for(const [flags,payload,expected] of [
  [0x82,bytes(2,2,1,11,12,21,31,32,33),[[11,12],[21],[31,32,33]]],
  [0x84,bytes(2,11,12,21,22,31,32),[[11,12],[21,22],[31,32]]],
  [0x86,bytes(3,0x83,0xbd,0xc2,11,12,13,21,31,32,33,34,41,42),[[11,12,13],[21],[31,32,33,34],[41,42]]],
]){
  samples=await collect(file(cluster(simple(flags,payload))));
  assert.deepEqual(samples.map(s=>[...s.data]),expected);
  assert.deepEqual(samples.map(s=>s.timestamp),expected.map((_,i)=>98000+10000*i));
}
// BlockGroup keyframe semantics come from ReferenceBlock, not SimpleBlock bit7.
const group=element(0xa0,cat(element(0xa1,bytes(0x81,0,2,0,7)),u(0x9b,25),u(0xfb,1)));
samples=await collect(file(cluster(group)));assert.equal(samples[0].key,false);assert.equal(samples[0].timestamp,102000);assert.equal(samples[0].duration,25000);
// Unknown-size Segment/Cluster end at the next level-one element.
const unknownCluster=cat(bytes(0x1f,0x43,0xb6,0x75,0xff),u(0xe7,100),simple(0x80,bytes(8)));
source=cat(header,bytes(0x18,0x53,0x80,0x67,0xff),info,tracks,unknownCluster,cluster(simple(0x80,bytes(9))));
assert.deepEqual((await collect(source)).map(s=>s.data[0]),[8,9]);
for(const source of [bytes(),bytes(0),file(cluster(simple(0x84,bytes(2,1,2)))),file(cluster(simple(0x82,bytes(2,255)))),file(cluster(simple(0x86,bytes(2,0xff,1))))])await assert.rejects(()=>collect(source));
source=file(cluster(simple(0x80,bytes(1))));await assert.rejects(()=>collect(source.subarray(0,source.length-1)),/parent|Truncated/);
const foreign=element(0xa3,bytes(0x82,0,0,0x80,1));await assert.rejects(()=>collect(file(cluster(foreign))),/missing track/);

// SDK NV12 conversion, including odd dimensions (four U/V samples for 3x3).
const planar=bytes(1,2,3,4,5,6,7,8,9,10,11,12,13,20,21,22,23);
const frame={visibleRect:{x:0,y:0,width:3,height:3},format:'I420',timestamp:123,duration:10,async copyTo(out,{layout}){assert.deepEqual(layout,[{offset:0,stride:3},{offset:9,stride:2},{offset:13,stride:2}]);out.set(planar);}};
const nv12=await videoFrameNv12(frame);assert.deepEqual([...nv12.data],[1,2,3,4,5,6,7,8,9,10,20,11,21,12,22,13,23]);assert.equal(nv12.uvStride,4);
await assert.rejects(()=>videoFrameNv12({...frame,format:'I010'}),/Unsupported/);
assert.throws(()=>videoTrackConfig({type:2,codecId:'A_VORBIS'}),/Unsupported/);
assert.throws(()=>videoTrackConfig({type:1,codecId:'V_VP9',codecPrivate:bytes(1)}),/CodecPrivate/);
console.log('PASS: WebM real container rules, lacing, timestamps, bounds, and NV12 layout');
