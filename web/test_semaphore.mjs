import assert from 'node:assert/strict';
import {writeFile} from 'node:fs/promises';
import create from '../build/web-semaphore/probe.mjs';
let finish,fail;
const done=new Promise((resolve,reject)=>{finish=resolve;fail=reject;});
const module=await create({onSemaphoreReport:text=>finish(JSON.parse(text)),onAbort:reason=>fail(new Error(String(reason)))});
let timer=setTimeout(()=>fail(new Error('Semaphore fixture timed out')),15000);
try{
  assert.equal(module._semaphore_probe_start(),0);const result=await done;assert.equal(module._semaphore_probe_join(),0);
  assert.equal(result.ok,true,JSON.stringify(result));assert.ok(result.checks>=1400);assert.equal(result.acquired,2);assert.equal(result.live,0);
  const report={...result,scope:'Production semaphore adapter, real pthread permits/wakeups, bounds, zero/negative/positive deadlines, SDK names and 700 live objects; not game boot'};
  await writeFile(new URL('../build/web-semaphore/report.json',import.meta.url),JSON.stringify(report,null,2));console.log(JSON.stringify(report));
}finally{clearTimeout(timer);module.PThread.terminateAllThreads();}
