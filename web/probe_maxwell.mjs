/** Actual captured Maxwell -> Wasm translator -> Wasm glslang diagnostics. */
import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {createMaxwellCompiler} from '../build/web-poc/maxwell-pipeline.mjs';
import {shaderCompilerFromInstance} from './shader-compiler.mjs';
import {lowerSpirv} from './spirv-compat.mjs';

const root=path.dirname(path.dirname(fileURLToPath(import.meta.url)));
const directory=path.join(root,'build/maxwell-capture');
const output=path.join(root,'build/web-maxwell');fs.mkdirSync(output,{recursive:true});
const compiler=await createMaxwellCompiler();
const {instance}=await WebAssembly.instantiate(fs.readFileSync(path.join(root,'build/web-poc/shader-compiler.wasm')),{});
const naga=shaderCompilerFromInstance(instance);
let files=fs.readdirSync(directory).filter(name=>name.endsWith('.mxd')).sort();
const count=Number(process.argv[2]||16);
if(!Number.isInteger(count)||count<1||count>256)throw new Error('count must be 1..256');
files=process.argv.includes('--spread')&&count>1&&files.length>count?Array.from({length:count},(_,i)=>files[Math.floor(i*(files.length-1)/(count-1))]):files.slice(0,count);
const results=[];
for(const name of files) {
  const start=performance.now();
  try {
    const result=compiler.compileCapture(fs.readFileSync(path.join(directory,name)));
    fs.writeFileSync(path.join(output,name+'.spv'),result.spirv);
    fs.writeFileSync(path.join(output,name+'.glsl'),result.glsl);
    const lowered=lowerSpirv(result.spirv,{topology:'triangles'});
    const code='diagnostic(off, derivative_uniformity);\n'+naga.compile(lowered.bytes);
    fs.writeFileSync(path.join(output,name+'.wgsl'),code);
    results.push({name,ok:true,milliseconds:performance.now()-start,spirvBytes:result.spirv.length,wgslBytes:code.length,metadata:result.metadata,layout:lowered.layout});
  } catch(error) {results.push({name,ok:false,error:String(error)});}
}
const report={scope:'Actual captured Maxwell -> Wasm translation/GLSL compilation -> WGSL; no GPU rendering proof',results,passed:results.filter(item=>item.ok).length,total:results.length};
fs.writeFileSync(path.join(output,'report.json'),JSON.stringify(report,null,2)+'\n');
console.log(JSON.stringify(report,null,2));
