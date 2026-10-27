import assert from 'node:assert/strict';
import {writeFile} from 'node:fs/promises';
import create from '../build/web-os/probe.mjs';
import {writePadState} from './game-input.mjs';
let complete;const completed=new Promise(resolve=>{complete=resolve;});
const module=await create({onOsProbeReport:text=>complete(JSON.parse(text)),onInputFixture:(offset,mode)=>{
  const words=new Int32Array(module.HEAPU8.buffer,offset,7);
  if(mode===2)Atomics.or(words,0,1);
  else writePadState(words,mode?[0x80501,0,-40000,40000,-12345,23456]:[0,0,0,0,0,0]);
}});
let timer;
try {
  assert.equal(module._os_probe_start(),0);
  const result=await Promise.race([completed,new Promise((_,reject)=>{timer=setTimeout(()=>reject(new Error(`OS fixture timed out at call ${module._os_probe_stage()}`)),20000);})]);
  assert.equal(module._os_probe_join(),0);
  result.scope='Self-authored OS calls through host_os.c: actual Wasm pthread event/thread waiters, 601 guest thread callbacks, TLS/address/registry reuse and JS shared input -> Npad; not a game/menu/input-response test';
  await writeFile(new URL('../build/web-os/report.json',import.meta.url),JSON.stringify(result,null,2));
  assert.equal(result.ok,true,JSON.stringify(result));console.log(JSON.stringify(result));
}finally{clearTimeout(timer);module.PThread.terminateAllThreads();}
