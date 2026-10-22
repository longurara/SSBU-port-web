import assert from 'node:assert/strict';
import {writeFile} from 'node:fs/promises';
const folder=process.argv.includes('--unaligned-control')?'web-host-fiber-unaligned':'web-host-fiber';
const {default:create}=await import(`../build/${folder}/probe.mjs`);
let complete;
const module=await create({onHostFiberReport:text=>complete(JSON.parse(text))});let timer;
try{
  const rounds=[];
  for(let round=0;round<3;round++){
  const completed=new Promise(resolve=>{complete=resolve;});
  assert.equal(module._host_fiber_start(),0);
  const result=await Promise.race([completed,new Promise((_,reject)=>{timer=setTimeout(()=>reject(new Error('Host fiber timeout')),15000);})]);
  assert.equal(module._host_fiber_join(),0);assert.equal(result.ok,true,JSON.stringify(result));assert.equal(result.stage,6);
  clearTimeout(timer);rounds.push(result);
  await new Promise(resolve=>setTimeout(resolve,10));
  }
  for(const result of rounds)assert.deepEqual(result.workerIds.toSorted(),rounds[0].workerIds.toSorted());
  const report={ok:true,checks:rounds.reduce((sum,result)=>sum+result.checks,0),rounds,scope:'Three actual pthread lifetimes on the same two Workers, guest FiberType imports with A -> B -> A, virtual registers/TLS and diagnostic frame; not game execution'};
  await writeFile(new URL(`../build/${folder}/report.json`,import.meta.url),JSON.stringify(report,null,2));console.log(JSON.stringify(report));
}finally{clearTimeout(timer);module.PThread.terminateAllThreads();}
