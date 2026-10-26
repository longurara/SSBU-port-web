import assert from 'node:assert/strict';
import {lowerSpirv} from './spirv-compat.mjs';

function binary(ops,bound=14,version=0x10000) {
  const words=[0x07230203,version,0,bound,0];
  for(const [op,a] of ops)words.push(((a.length+1)<<16)|op,...a);
  return new Uint8Array(new Uint32Array(words).buffer);
}
function instructions(bytes) {
  const words=new Uint32Array(bytes.buffer,bytes.byteOffset,bytes.byteLength/4),ops=[];
  for(let i=5;i<words.length;i+=words[i]>>>16)ops.push([words[i]&65535,Array.from(words.subarray(i+1,i+(words[i]>>>16)))]);
  return ops;
}
function fixture(width=1,floatBits=32) {
  const ops=[[19,[1]],[20,[2]],[22,[3,floatBits]],[33,[5,1]]];let bool=2,float=3;
  if(width>1){ops.push([23,[7,2,width]],[23,[8,3,width]]);bool=7;float=8;}
  ops.push([45,[float,6]],[54,[1,10,0,5]],[248,[11]],[156,[bool,12,6]],[157,[bool,13,6]],[253,[]],[56,[]]);
  return binary(ops);
}
const special=[0,0x80000000,1,0x7f7fffff,0x7f800000,0xff800000,0x7f800001,0x7fc00000,0xffc00001];
let seed=123;
for(let i=0;i<1000;i++){seed=(Math.imul(seed,1664525)+1013904223)>>>0;special.push(seed);}
for(const width of [1,2,3,4]) {
  const {bytes,layout}=lowerSpirv(fixture(width)),ops=instructions(bytes);
  assert.equal(layout.classifications,2);
  for(let index=0;index<special.length;index+=width) {
    const lanes=Array.from({length:width},(_,i)=>special[(index+i)%special.length]),env=new Map([[6,width===1?lanes[0]:lanes]]);
    for(const [op,a] of ops) {
      if(op===43)env.set(a[1],a[2]);
      if(op===44)env.set(a[1],a.slice(2).map(id=>env.get(id)));
      if(op===124)env.set(a[1],env.get(a[2]));
      if([170,172,199].includes(op)) {
        const x=env.get(a[2]),y=env.get(a[3]),apply=(x,y)=>op===199?x&y:op===170?x===y:x>y;
        env.set(a[1],Array.isArray(x)?x.map((v,i)=>apply(v,y[i])):apply(x,y));
      }
    }
    const values=new Float32Array(new Uint32Array(lanes).buffer);
    const nan=Array.from(values,Number.isNaN),inf=Array.from(values,x=>!Number.isNaN(x)&&!Number.isFinite(x));
    assert.deepEqual(env.get(12),width===1?nan[0]:nan);assert.deepEqual(env.get(13),width===1?inf[0]:inf);
  }
}
for(const size of [16,64])assert.throws(()=>lowerSpirv(fixture(1,size)),/f32/);
for(const data of [new Uint8Array(0),new Uint8Array(24),binary([[0,[]]]).subarray(0,23)])assert.throws(()=>lowerSpirv(data));

// A direct combined image/sampler has two distinct bindings after lowering.
const sampled=binary([[17,[1]],[14,[0,1]],[15,[4,8,0x6e69616d,0,5]],
  [71,[5,34,1]],[71,[5,33,7]],[22,[1,32]],[25,[2,1,1,0,0,0,1,0]],[27,[3,2]],
  [32,[4,0,3]],[19,[6]],[33,[7,6]],[59,[4,5,0]],[54,[6,8,0,7]],[248,[9]],
  [61,[3,10,5]],[100,[2,11,10]],[253,[]],[56,[]]],12,0x10400);
const split=lowerSpirv(sampled),ops=instructions(split.bytes);
assert.deepEqual(split.layout.samplers,[{group:1,imageBinding:7,samplerBinding:263,sourceVariable:5}]);
assert.equal(ops.filter(([op])=>op===86).length,1);
assert.equal(ops.find(([op,a])=>op===32&&a[0]===4)[1][2],2);
const samplerVariable=ops.find(([op,a])=>op===71&&a[1]===33&&a[2]===263)[1][0];
assert.ok(ops.find(([op])=>op===15)[1].includes(samplerVariable));
function atBinding(binding){
  return binary(instructions(sampled).map(([op,a])=>[op,op===71&&a[0]===5&&a[1]===33?[5,33,binding]:a]),12,0x10400);
}
const vertexSampler=lowerSpirv(atBinding(0)).layout.samplers[0],fragmentSampler=lowerSpirv(atBinding(40)).layout.samplers[0];
assert.deepEqual([vertexSampler.imageBinding,vertexSampler.samplerBinding,fragmentSampler.imageBinding,fragmentSampler.samplerBinding],[0,256,40,296]);
assert.equal(new Set([vertexSampler.imageBinding,vertexSampler.samplerBinding,fragmentSampler.imageBinding,fragmentSampler.samplerBinding]).size,4);
assert.throws(()=>lowerSpirv(atBinding(256)),/namespace/);
const conflictOps=instructions(sampled);
conflictOps.splice(conflictOps.findIndex(([op])=>op===22),0,[71,[12,34,1]],[71,[12,33,263]]);
conflictOps.splice(conflictOps.findIndex(([op])=>op===54),0,[59,[4,12,0]]);
const conflict=binary(conflictOps,13,0x10400);
assert.throws(()=>lowerSpirv(conflict),/conflicts/);

const point=binary([[71,[5,11,1]],[22,[1,32]],[32,[4,3,1]],[59,[4,5,3]],
  [19,[6]],[33,[7,6]],[54,[6,8,0,7]],[248,[9]],[253,[]],[56,[]]],10);
assert.throws(()=>lowerSpirv(point),/triangle topology/);
assert.deepEqual(lowerSpirv(point,{topology:'triangles'}).layout.pointSizeLocations,[0]);

for(const width of [1,2,3,4]) {
  const integer=width===1?2:5;
  const source=[[19,[1]],[21,[2,32,0]],[33,[4,1]]];
  if(width>1)source.push([23,[5,2,width]]);
  source.push([30,[6,integer,integer]],[45,[integer,7]],[45,[integer,8]],
    [54,[1,10,0,4]],[248,[11]],[149,[6,12,7,8]],[150,[6,13,7,8]],[253,[]],[56,[]]);
  const lowered=lowerSpirv(binary(source)),ops=instructions(lowered.bytes);
  assert.equal(lowered.layout.integerOperations,2);
  const inputs=[0,1,0x7fffffff,0x80000000,0xffffffff,...special.slice(0,150)];
  for(let k=0;k<inputs.length;k++) {
    const left=Array.from({length:width},(_,lane)=>inputs[(k+lane)%inputs.length]);
    const right=Array.from({length:width},(_,lane)=>inputs[(inputs.length-1-k+lane)%inputs.length]);
    const env=new Map([[7,width===1?left[0]:left],[8,width===1?right[0]:right]]);
    for(const [op,a] of ops) {
      if(op===43)env.set(a[1],a[2]);
      if(op===44)env.set(a[1],a.slice(2).map(id=>env.get(id)));
      if([128,130,176].includes(op)) {
        const x=env.get(a[2]),y=env.get(a[3]),apply=(x,y)=>op===128?(x+y)>>>0:op===130?(x-y)>>>0:x<y;
        env.set(a[1],Array.isArray(x)?x.map((v,i)=>apply(v,y[i])):apply(x,y));
      }
      if(op===169) {
        const condition=env.get(a[2]),yes=env.get(a[3]),no=env.get(a[4]);
        env.set(a[1],Array.isArray(condition)?condition.map((v,i)=>v?yes[i]:no[i]):condition?yes:no);
      }
      if(op===80)env.set(a[1],a.slice(2).map(id=>env.get(id)));
    }
    const sums=left.map((v,i)=>BigInt(v)+BigInt(right[i])),differences=left.map((v,i)=>BigInt(v)-BigInt(right[i]));
    const pack=values=>width===1?values[0]:values;
    assert.deepEqual(env.get(12),[pack(sums.map(v=>Number(BigInt.asUintN(32,v)))),pack(sums.map(v=>Number(v>>32n)))]);
    assert.deepEqual(env.get(13),[pack(differences.map(v=>Number(BigInt.asUintN(32,v)))),pack(differences.map(v=>v<0n?1:0))]);
  }
}
for(const builtin of [4424,4425,4426]) {
  const source=binary([[17,[4427]],[15,[0,8,0x6e69616d,0,5]],[71,[5,11,builtin]],
    [19,[1]],[21,[2,32,1]],[21,[3,32,0]],[32,[4,1,2]],[59,[4,5,1]],[33,[6,1]],
    [54,[1,8,0,6]],[248,[9]],[61,[2,10,5]],[253,[]],[56,[]]],11);
  const {bytes,layout}=lowerSpirv(source),ops=instructions(bytes);
  assert.deepEqual(layout.drawParameters,{group:3,binding:1,bytes:16,fields:['baseVertex','baseInstance','drawIndex']});
  assert.ok(!ops.some(([op,a])=>op===17&&a[0]===4427));
  assert.ok(!ops.find(([op])=>op===15)[1].slice(4).includes(5));
  const chain=ops.find(([op])=>op===65)[1];
  const member=ops.find(([op,a])=>op===43&&a[1]===chain[3])[1][2];
  assert.equal(member,builtin-4424); // No replacement with a constant zero.
  assert.ok(ops.some(([op,a])=>op===124&&a[1]===10)); // Original signed ABI.
}
for(const mask of [24,32,64])assert.throws(()=>lowerSpirv(binary([[96,[1,2,3,4,5,mask,6]]],7)),/explicit lowering/);
function viewportFixture({branchWrite=false,alias=false,constant=false}={}){
  return binary([[17,[1]],[17,[5254]],[17,[4427]],[14,[0,1]],[15,[0,20,0x6e69616d,0,11,12,13]],
    [71,[11,11,43]],[71,[12,11,4425]],[71,[13,11,10]],
    [19,[1]],[21,[2,32,1]],[21,[3,32,0]],[32,[4,1,2]],[32,[5,3,2]],[32,[6,7,3]],[33,[7,1]],[20,[26]],
    [43,[2,8,0]],[43,[3,9,0]],[43,[3,10,1]],[41,[26,27]],
    [59,[4,11,1]],[59,[4,12,1]],[59,[5,13,3]],
    [54,[1,20,0,7]],[248,[21]],[59,[6,14,7,9]],
    [61,[2,15,11]],[61,[2,16,12]],[130,[2,17,15,16]],[124,[3,18,17]],[62,[14,18]],
    ...(alias?[[83,[6,28,14]],[62,[28,10]]]:[]),
    [247,[24,0]],[250,[27,22,23]],[248,[22]],...(branchWrite?[[62,[14,10]]]:[]),[249,[24]],[248,[23]],[249,[24]],[248,[24]],
    [61,[3,19,14]],[124,[2,25,19]],[62,[13,constant?8:25]],[253,[]],[56,[]]],29);
}
const provenViewport=lowerSpirv(viewportFixture(),{singleInstanceBase:7});
assert.deepEqual(provenViewport.layout.singleViewport,{outputs:[10],instanceBase:7});
assert.ok(!instructions(provenViewport.bytes).some(([op,a])=>op===17&&a[0]===5254||op===59&&a[1]===13||op===62&&a[0]===13));
assert.throws(()=>lowerSpirv(viewportFixture()),/unproven viewport/);
assert.throws(()=>lowerSpirv(viewportFixture({branchWrite:true}),{singleInstanceBase:7}),/unproven viewport/);
assert.throws(()=>lowerSpirv(viewportFixture({alias:true}),{singleInstanceBase:7}),/unproven viewport/);
assert.deepEqual(lowerSpirv(viewportFixture({constant:true})).layout.singleViewport.outputs,[10]);
console.log('SPIR-V compatibility: PASS (IEEE bits, carry/borrow against BigInt, sampler interfaces, draw parameter mapping, gather operand guards and input guards)');

// Component maps are applied after sample/fetch, while gather remaps its
// channel selector. Unknown/invalid cases must remain explicit errors.
const swizzleOps=instructions(sampled);
const fnAt=swizzleOps.findIndex(([op])=>op===54);
swizzleOps.splice(fnAt,0,[23,[12,1,4]],[23,[13,1,2]],[43,[1,14,0]],[44,[13,15,14,14]]);
const retAt=swizzleOps.findIndex(([op])=>op===253);
swizzleOps.splice(retAt,0,[88,[12,16,10,15,2,14]]);
const swizzleSample=binary(swizzleOps,17,0x10400);
const mapped=lowerSpirv(swizzleSample,{textureSwizzles:{'1/7':[1,1,1,2]}}),mappedOps=instructions(mapped.bytes);
assert.equal(mapped.layout.textureSwizzles,1);
const result=mappedOps.find(([op,a])=>op===80&&a[1]===16)[1];assert.equal(result[2],result[3]);assert.equal(result[3],result[4]);
assert.ok(mappedOps.some(([op,a])=>op===43&&a[1]===result[2]&&a[2]===0x3f800000));
assert.ok(mappedOps.some(([op,a])=>op===81&&a[1]===result[5]&&a[3]===0));
for(const mapping of [{'1/7':[1,1,1,6]},{'1/7':[1,1,1]},{'1/7':[1,1,1,1.5]},{bad:[1,1,1,2]}])assert.throws(()=>lowerSpirv(swizzleSample,{textureSwizzles:mapping}),/Invalid texture swizzle/);
const identity=lowerSpirv(swizzleSample,{textureSwizzles:{'1/7':[2,3,4,5]}});assert.equal(identity.layout.textureSwizzles,undefined);
const drefOps=swizzleOps.map(([op,a])=>op===88?[90,[1,16,10,15,14,2,14]]:[op,a]);
assert.throws(()=>lowerSpirv(binary(drefOps,17,0x10400),{textureSwizzles:{'1/7':[1,1,1,2]}}),/Comparison\/sparse/);
const unknownOps=swizzleOps.map(([op,a])=>op===88?[88,[...a.slice(0,2),999,...a.slice(3)]]:[op,a]);
assert.throws(()=>lowerSpirv(binary(unknownOps,1000,0x10400),{textureSwizzles:{'1/7':[1,1,1,2]}}),/Unresolved image source/);
console.log('Texture swizzle contracts: PASS (typed constants, channel extraction, identity, invalid maps, comparison and unresolved sources)');

// Queries return the full vec2, including when glslang stores it in a local.
const queryOps=instructions(sampled).map(([op,a])=>[op,[...a]]);
queryOps.splice(queryOps.findIndex(([op])=>op===54),0,[23,[12,1,2]],[43,[1,13,0x3f000000]],[44,[12,14,13,13]]);
queryOps.splice(queryOps.findIndex(([op])=>op===253),0,[105,[12,15,10,14]]);
const query=binary(queryOps,16,0x10400),sampler=new Uint8Array(48),sv=new DataView(sampler.buffer);
sampler[0]=5;sampler[1]=1;sv.setFloat32(8,-1000,true);sv.setFloat32(12,1000,true);sv.setFloat32(20,1,true);
const queryOptions={lodQuerySamplers:{'1/7':Array.from(sampler)}};
assert.equal(lowerSpirv(query,queryOptions).layout.lodQueries,1);
assert.ok(!instructions(lowerSpirv(query,queryOptions).bytes).some(([op])=>op===105));
assert.throws(()=>lowerSpirv(query),/actual 48-byte/);
for(const [offset,value]of [[16,1],[20,2],[44,1]]){
  const changed=sampler.slice();new DataView(changed.buffer).setFloat32(offset,value,true);
  assert.throws(()=>lowerSpirv(query,{lodQuerySamplers:{'1/7':Array.from(changed)}}),/separate lowering/);
}
for(const [offset,value]of [[0,6],[7,1]]){
  const changed=Array.from(sampler);changed[offset]=value;
  assert.throws(()=>lowerSpirv(query,{lodQuerySamplers:{'1/7':changed}}),/Invalid ImageQueryLod|separate lowering/);
}
for(const mutate of [
  (op,a)=>op===15?[0,...a.slice(1)]:a,
  (op,a)=>op===25?[...a.slice(0,2),3,...a.slice(3)]:a,
  (op,a)=>op===25?[...a.slice(0,4),1,...a.slice(5)]:a,
  (op,a)=>op===25?[...a.slice(0,5),1,...a.slice(6)]:a,
])assert.throws(()=>lowerSpirv(binary(queryOps.map(([op,a])=>[op,mutate(op,a)]),16,0x10400),queryOptions),/fragment 2D/);
console.log('LOD query contracts: PASS (full vector, actual sampler required, unsupported stage/dimensions/sampling state rejected)');
