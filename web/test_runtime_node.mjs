/** Actual local constructor graph; no WebGPU device or nnMain execution. */
import assert from 'node:assert/strict';
import {readFile,writeFile} from 'node:fs/promises';
import {createHash} from 'node:crypto';
import {fileURLToPath} from 'node:url';
const build=process.argv[2]||'compact';
const modules={debug:'runtime-debug',compact:'runtime-debug-compact',fibers:'runtime-fibers','fibers-compact':'runtime-fibers-compact'};
if(!Object.hasOwn(modules,build))throw new Error('Unknown runtime variant');
const name=modules[build];
const {default:create}=await import(`../build/web-poc/${name}.mjs`);
let finish,fail;
const done=new Promise((resolve,reject)=>{finish=resolve;fail=reject;});
const module=await create({onRuntimeReport:text=>finish(JSON.parse(text))});
const timer=setTimeout(()=>fail(new Error('Constructor fixture timed out')),60000);
try{
  module.FS.mkdir('/program');
  module.FS.mount(module.NODEFS,{root:fileURLToPath(new URL('../build/native/binaries/13.0.4/',import.meta.url))},'/program');
  if(!module.ccall('elf_load','number',['string'],['/program/main.elf']))throw new Error(module.ccall('elf_info','string',[],[]));
  assert.equal(module._runtime_start(0),0);
  const result=await done;assert.equal(module._runtime_join(),0);
  assert.equal(result.ok,true,JSON.stringify(result));assert.equal(result.constructorsCompleted,306);assert.equal(result.constructorsTotal,306);assert.equal(result.nnMainRun,false);
  const bytes=await readFile(new URL(`../build/web-poc/${name}.wasm`,import.meta.url));
  const report={timestamp:new Date().toISOString(),runtimeBuild:build,wasmSha256:createHash('sha256').update(bytes).digest('hex'),
    results:{runtime:result},scope:'Actual 306 local game constructors in Node; no nnMain, GPU rendering, input or match verification'};
  await writeFile(new URL(`../build/web-poc/${name}-node-report.json`,import.meta.url),JSON.stringify(report,null,2));console.log(JSON.stringify(report));
}finally{clearTimeout(timer);module.PThread.terminateAllThreads();}
