import assert from 'node:assert/strict';
import {writeFile} from 'node:fs/promises';
const baseline=process.argv.includes('--baseline');
const {default:create}=await import(`../build/web-fiber-migration/${baseline?'baseline':'probe'}.mjs`);
let complete;const completed=new Promise(resolve=>{complete=resolve;});
const module=await create({onMigrationReport:text=>complete(JSON.parse(text))});let timer;
try {
  assert.equal(module._migration_start(),0);
  const result=await Promise.race([completed,new Promise((_,reject)=>{timer=setTimeout(()=>reject(new Error('Migration fixture timed out')),15000);})]);
  assert.equal(module._migration_join(),0);assert.equal(result.ok,true,JSON.stringify(result));assert.equal(result.stage,6);
  result.scope='Synthetic A -> B -> A stackful migration across actual Wasm pthreads, nested locals and setjmp/longjmp; not the game scheduler';
  await writeFile(new URL(`../build/web-fiber-migration/${baseline?'baseline-report':'report'}.json`,import.meta.url),JSON.stringify(result,null,2));
  console.log(JSON.stringify(result));
}finally{clearTimeout(timer);module.PThread.terminateAllThreads();}
