import assert from 'node:assert/strict';
import {MovieBackend,MovieOp as OP,MovieStatus as ST} from './movie-backend.mjs';
const join=(...parts)=>Uint8Array.from(parts.flatMap(p=>[...p]));
function el(id,data){const hex=id.toString(16).padStart(Math.ceil(id.toString(16).length/2)*2,'0');if(data.length>=127)throw Error('Test element size');return join(Uint8Array.from(hex.match(/../g),s=>parseInt(s,16)),Uint8Array.of(128+data.length),data);}
const u=(id,n)=>el(id,Uint8Array.of(n)),str=(id,s)=>el(id,new TextEncoder().encode(s));
const header=el(0x1a45dfa3,str(0x4282,'webm'));
const info=el(0x1549a966,el(0x2ad7b1,Uint8Array.of(15,66,64)));
const track=el(0xae,join(u(0xd7,1),u(0x83,1),str(0x86,'V_VP9'),el(0xe0,join(u(0xb0,4),u(0xba,2)))));
const audio=el(0xae,join(u(0xd7,2),u(0x83,2),str(0x86,'A_VORBIS')));
const cluster=(time,key,byte)=>el(0x1f43b675,join(u(0xe7,time),el(0xa3,Uint8Array.of(129,0,0,key?128:0,byte))));
const body=join(info,el(0x1654ae6b,join(track,audio)),cluster(0,true,1),cluster(10,false,2),cluster(20,true,3));
// Segment size is two bytes; packets here test transport semantics, not decoding.
const source=join(header,Uint8Array.of(0x18,0x53,0x80,0x67,0x40|(body.length>>8),body.length&255),body);
const backend=new MovieBackend(),call=(operation,handle,args={})=>backend.request({operation,handle,...args});
const id=(await call(OP.CREATE)).value;
await Promise.all([call(OP.APPEND,id,{bytes:source.slice(0,20)}),call(OP.APPEND,id,{bytes:source.slice(20)})]);
assert.equal((await call(OP.OPEN,id)).value,2);
await assert.rejects(()=>call(OP.APPEND,id,{bytes:Uint8Array.of(1)}),e=>e.status===ST.INVALID);
await assert.rejects(()=>call(OP.SELECT,id,{index:1,flags:1}),e=>e.status===ST.UNSUPPORTED);
await call(OP.SELECT,id,{index:0,flags:1});
let peek=await call(OP.PEEK,id);assert.equal(new DataView(peek.bytes.buffer).getBigInt64(8,true),0n);
assert.deepEqual([...(await call(OP.READ,id)).bytes],[1]);await call(OP.ADVANCE,id);
await call(OP.SELECT,id,{index:1,flags:0}); // Unselecting audio must not rewind video.
peek=await call(OP.PEEK,id);assert.equal(new DataView(peek.bytes.buffer).getBigInt64(8,true),10000n);
await call(OP.SEEK,id,{time:15000});peek=await call(OP.PEEK,id);assert.equal(new DataView(peek.bytes.buffer).getBigInt64(8,true),0n);
await call(OP.SEEK,id,{time:20000});assert.deepEqual([...(await call(OP.PEEK,id)).bytes.slice(8,16)],[32,78,0,0,0,0,0,0]);assert.deepEqual([...(await call(OP.READ,id)).bytes],[3]);
await call(OP.ADVANCE,id);assert.equal((await call(OP.PEEK,id)).status,ST.END);
await call(OP.CLOSE,id);assert.equal(backend.entries.size,0);await assert.rejects(()=>call(OP.PEEK,id),e=>e.status===ST.INVALID);
// Every dispatch failure still completes the actual C wait token exactly once;
// failed reads cannot overwrite a caller's staging range.
const completions=[],heap=new Uint8Array(64).fill(0xa5),module={HEAPU8:heap,_movie_complete:(...args)=>completions.push(args)};
await backend.dispatch(module,{token:1,operation:OP.READ,handle:id,pointer:8,length:10});
assert.deepEqual(completions,[[1,ST.INVALID,0]]);assert.ok(heap.every(b=>b===0xa5));assert.equal(backend.pending.size,0);
await backend.dispatch(module,{token:2,operation:OP.CREATE,pointer:63,length:2});assert.deepEqual(completions[1],[2,ST.INVALID,0]);
let takes=0;const mock=backend.add({kind:'decoder',decoder:{take(){takes++;},close(){}}});
await backend.dispatch(module,{token:3,operation:OP.ACQUIRE,handle:mock,pointer:0,length:16});assert.deepEqual(completions[2],[3,ST.INVALID,0]);assert.equal(takes,0);
await backend.close();await assert.rejects(()=>call(OP.CREATE),e=>e.status===ST.INVALID);
console.log('PASS: movie bridge serialization, keyframe seek, selection, EOF, lifetimes and completion errors');
