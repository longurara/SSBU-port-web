import assert from 'node:assert/strict';
import {specializeUniformBranches} from './spirv-uniform-branches.mjs';
const op=(code,...a)=>[(a.length+1)<<16|code,...a];
function fixture({readOnly=true,stride=4,offset=0,copy=false}={}){
 const w=[0x07230203,0x10000,0,64,0,
 ...op(17,1),...op(14,0,1),...op(15,4,17,0x6e69616d,0),...op(16,17,7),
 ...op(71,16,34,0),...op(71,16,33,43),...op(71,7,3),...op(72,7,0,35,offset),...op(71,6,6,stride),...(readOnly?op(71,16,24):[]),
 ...op(19,1),...op(20,5),...op(21,3,32,0),...op(22,4,32),...op(29,6,3),...op(30,7,6),...op(32,8,2,7),...op(32,9,2,3),...op(32,10,7,5),...op(33,11,1),
 ...op(43,3,12,0),...op(43,3,13,1),...op(43,4,14,0),...op(42,5,15),...op(59,8,16,2),
 ...op(54,1,17,0,11),...op(248,18),...op(59,10,19,7,15),...(copy?op(63,19,19):[]),
 ...op(65,9,20,16,12,13),...op(61,3,21,20),...op(124,4,22,21),...op(183,5,23,14,22),...op(62,19,23),...op(61,5,24,19),...op(247,26,0),...op(250,24,25,26),...op(248,25),...op(62,19,15),...op(249,26),...op(248,26),...op(253),...op(56)];
 return new Uint8Array(new Uint32Array(w).buffer);
}
let checks=0;
for(const bits of [0,0x80000000,0x3f800000,0xbf800000,0x7fc00000,0x7f800000]){
 const input=fixture(),r=specializeUniformBranches(input,{'0/43':[0,bits]});
 assert.equal(r.branches,[0,0x80000000].includes(bits)?1:0);checks++;
 if(r.branches){assert.deepEqual(r.reads,['0/43/1']);assert.ok(r.bytes.length<input.length);checks+=2;}
 else{assert.deepEqual(r.bytes,input);checks++;}
}
for(const options of [{readOnly:false},{stride:8},{offset:4},{copy:true}]){
 const input=fixture(options),r=specializeUniformBranches(input,{'0/43':[0,0]});assert.equal(r.branches,0);assert.deepEqual(r.bytes,input);checks+=2;
}
for(const words of [{},{'0/43':[]},{'0/43':[0]},{'0/44':[0,0]}]){assert.equal(specializeUniformBranches(fixture(),words).branches,0);checks++;}
console.log(`Uniform branch proof: PASS (${checks} checks; signed zero, NaN, unknown data, readonly layout and memory invalidation)`);
