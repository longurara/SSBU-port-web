import assert from 'node:assert/strict';
import {stageInterface,diagnosticVertexLayout} from './wgsl-interface.mjs';
assert.deepEqual(diagnosticVertexLayout(`@vertex fn main(@builtin(vertex_index) id:u32,@location(3) a:vec4<f32>,@location(0) b:vec2<i32>)->@builtin(position)vec4<f32>{}`),
  {entryPoint:'main',buffers:[{arrayStride:24,stepMode:'vertex',attributes:[{shaderLocation:0,offset:0,format:'sint32x2'},{shaderLocation:3,offset:8,format:'float32x4'}]}]});
assert.deepEqual(diagnosticVertexLayout(`struct Output{@location(7) a:vec3<f32>,};struct Input{@location(2) uv:vec2<f32>,@builtin(instance_index) id:u32,};@vertex fn vertex_main(input:Input)->Output{}`),
  {entryPoint:'vertex_main',buffers:[{arrayStride:8,stepMode:'vertex',attributes:[{shaderLocation:2,offset:0,format:'float32x2'}]}]});
assert.equal(stageInterface('@compute @workgroup_size(8,4,1) fn compute_main(){}','compute').entryPoint,'compute_main');
assert.deepEqual(diagnosticVertexLayout('@vertex fn main(@builtin(vertex_index) id:u32)->@builtin(position)vec4<f32>{}').buffers,[]);
assert.deepEqual(stageInterface('@fragment fn main(){}','fragment').outputs,[]);
assert.deepEqual(stageInterface('@fragment fn main()->@builtin(frag_depth) f32{}','fragment').outputs,[]);
assert.deepEqual(stageInterface('@fragment fn main()->@location(2) vec4<f32>{}','fragment').outputs,[{shaderLocation:2,components:4,kind:'f32'}]);
assert.deepEqual(stageInterface('struct Out{@location(3) a:vec4<u32>,@builtin(frag_depth) z:f32,@location(1) b:f32};@fragment fn main()->Out{}','fragment').outputs,[{shaderLocation:1,components:1,kind:'f32'},{shaderLocation:3,components:4,kind:'u32'}]);
for(const source of [
  '@vertex fn main(@location(0) a:vec4<f32>,@location(0) b:vec4<f32>){}',
  '@vertex fn main(@location(0) a:vec4<f16>){}',
  '@vertex fn main(@location(32) a:u32){}',
  '@vertex fn main(input:Missing){}',
  '@vertex fn main(@location(0) a:vec4<f32>){} @vertex fn second(){}',
  '@vertex fn main(@location(0) a:vec4<f32>'
])assert.throws(()=>diagnosticVertexLayout(source));
console.log('Bounded Naga entry interface reflection: PASS (synthetic layout only)');
