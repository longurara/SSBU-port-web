/* Exercise actual C loader bounds/lifetime checks and the native-dump bootstrap.
 * Node uses MEMFS here; the separate browser report covers WORKERFS File reads. */
import assert from 'node:assert/strict';
import { readFile, writeFile } from 'node:fs/promises';
import create from './assets.mjs';

const module = await create();
const call = (name, ...args) => module.ccall(name, 'number', ['string'], args);
const info = () => JSON.parse(module.ccall('elf_info', 'string', [], []));
function fixture() {
  const bytes = new Uint8Array(4100), v = new DataView(bytes.buffer);
  bytes.set([127,69,76,70,2,1,1]);
  v.setUint16(16, 3, true); v.setUint16(18, 183, true); v.setUint32(20, 1, true);
  v.setBigUint64(32, 64n, true); v.setUint16(52, 64, true);
  v.setUint16(54, 56, true); v.setUint16(56, 1, true);
  v.setUint32(64, 1, true); v.setUint32(68, 4, true);
  v.setBigUint64(72, 4096n, true); v.setBigUint64(96, 4n, true);
  v.setBigUint64(104, 8192n, true); v.setBigUint64(112, 4096n, true);
  bytes.set([1,2,3,4], 4096);
  return bytes;
}
let checks = 0;
const scenarios = [
  ['truncated header', b => b.subarray(0, 32)],
  ['wrong machine', b => { new DataView(b.buffer).setUint16(18, 62, true); return b; }],
  ['program headers overflow', b => { new DataView(b.buffer).setBigUint64(32, 0xfffffffffffffff0n, true); return b; }],
  ['segment exceeds file', b => { new DataView(b.buffer).setBigUint64(96, 5n, true); return b; }],
  ['file bigger than memory', b => { new DataView(b.buffer).setBigUint64(104, 3n, true); return b; }],
  ['unaligned guest segment', b => { new DataView(b.buffer).setBigUint64(80, 1n, true); return b; }],
  ['guest range overflow', b => { new DataView(b.buffer).setBigUint64(104, 0xffffffffffffffffn, true); return b; }],
  ['no LOAD segment', b => { new DataView(b.buffer).setUint32(64, 2, true); return b; }],
];
for (const [name, transform] of scenarios) {
  module.FS.writeFile('/test.elf', transform(fixture()));
  assert.equal(call('elf_load', '/test.elf'), 0, name);
  assert.equal(info().ok, false, name);
  // A rejected input must not poison later mapping, BSS zeroing or protection.
  module.FS.writeFile('/test.elf', fixture());
  assert.equal(call('elf_load', '/test.elf'), 1, `${name}: recovery`);
  const result = info();
  assert.equal(result.pages, 2); assert.equal(result.bssBytes, 8188);
  assert.equal(result.readonlyEnforced, true);
  module._elf_unload(); module._elf_unload();
  checks++;
}
const elf = await readFile(process.argv[2]);
module.FS.writeFile('/main.elf', elf);
assert.equal(call('elf_load', '/main.elf'), 1);
const image = info();
assert.equal(image.segments, 3); assert.equal(image.pages, 29767);
assert.equal(module._elf_relocate(), 1);
const relocations = JSON.parse(module.ccall('elf_relocation_info', 'string', [], []));
const manifest = JSON.parse(await readFile(new URL('./elf-manifest.json', import.meta.url), 'utf8'));
for (const [key, expected] of Object.entries(manifest.relocations.counts)) assert.equal(relocations[key], expected, key);
for (const sample of manifest.relocations.samples) assert.equal(module._elf_word(BigInt(sample.offset)), BigInt(sample.expected));
assert.equal(module._startup_run(), 1);
const startup = JSON.parse(module.ccall('startup_info', 'string', [], []));
assert.equal(startup.ok, true); assert.equal(startup.importsHandled, 3);
module._elf_unload();
const report = { ok: true, invalidInputScenarios: checks, image, relocations, startup, scope: 'C loader, relocation + bootstrap with MEMFS; no full game boot.' };
await writeFile(new URL('./elf-node-report.json', import.meta.url), JSON.stringify(report, null, 2));
console.log(`ELF: PASS (${checks} malformed-input/recovery scenarios, real image mapping and nninitStartup)`);
