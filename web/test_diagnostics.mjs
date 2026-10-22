import assert from 'node:assert/strict';
import {writeFile} from 'node:fs/promises';
import create from '../build/web-diagnostics/probe.mjs';
const module=await create();
try{
  assert.equal(module._diagnostic_check(),0);const checks=module._diagnostic_count();assert.ok(checks>60000);
  assert.equal(module._diagnostic_memory(),0);
  const report={ok:true,formatComparisons:checks,mappedReadWrite:true,unmappedReadRecovered:true,readonlyWriteRecovered:true,
    diagnosticTruncationAndFramePreservation:true,
    scope:'Production trap, TLS frame and memory functions; byte/canary comparison against snprintf, no game boot or fiber migration'};
  await writeFile(new URL('../build/web-diagnostics/report.json',import.meta.url),JSON.stringify(report,null,2));console.log(JSON.stringify(report));
}finally{module.PThread.terminateAllThreads();}
