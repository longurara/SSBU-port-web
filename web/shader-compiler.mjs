/** One compiler instance per Worker. Input must already use WebGPU-compatible
 * interfaces; unsupported SPIR-V is reported instead of replaced by a dummy. */
export async function createShaderCompiler(url = new URL('./shader-compiler.wasm', import.meta.url)) {
  const response = await fetch(url);
  if (!response.ok) throw new Error(`Shader compiler HTTP ${response.status}`);
  const {instance} = await WebAssembly.instantiateStreaming(response, {});
  return shaderCompilerFromInstance(instance);
}

export function shaderCompilerFromInstance(instance) {
  const api = instance.exports;
  function compileWithBindings(bytes) {
      const input = bytes instanceof Uint8Array ? bytes : new Uint8Array(bytes);
      if (!input.byteLength || input.byteLength > 16 * 1024 * 1024) throw new Error('SPIR-V input must be 1 byte to 16 MiB');
      const pointer = api.shader_input(input.byteLength);
      if (!pointer) throw new Error('Cannot allocate shader input');
      new Uint8Array(api.memory.buffer, pointer, input.byteLength).set(input);
      const status = api.shader_convert();
      const output = new TextDecoder().decode(new Uint8Array(api.memory.buffer, api.shader_output(), api.shader_output_size()));
      if (status) throw new Error(output);
      const activeBindings=JSON.parse(new TextDecoder().decode(new Uint8Array(api.memory.buffer,api.shader_bindings(),api.shader_bindings_size())));
      return {code:output,activeBindings};
  }
  return {compile:bytes=>compileWithBindings(bytes).code,compileWithBindings};
}
