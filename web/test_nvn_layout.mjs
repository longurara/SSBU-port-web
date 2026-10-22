import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import create from '../build/web-nvn-layout/probe.mjs';
const module=await create();
const cases=JSON.parse(await readFile(new URL('../build/web-nvn-layout/reference.json',import.meta.url),'utf8'));
const input=module._malloc(48),output=module._malloc(64);
try {
  for(const test of cases) {
    let view=new DataView(module.HEAPU8.buffer);
    test.fields.forEach((value,index)=>view.setUint32(input+index*4,value,true));
    assert.equal(module._layout_probe(input,output),1,module.UTF8ToString(module._layout_probe_error()));
    view=new DataView(module.HEAPU8.buffer);
    assert.deepEqual(Array.from({length:8},(_,index)=>view.getBigUint64(output+index*8,true)),test.expected.map(BigInt),JSON.stringify(test.fields));
  }
  console.log(`${cases.length} texture-layout configurations / ${cases.length*5} SDK queries match AArch64 execution`);
} finally {module._free(input);module._free(output);module.PThread.terminateAllThreads();}
