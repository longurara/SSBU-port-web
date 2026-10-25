import assert from 'node:assert/strict';
import {MovieBackend,MovieOp as OP,MovieStatus as ST} from './movie-backend.mjs';
// Container/selection fixture only. Tiny payloads are intentionally not coded
// media; real codec/SDK hashes are checked by test_movie_av.py.
const join=(...p)=>Uint8Array.from(p.flatMap(x=>[...x]));
function el(id,data){const h=id.toString(16).padStart(Math.ceil(id.toString(16).length/2)*2,'0');let width=1;while(data.length>=2**(7*width)-1)width++;let n=data.length+2**(7*width);const size=new Uint8Array(width);for(let i=width-1;i>=0;i--){size[i]=n&255;n=Math.floor(n/256);}return join(Uint8Array.from(h.match(/../g),x=>parseInt(x,16)),size,data);}
const u=(id,n)=>el(id,Uint8Array.of(n)),s=(id,str)=>el(id,new TextEncoder().encode(str));
const identification=new Uint8Array(30);identification.set([1,118,111,114,98,105,115]);identification[11]=2;new DataView(identification.buffer).setUint32(12,48000,true);identification[28]=0xb8;identification[29]=1;
const privateBytes=join(Uint8Array.of(2,30,7),identification,Uint8Array.of(3,118,111,114,98,105,115),Uint8Array.of(5,118,111,114,98,105,115));
const video=el(0xae,join(u(0xd7,1),u(0x83,1),s(0x86,'V_VP9'),el(0xe0,join(u(0xb0,4),u(0xba,2)))));
const rate=new Uint8Array(8);new DataView(rate.buffer).setFloat64(0,48000);
const audio=el(0xae,join(u(0xd7,2),u(0x83,2),s(0x86,'A_VORBIS'),el(0x63a2,privateBytes),el(0xe1,join(el(0xb5,rate),u(0x9f,2)))));
const block=(track,time,byte)=>el(0x1f43b675,join(u(0xe7,time),el(0xa3,Uint8Array.of(128+track,0,0,time===0?128:0,byte))));
const data=join(el(0x1a45dfa3,s(0x4282,'webm')),el(0x18538067,join(el(0x1549a966,el(0x2ad7b1,Uint8Array.of(15,66,64))),el(0x1654ae6b,join(video,audio)),block(1,0,10),block(2,20,20),block(1,40,30),block(2,60,40),block(1,80,50))));
const backend=new MovieBackend(),call=(op,id,args={})=>backend.request({operation:op,handle:id,...args}),id=(await call(OP.CREATE)).value;
await call(OP.APPEND,id,{bytes:data});assert.equal((await call(OP.OPEN,id)).value,2);
assert.equal((await call(OP.PEEK,id)).status,ST.END);assert.equal(backend.entries.get(id).eof,false);
await call(OP.SELECT,id,{index:0,flags:1});
const peek=async()=>{const r=await call(OP.PEEK,id);return r.status?{status:r.status}:{status:0,track:new DataView(r.bytes.buffer).getUint32(0,true),time:Number(new DataView(r.bytes.buffer).getBigInt64(8,true))};};
assert.deepEqual(await peek(),{status:0,track:0,time:0});
await call(OP.SELECT,id,{index:1,flags:1});assert.deepEqual(await peek(),{status:0,track:0,time:0});assert.deepEqual([...(await call(OP.READ,id)).bytes],[10]);
await call(OP.ADVANCE,id);assert.deepEqual(await peek(),{status:0,track:1,time:20000});
await call(OP.SELECT,id,{index:1,flags:0});assert.deepEqual(await peek(),{status:0,track:0,time:40000});
await call(OP.SELECT,id,{index:1,flags:1});assert.deepEqual(await peek(),{status:0,track:0,time:40000});
await call(OP.ADVANCE,id);assert.deepEqual(await peek(),{status:0,track:1,time:60000});
await call(OP.SELECT,id,{index:0,flags:0});assert.deepEqual(await peek(),{status:0,track:1,time:60000});
await call(OP.ADVANCE,id);assert.equal((await peek()).status,ST.END);assert.equal(backend.entries.get(id).eof,true);
await call(OP.SELECT,id,{index:0,flags:1});assert.equal((await peek()).status,ST.END); // No implicit rewind at EOF.
await call(OP.SEEK,id,{time:0});assert.deepEqual(await peek(),{status:0,track:0,time:0});
await call(OP.CLOSE,id);await backend.close();
console.log('PASS: simultaneous video/audio selection, pending sample ownership, live selection changes, actual EOF and explicit seek');
