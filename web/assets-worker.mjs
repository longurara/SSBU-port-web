import create from './assets.mjs';

let busy = false;
self.onmessage = async ({ data }) => {
  if (busy) return;
  busy = true;
  let module;
  try {
    module = await create();
    const { file, manifest, kind = 'assets' } = data;
    if (!(file instanceof File) || file.size !== manifest.size) throw new Error('Kích thước file không khớp dump đã chuẩn bị.');
    module.FS.mkdir('/romfs');
    module.FS.mount(module.WORKERFS, { blobs: [{ name: kind === 'elf' ? 'main.elf' : 'data.arc', data: file }] }, '/romfs');
    if (kind === 'elf') {
      const started = performance.now();
      module.ccall('elf_load', 'number', ['string'], ['/romfs/main.elf']);
      const result = JSON.parse(module.ccall('elf_info', 'string', [], []));
      if (!result.ok) throw new Error(result.error);
      result.samples = manifest.samples.map(sample => ({ ...sample,
        actual: module._elf_hash(BigInt(sample.offset), sample.length) >>> 0 }));
      result.ok = result.readonlyEnforced && result.segments === manifest.segments && result.samples.every(sample => sample.actual === sample.fnv32);
      if (result.ok) {
        module._elf_relocate();
        result.relocations = JSON.parse(module.ccall('elf_relocation_info', 'string', [], []));
        const countsMatch = Object.entries(manifest.relocations.counts).every(([key, expected]) => result.relocations[key] === expected);
        const valuesMatch = manifest.relocations.samples.every(sample => module._elf_word(BigInt(sample.offset)) === BigInt(sample.expected));
        result.relocations.referenceMatch = countsMatch && valuesMatch;
        result.ok = result.relocations.ok && result.relocations.referenceMatch;
        result.relocationsApplied = result.ok;
      }
      if (result.ok) {
        module._startup_run();
        result.startup = JSON.parse(module.ccall('startup_info', 'string', [], []));
        result.ok = result.startup.ok;
      }
      result.wasmHeapBytes = module.HEAPU8.byteLength;
      result.milliseconds = performance.now() - started;
      result.scope = manifest.scope;
      module._elf_unload();
      self.postMessage({ type: 'result', result });
      return;
    }
    const size = module.ccall('asset_size', 'bigint', ['string'], ['/romfs/data.arc']);
    if (size !== BigInt(manifest.size)) throw new Error(`64-bit file size mismatch: ${size}`);
    const samples = [];
    const started = performance.now();
    for (const sample of manifest.samples) {
      const hash = module.ccall('asset_hash', 'number', ['string', 'bigint', 'number'], ['/romfs/data.arc', BigInt(sample.offset), sample.length]) >>> 0;
      const status = module._asset_status();
      const ok = status === 0 && hash === sample.fnv32;
      samples.push({ ...sample, actual: hash, status, ok });
      self.postMessage({ type: 'progress', completed: samples.length, total: manifest.samples.length });
    }
    // Short reads must be reported, rather than hashing uninitialised bytes.
    module.ccall('asset_hash', 'number', ['string', 'bigint', 'number'], ['/romfs/data.arc', size - 2n, 4]);
    const eofRejected = module._asset_status() === 4;
    const result = { size: String(size), samples, eofRejected,
      bytesChecked: samples.reduce((sum, item) => sum + item.length, 0),
      milliseconds: performance.now() - started,
      wasmHeapBytes: module.HEAPU8.byteLength,
      ok: eofRejected && samples.every(sample => sample.ok),
      scope: 'Wasm C stdio + WORKERFS range reads of a local File; sampled checks, not whole-file verification or game boot.' };
    self.postMessage({ type: 'result', result });
  } catch (error) { self.postMessage({ type: 'error', message: String(error) }); }
  finally {
    try { module.FS.unmount('/romfs'); } catch { /* not mounted on input errors */ }
    self.close();
  }
};
