import assert from 'node:assert/strict';
import {writeFile} from 'node:fs/promises';
const asyncify=process.argv.includes('--asyncify');
const flavor=asyncify?'game32-asyncify-opt1':'game32-compact';
const {default:create}=await import(`../build/web-poc/${flavor}.mjs`);
let finish,fail;
const done=new Promise((resolve,reject)=>{finish=resolve;fail=reject;});
const failures=[];
const timer=setTimeout(()=>fail(new Error('Compact guest fixture timeout')),20000);
try{
  await create({print(line){
    if(line.startsWith('SSBU_')){
      const at=line.indexOf(' '),kind=line.slice(0,at),value=JSON.parse(line.slice(at+1));
      if(kind==='SSBU_CHECK'&&!value.ok)failures.push(value);
      if(kind.endsWith('_RESULT'))finish(value);
    }
  },onAbort:reason=>fail(new Error(String(reason)))});
  const result=await done;
  assert.equal(result.failures,0);assert.equal(result.checks,384);
  assert.equal(result.functions,96);assert.equal(result.pointerBits,32);
  const report={...result,failuresDetail:failures,asyncify,
    scope:'96 actual lifted game leaves versus 384 existing AArch64 oracle states; O1 code generation without LLVM IR passes; not full game execution'};
  await writeFile(new URL(`../build/${flavor}-report.json`,import.meta.url),JSON.stringify(report,null,2));
  console.log(JSON.stringify(report));
}finally{clearTimeout(timer);}
