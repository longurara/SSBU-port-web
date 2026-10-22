import assert from 'node:assert/strict';
import { readFile, writeFile } from 'node:fs/promises';
import {fileURLToPath} from 'node:url';
const {default: create} = await import(`../build/web-poc/${process.env.SSBU_WEB_RUNTIME || 'runtime'}.mjs`);

let complete;
const completed = new Promise(resolve => { complete = resolve; });
const module = await create({onRuntimeReport:text => complete(JSON.parse(text))});
const primeReference = JSON.parse(await readFile(new URL('../build/web-sdk/prime-reference.json', import.meta.url), 'utf8'));
for (const test of primeReference) assert.equal(module._web_next_prime(BigInt(test.input)), BigInt(test.output));
const path = process.argv.slice(2).find(argument => !argument.startsWith('--')) || new URL('../build/native/binaries/13.0.4/main.elf', import.meta.url);
module.FS.writeFile('/main.elf', await readFile(path));
module.FS.mkdir('/romfs');
module.FS.mount(module.NODEFS,{root:fileURLToPath(new URL('../build/dump-13.0.4/romfs',import.meta.url))},'/romfs');
assert.equal(module.ccall('elf_load', 'number', ['string'], ['/main.elf']), 1);
assert.equal(module._elf_relocate(), 1);
assert.equal(module._runtime_start(process.argv.includes('--main') ? 1 : 0), 0);
let timer;
let result;
try {
  result = await Promise.race([completed,new Promise((_,reject) => {timer=setTimeout(() => reject(new Error('Runtime timeout')),120000);})]);
} finally { clearTimeout(timer); if (result && result.phase !== 'guestThread') module._runtime_join(); module.PThread.terminateAllThreads(); }
result.sdkPrimeReferenceCases = primeReference.length;
await writeFile(new URL('../build/web-poc/runtime-node-report.json', import.meta.url), JSON.stringify(result, null, 2));
console.log(JSON.stringify(result, null, 2));
process.exitCode = result.ok ? 0 : 2;
