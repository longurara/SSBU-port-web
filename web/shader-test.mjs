import {createShaderCompiler} from './shader-compiler.mjs';

export async function testShaderCompiler(device) {
  const fixtureResponse = await fetch('./shader-test.json');
  if (!fixtureResponse.ok) throw new Error(`Shader test HTTP ${fixtureResponse.status}`);
  const fixture = await fixtureResponse.json();
  const compiler = await createShaderCompiler();
  const reflected=compiler.compileWithBindings(new Uint32Array(fixture.words).buffer),code=reflected.code;
  if(JSON.stringify(reflected.activeBindings)!=='[[0,0]]')throw new Error('Shader compiler resource reflection mismatch');
  // Reject -> recover -> recompile on the same Wasm instance.
  let rejected = false;
  try { compiler.compile(new Uint8Array([1,2,3,4])); } catch { rejected = true; }
  if (!rejected || compiler.compile(new Uint32Array(fixture.words).buffer) !== code)
    throw new Error('Shader compiler error recovery failed');
  device.pushErrorScope('validation');
  const module = device.createShaderModule({code});
  const diagnostics = await module.getCompilationInfo();
  const errors = diagnostics.messages.filter(item => item.type === 'error');
  if (errors.length) throw new Error(errors.map(item=>item.message).join('; '));
  const pipeline = await device.createComputePipelineAsync({layout:'auto',compute:{module,entryPoint:'main'}});
  const output = device.createBuffer({size:16,usage:GPUBufferUsage.STORAGE|GPUBufferUsage.COPY_SRC});
  const readback = device.createBuffer({size:16,usage:GPUBufferUsage.COPY_DST|GPUBufferUsage.MAP_READ});
  try {
    const group = device.createBindGroup({layout:pipeline.getBindGroupLayout(0),entries:[{binding:0,resource:{buffer:output}}]});
    const encoder = device.createCommandEncoder();
    const pass = encoder.beginComputePass();
    pass.setPipeline(pipeline); pass.setBindGroup(0,group); pass.dispatchWorkgroups(4); pass.end();
    encoder.copyBufferToBuffer(output,0,readback,0,16); device.queue.submit([encoder.finish()]);
    await readback.mapAsync(GPUMapMode.READ);
    const values = Array.from(new Uint32Array(readback.getMappedRange()));
    readback.unmap();
    const error = await device.popErrorScope();
    if (error) throw new Error(error.message);
    if (values.some((value,index)=>value!==fixture.expected[index])) throw new Error(`Fallthrough mismatch: ${values}`);
    return {ok:true,values,activeBindings:reflected.activeBindings,malformedInputRejected:true,recoveryPassed:true,wgslBytes:code.length,
      scope:'Self-authored SPIR-V compiled by Wasm and executed on WebGPU; not a game draw'};
  } finally {output.destroy();readback.destroy();}
}
