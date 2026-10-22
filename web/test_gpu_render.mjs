/** Cancellation during asynchronous pipeline creation must not submit work to
 * destroyed resources. Physical clear/readback coverage is in test-gpu-render. */
import assert from 'node:assert/strict';
import {WebGpuRender} from './gpu-render.mjs';
globalThis.GPUBufferUsage={UNIFORM:64,COPY_DST:8};
function packet(){const bytes=new Uint8Array(60),v=new DataView(bytes.buffer);v.setUint32(0,1,true);v.setUint32(4,1,true);v.setUint32(20,1,true);v.setFloat32(44,0.5,true);return bytes;}
let resume,submits=0,views=0,writes=[],destroyed=0;
const pass={setPipeline(){},setBindGroup(){},setScissorRect(){},draw(){},end(){}};
const encoder={beginRenderPass:()=>pass,finish:()=>({})};
const pipeline={getBindGroupLayout:()=>({})};
const device={createShaderModule:()=>({}),createRenderPipelineAsync:()=>new Promise(resolve=>{resume=resolve;}),
  createCommandEncoder:()=>encoder,createBuffer:()=>({destroy(){destroyed++;}}),createBindGroup:()=>({}),
  queue:{submit(){submits++;},writeBuffer(buffer,offset,data){writes.push(Array.from(data));}}};
const record={format:{format:'rgba8unorm',renderable:true},dimension:'2d',width:4,height:4,levels:1,layers:1,
  texture:{createView(){views++;return {};}}};
const textures={get(handle){assert.equal(handle,1);return record;}};
let render=new WebGpuRender(device,textures,action=>action());
const pending=render.clearColors(packet());render.close();resume(pipeline);
await assert.rejects(pending,/closed/);assert.equal(submits,0);assert.equal(views,0);assert.equal(render.pipelines.size,0);
await assert.rejects(render.clearColors(packet()),/closed/);
await assert.rejects(render.pipeline('rgba8unorm',0,1),/closed/);
render=new WebGpuRender(device,textures,action=>action());
const input=packet(),work=render.clearColors(input);
new DataView(input.buffer).setFloat32(44,0.75,true);resume(pipeline);await work;
assert.equal(submits,1);assert.equal(views,1);assert.deepEqual(writes,[[0.5,0,0,0]]);assert.equal(destroyed,1);
render.close();
console.log('Color-clear cancellation, immutable parsed values and staging cleanup: PASS (mock device)');
