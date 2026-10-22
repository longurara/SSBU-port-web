import assert from 'node:assert/strict';
import {writeFile} from 'node:fs/promises';
import create from '../build/web-mutex/probe.mjs';
let finish,fail;
const done=new Promise((resolve,reject)=>{finish=resolve;fail=reject;});
const module=await create({onMutexReport:text=>finish(JSON.parse(text)),onAbort:reason=>fail(new Error(String(reason)))});
const timer=setTimeout(()=>fail(new Error('Mutex fixture timed out')),20000);
try{
  assert.equal(module._mutex_probe_start(),0);
  const result=await done;assert.equal(module._mutex_probe_join(),0);
  assert.equal(result.ok,true,JSON.stringify(result));assert.ok(result.checks>40000);
  assert.ok(result.peakMutexes>=1536);assert.equal(result.liveMutexes,0);assert.equal(result.counter,20001);
  const report={...result,scope:'Real pthread exclusion, recursive/error-check modes, wrong-owner rejection, busy destroy, SDK imports and 1536 live guest mutex keys; not game execution'};
  await writeFile(new URL('../build/web-mutex/report.json',import.meta.url),JSON.stringify(report,null,2));console.log(JSON.stringify(report));
}finally{clearTimeout(timer);module.PThread.terminateAllThreads();}
