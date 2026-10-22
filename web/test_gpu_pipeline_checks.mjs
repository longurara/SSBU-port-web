/** Mock scheduling/lifetime checks; actual pipeline validation is a browser probe. */
import assert from 'node:assert/strict';
import {WebGpuBackend} from './gpu-backend.mjs';
let scopes=0,descriptor,release,fail=false;
const device={limits:new Proxy({},{get:()=>4096}),lost:new Promise(()=>{}),addEventListener(){},
  pushErrorScope(){scopes++;},async popErrorScope(){scopes--;return null;},queue:{},
  async createRenderPipelineAsync(value){descriptor=value;return {};},
  async createComputePipelineAsync(value){descriptor=value;if(fail)throw new Error('pipeline rejected');return {};}};
const stages={
  1:[{metadata:{stage:0},module:{},code:'@vertex fn vertex_main(@location(2) uv:vec2<f32>)->@builtin(position) vec4<f32>{}'}],
  2:[{metadata:{stage:1},module:{},code:'@fragment fn fragment_main()->@location(0) vec4<f32>{}'}],
  3:[{metadata:{stage:5},module:{},code:'@compute @workgroup_size(1) fn compute_main(){}'}]
};
const backend=new WebGpuBackend(device);backend.programs.compile=async handle=>stages[handle];
await assert.rejects(backend.checkGraphicsPrograms(3,2,{}),/separate/);
const result=await backend.checkGraphicsPrograms(1,2,{});
assert.equal(result.syntheticVertexLayout,true);assert.equal(descriptor.vertex.entryPoint,'vertex_main');
assert.deepEqual(descriptor.vertex.buffers,[{arrayStride:8,stepMode:'vertex',attributes:[{shaderLocation:2,offset:0,format:'float32x2'}]}]);
assert.equal(descriptor.fragment.entryPoint,'fragment_main');assert.equal(scopes,0);
fail=true;await assert.rejects(backend.checkComputeProgram(3,{}),/pipeline rejected/);assert.equal(scopes,0);
fail=false;assert.equal((await backend.checkComputeProgram(3,{})).entryPoint,'compute_main');assert.equal(scopes,0);
device.createComputePipelineAsync=()=>new Promise(resolve=>{release=resolve;});
const pending=backend.checkComputeProgram(3,{});await new Promise(resolve=>setTimeout(resolve,0));
const count=backend.statistics.diagnosticComputePipelines;backend.close();release({});
await assert.rejects(pending,/closed/);assert.equal(backend.statistics.diagnosticComputePipelines,count);assert.equal(scopes,0);
console.log('Diagnostic pipeline queue/rejection/recovery/close: PASS (mock device; not GPU validation)');
