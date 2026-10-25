import assert from 'node:assert/strict';
import {writeFile} from 'node:fs/promises';
import create from '../build/web-movie-values/probe.mjs';
const module=await create();
try{
  const result=JSON.parse(module.ccall('movie_values_probe','string',[],[]));
  result.scope='Production movie Buffer and MediaData imports, raw SDK offsets, guest allocator callbacks, numeric types, guest-owned strings, deep copies, callback reentry, register preservation and two concurrent Wasm pthreads. Not a decoder/game test.';
  await writeFile(new URL('../build/web-movie-values/report.json',import.meta.url),JSON.stringify(result,null,2)+'\n');
  assert.equal(result.ok,true,JSON.stringify(result));assert.ok(result.checks>500);
  console.log(`PASS: ${result.checks} production movie value ABI/ownership checks`);
}finally{module.PThread?.terminateAllThreads();}
