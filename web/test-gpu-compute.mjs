import {WebGpuBackend} from './gpu-backend.mjs';

/** Self-authored test shader, injected only in fixture compiler instances. */
export function fixtureComputeShader(){return {
  code:`struct Params{count:u32,bias:u32,pad:vec2<u32>};
    @group(0) @binding(3) var<storage,read> params:Params;
    @group(0) @binding(20) var<storage,read_write> src:array<u32>;
    @group(0) @binding(21) var<storage,read_write> dst:array<u32>;
    @group(0) @binding(38) var<storage,read_write> written:atomic<u32>;
    @compute @workgroup_size(4) fn main(@builtin(global_invocation_id) id:vec3<u32>){
      if(id.x<params.count){dst[id.x]=src[id.x]*3u+params.bias;atomicOr(&written,2u);}}`,
  activeBindings:[[0,3],[0,20],[0,21],[0,38]],metadata:{stage:5,cbufs:8,ssbos:3,ssbosWritten:3,textures:0,images:0},layout:{samplers:[],pushConstant:null,drawParameters:null}};}

export async function testGpuCompute(device){
  const backend=new WebGpuBackend(device);backend.programs.compilerFactory=async()=>({compile:()=>fixtureComputeShader()});
  const packet=new Uint8Array(12+0x75c+32),pv=new DataView(packet.buffer);pv.setUint32(0,1,true);pv.setUint32(4,0x75c,true);pv.setUint32(8,32,true);pv.setUint32(12,0x98761234,true);pv.setUint32(12+0x6f8,32,true);pv.setUint32(12+0x714,5,true);
  const create=async words=>{const bytes=new Uint8Array(new Uint32Array(words).buffer),{value}=await backend.request(1,{size:bytes.length});await backend.request(2,{handle:value,offset:0,size:bytes.length,bytes});return value;};
  let checks=0;const check=(ok,message)=>{checks++;if(!ok)throw new Error(message);};
  try{
    const {value:program}=await backend.request(12,{bytes:packet}),{value:snapshot}=await backend.request(20,{handle:program});await backend.request(13,{handle:program});
    const source=await create([123,1,2,3,4,5,6,7,999,456]),target=await create([111,222,333,...Array(8).fill(0xdeadbeef),444]),next=await create([...Array(8).fill(0xdeadbeef)]),params=await create([99,99,7,5,0,0]);
    const p={version:1,program:snapshot,groups:[2,1,1],ubo:Array.from({length:15},()=>[0,0,0]),ssbo:Array.from({length:16},()=>[0,0,0]),textures:Array(32).fill(null)};
    p.ubo[0]=[params,8,16];p.ssbo[0]=[source,4,32];p.ssbo[1]=[target,12,32];
    const dispatch=()=>backend.request(26,{bytes:new TextEncoder().encode(JSON.stringify(p))});
    check((await dispatch()).value===2,'Actual write mask must exclude the read-only source');
    const read=async handle=>new Uint32Array((await backend.request(3,{handle,offset:0,size:backend.buffer(handle).size})).bytes.buffer);
    const first=await read(target);check(first[0]===111&&first[1]===222&&first[2]===333&&first[11]===444&&first[10]===0xdeadbeef,'Misaligned writable range changed guard bytes');
    for(let i=0;i<7;i++)check(first[i+3]===(i+1)*3+5,'Compute numeric output differs');
    p.ssbo[0]=[target,12,32];p.ssbo[1]=[next,0,32];check((await dispatch()).value===2,'Dependent dispatch mask differs');
    const second=await read(next);for(let i=0;i<7;i++)check(second[i]===((i+1)*3+5)*3+5,'Dependent dispatch lost prior output');
    const before=backend.statistics.computeDispatches;p.groups=[0,1,1];check((await dispatch()).value===0&&backend.statistics.computeDispatches===before,'Zero-work dispatch performed work');p.groups=[2,1,1];
    p.ssbo[1]=[target,16,32];let alias=false;try{await dispatch();}catch(error){alias=String(error).includes('Overlapping writable');}check(alias&&backend.statistics.computeDispatches===before,'Writable alias was accepted');
    p.ssbo[1]=[next,4,32];let bounds=false;try{await dispatch();}catch(error){bounds=String(error).includes('within the buffer');}check(bounds&&backend.statistics.computeDispatches===before,'Out-of-range storage was accepted');
    const unchanged=await read(next);check(unchanged.every((n,i)=>n===second[i]),'Rejected dispatch changed output');
    await backend.request(21,{handle:snapshot});let stale=false;try{await dispatch();}catch(error){stale=String(error).includes('stale');}check(stale,'Released compute snapshot remained usable');
    return {ok:true,checks,statistics:{...backend.statistics},scope:'Self-authored compute -> physical storage, actual write-mask, misaligned copyback, dependent dispatch and rejection guards; no game shader or guest writeback claim'};
  }finally{backend.close();}
}
