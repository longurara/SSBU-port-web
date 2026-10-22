import createMaxwell from './maxwell.mjs';

/** Own one compiler per Worker. Results are copied before the C compiler is
 * reused, so program variants do not retain borrowed compiler heap pointers. */
export async function createMaxwellCompiler() {
  const module = await createMaxwell();
  function compile(control, data, options = {}) {
    control = control instanceof Uint8Array ? control : new Uint8Array(control);
    data = data instanceof Uint8Array ? data : new Uint8Array(data);
    if (control.length < 0x720 || control.length > 2 * 1024 * 1024 || data.length > 8 * 1024 * 1024)
      throw new Error('Invalid NVN shader storage sizes');
    const flags = ['integerAttributes','signedAttributes','integerTextures','signedTextures','forceOutputs'].map(key => {
      const value = options[key] ?? 0;
      if (!Number.isInteger(value) || value < 0 || value > 0xffffffff) throw new Error(`Invalid ${key}`);
      return value;
    });
    const pointer = module._malloc(control.length + data.length);
    if (!pointer) throw new Error('Cannot allocate Maxwell input');
    try {
      module.HEAPU8.set(control, pointer);module.HEAPU8.set(data, pointer + control.length);
      const ok = module._mx_compile(pointer,control.length,pointer+control.length,data.length,...flags);
      if (!ok) throw new Error(module.UTF8ToString(module._mx_error()));
      const spirv = module.HEAPU8.slice(module._mx_spirv(),module._mx_spirv()+module._mx_spirv_size());
      return {spirv,glsl:module.UTF8ToString(module._mx_glsl()),metadata:JSON.parse(module.UTF8ToString(module._mx_info()))};
    } finally {module._free(pointer);}
  }
  return {compile,compileCapture(bytes,options) {
    bytes = bytes instanceof Uint8Array ? bytes : new Uint8Array(bytes);
    if (bytes.length < 20) throw new Error('Truncated MXD1 capture');
    const view = new DataView(bytes.buffer,bytes.byteOffset,bytes.byteLength);
    if (view.getUint32(0,true) !== 0x3144584d) throw new Error('Expected MXD1 capture');
    const controlSize=view.getUint32(12,true),dataSize=view.getUint32(16,true);
    if (20+controlSize+dataSize !== bytes.length) throw new Error('MXD1 capture sizes do not match storage');
    return compile(bytes.subarray(20,20+controlSize),bytes.subarray(20+controlSize),options);
  }};
}
