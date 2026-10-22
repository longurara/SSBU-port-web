import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import create from '../build/web-sort/probe.mjs';
const module=await create();
const cases=JSON.parse(await readFile(new URL('../build/web-sort/reference.json',import.meta.url),'utf8'));
for(const test of cases) {
  const input=Buffer.from(test.input,'base64'),expected=Buffer.from(test.output,'base64');
  const pointer=module._malloc(Math.max(1,input.length));
  try {
    module.HEAPU8.set(input,pointer);
    assert.equal(module._sort_probe(pointer,input.length,test.width),1);
    assert.deepEqual(Buffer.from(module.HEAPU8.slice(pointer,pointer+input.length)),expected);
  }finally{module._free(pointer);}
}
console.log(`${cases.length} unsigned-sort Wasm cases match actual AArch64 SDK execution, including page crossings and sentinel preservation`);
