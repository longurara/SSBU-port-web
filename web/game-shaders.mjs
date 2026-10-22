import {createMaxwellCompiler} from './maxwell-pipeline.mjs';
import {createShaderCompiler} from './shader-compiler.mjs';
import {lowerSpirv} from './spirv-compat.mjs';
import {specializeUniformBranches} from './spirv-uniform-branches.mjs';

/** The renderer provides real topology and resource state for each variant.
 * No shader cache or native compilation service is required. */
export async function createGameShaderCompiler() {
  const [maxwell,wgsl]=await Promise.all([createMaxwellCompiler(),createShaderCompiler()]);
  function finish(translated,options) {
    const specialization=options.uniformWords?specializeUniformBranches(translated.spirv,options.uniformWords):null;
    const lowered=lowerSpirv(specialization?.bytes??translated.spirv,options);
    if(specialization?.branches)lowered.layout.uniformSpecialization={branches:specialization.branches,reads:specialization.reads};
    // The Maxwell translator reconstructs control flow as a per-invocation
    // PC loop. WGSL cannot prove the original warp reconvergence from this
    // representation. Retain the original implicit-derivative operations;
    // suppress only their static uniformity diagnostic, not GPU validation.
    // Pixel/derivative parity still requires actual native-vs-web draw tests.
    const generated=wgsl.compileWithBindings(lowered.bytes);
    const code='diagnostic(off, derivative_uniformity);\n'+generated.code;
    return {code,activeBindings:generated.activeBindings,layout:lowered.layout,metadata:translated.metadata};
  }
  return {
    compile(control,data,options={}) {return finish(maxwell.compile(control,data,options),options);},
    compileCapture(bytes,options={}) {return finish(maxwell.compileCapture(bytes,options),options);}
  };
}
