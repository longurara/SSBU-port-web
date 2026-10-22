import assert from 'node:assert/strict';
import {deviceCapabilities} from './gpu-backend.mjs';
const limits={maxUniformBuffersPerShaderStage:12,maxUniformBufferBindingSize:65536,minUniformBufferOffsetAlignment:256,
  maxColorAttachments:8,maxVertexBuffers:8,maxStorageBuffersPerShaderStage:10,maxSampledTexturesPerShaderStage:16,
  maxVertexAttributes:16,maxComputeWorkgroupSizeX:256,maxComputeWorkgroupSizeY:256,maxComputeWorkgroupSizeZ:64,
  maxComputeInvocationsPerWorkgroup:256,maxComputeWorkgroupsPerDimension:65535,maxStorageTexturesPerShaderStage:4,
  maxTextureDimension2D:8192,maxTextureArrayLayers:256,maxTextureDimension3D:2048,maxSamplersPerShaderStage:16};
const caps=deviceCapabilities({limits});
assert.equal(caps.length,0x62);
for(const [index,value] of [[0,53],[1,312],[2,12],[4,256],[6,8],[7,0],[8,10],[0x1c,256],[0x1e,64],
  [0x20,65535],[0x23,4],[0x26,1],[0x29,1],[0x2a,0],[0x2c,14],[0x2d,256],[0x2e,1],
  [0x31,5],[0x3a,65536],[0x3b,0],[0x3d,8192],[0x3e,0],[0x3f,2048],[0x42,0],[0x4f,0],
  [0x58,4],[0x5a,0],[0x5e,16],[0x5f,16],[0x61,0]])assert.equal(caps[index],value,`capability ${index.toString(16)}`);
assert.throws(()=>deviceCapabilities({limits:{...limits,maxTextureDimension2D:NaN}}));
console.log('NVN capabilities: PASS (98 defined fields; physical limits, ABI alignment, unsupported features)');
