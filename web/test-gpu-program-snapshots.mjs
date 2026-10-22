import {WebGpuBackend} from './gpu-backend.mjs';
import {stageInterface} from './wgsl-interface.mjs';

/** Actual captured sources survive replacement/finalization, then compile to
 * accepted WebGPU modules and a compute pipeline. No dispatch or draw occurs. */
export async function testProgramSnapshots(device) {
  const response=await fetch('./maxwell-fixtures.json');
  if(!response.ok)throw new Error(`Maxwell fixture HTTP ${response.status}`);
  const {fixtures}=await response.json();
  async function source(name){
    const fixture=fixtures.find(item=>item.name===name);
    if(!fixture)throw new Error(`Missing captured snapshot fixture ${name}`);
    const bytes=Uint8Array.from(atob(fixture.bytes),character=>character.charCodeAt(0));
    const digest=Array.from(new Uint8Array(await crypto.subtle.digest('SHA-256',bytes)),b=>b.toString(16).padStart(2,'0')).join('');
    if(digest!==fixture.sha256)throw new Error('Captured snapshot source hash differs');
    const v=new DataView(bytes.buffer),control=v.getUint32(12,true),data=v.getUint32(16,true);
    if(v.getUint32(0,true)!==0x3144584d||20+control+data!==bytes.length)throw new Error('Malformed captured snapshot source');
    const packet=new Uint8Array(12+control+data),header=new DataView(packet.buffer);
    header.setUint32(0,1,true);header.setUint32(4,control,true);header.setUint32(8,data,true);packet.set(bytes.subarray(20),12);
    return {packet,name,sha256:digest};
  }
  const compute=await source('s00016_0.mxd'),vertex=await source('s00013_0.mxd');
  const backend=new WebGpuBackend(device),options={topology:'triangles'};
  let checks=0;
  const check=(value,message)=>{checks++;if(!value)throw new Error(message);};
  try{
    const original=(await backend.request(12,{bytes:compute.packet})).value;
    const replaced=(await backend.request(12,{bytes:compute.packet})).value;
    const other=(await backend.request(12,{bytes:vertex.packet})).value;
    const [expectedCompute]=await backend.compileProgram(original,options);
    const [expectedVertex]=await backend.compileProgram(other,options);
    check(expectedCompute.metadata.stage===5&&expectedVertex.metadata.stage===0,'Expected distinct compute/vertex captures');
    const retained=(await backend.request(20,{handle:replaced})).value;
    const shared=(await backend.request(20,{handle:replaced})).value;
    await backend.request(12,{handle:replaced,bytes:vertex.packet});
    const changed=(await backend.request(20,{handle:replaced})).value;
    compute.packet.fill(0);vertex.packet.fill(0);
    await backend.request(13,{handle:replaced});
    const [oldStage]=await backend.compileProgramSnapshot(retained,options);
    const [newStage]=await backend.compileProgramSnapshot(changed,options);
    check(oldStage.code===expectedCompute.code,'Retained compute source changed after replacement/finalization');
    check(newStage.code===expectedVertex.code,'Retained vertex source changed after finalization');
    check(oldStage.module!==expectedCompute.module&&newStage.module!==expectedVertex.module,'Expected fresh compilation from retained sources');
    check((await backend.compileProgramSnapshot(shared,options))[0]===oldStage,'Retained aliases did not share their immutable variant');
    await backend.request(21,{handle:retained});
    let rejected=false;try{await backend.compileProgramSnapshot(retained,options);}catch(error){rejected=/stale/.test(String(error));}
    check(rejected,'Released snapshot was accepted');
    check((await backend.compileProgramSnapshot(shared,options))[0]===oldStage,'Releasing one token invalidated another token');
    const {entryPoint}=stageInterface(oldStage.code,'compute');
    await backend.checked(()=>device.createComputePipelineAsync({layout:'auto',compute:{module:oldStage.module,entryPoint}}));
    checks++;
    await backend.request(21,{handle:shared});await backend.request(21,{handle:changed});
    check(backend.statistics.liveProgramSnapshots===0,'Retained snapshots leaked');
    check(backend.statistics.shadersCompiled===4,'Unexpected source/variant compilation count');
    return {ok:true,checks,sources:[{name:compute.name,sha256:compute.sha256},{name:vertex.name,sha256:vertex.sha256}],
      retainedComputePipeline:true,entryPoint,statistics:{...backend.statistics},
      scope:'Actual captured compute and vertex programs retained across replacement/finalization, immutable variant checks and actual WebGPU pipeline validation; no dispatch, draw or native pixel comparison'};
  }finally{backend.close();}
}
