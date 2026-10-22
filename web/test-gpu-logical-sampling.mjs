import {WebGpuBackend} from './gpu-backend.mjs';
import {WebGpuLogicalSamples} from './gpu-logical-sampling.mjs';

/** Real BC storage -> lossless logical-size GPU view, with analytic texels and
 * sampling probes. These are fixtures, not captured game pixels. */
export async function testLogicalSampling(device){
  const b=new WebGpuBackend(device),copies=new WebGpuLogicalSamples(b),resources=[];let checks=0;
  device.pushErrorScope('validation');
  const check=(ok,message)=>{checks++;if(!ok)throw Error(message);};
  const colors=[[1,0,0,1],[0,1,0,1],[0,0,1,1],[1,1,1,1]],codes=[0xf800,0x07e0,0x001f,0xffff];
  function packet(nvn,width=7,height=5,layers=2,levels=2){
    const parts=[];let total=36;
    for(let layer=0;layer<layers;layer++)for(let mip=0;mip<levels;mip++){
      const w=Math.max(1,width>>mip),h=Math.max(1,height>>mip),wb=Math.ceil(w/4),hb=Math.ceil(h/4),data=new Uint8Array(wb*hb*8),v=new DataView(data.buffer);
      for(let y=0;y<hb;y++)for(let x=0;x<wb;x++){const at=(y*wb+x)*8,color=codes[(x+y*2+layer+mip)%4];v.setUint16(at,color,true);v.setUint16(at+2,color,true);}
      parts.push({mip,layer,w,h,wb,hb,data});total+=24+data.length;
    }
    const bytes=new Uint8Array(total),v=new DataView(bytes.buffer);[0x5854564e,nvn,layers===1?1:4,width,height,layers,levels,0,parts.length].forEach((n,i)=>v.setUint32(i*4,n,true));let at=36;
    for(const p of parts){[p.mip,p.layer,p.w,p.h,1,p.data.length].forEach((n,i)=>v.setUint32(at+i*4,n,true));at+=24;bytes.set(p.data,at);at+=p.data.length;}return {bytes,parts};
  }
  async function read(texture,mip,layer,w,h){const pitch=Math.ceil(w*16/256)*256,buffer=device.createBuffer({size:pitch*h,usage:GPUBufferUsage.COPY_DST|GPUBufferUsage.MAP_READ});
    try{const e=device.createCommandEncoder();e.copyTextureToBuffer({texture,mipLevel:mip,origin:[0,0,layer]},{buffer,bytesPerRow:pitch},[w,h]);device.queue.submit([e.finish()]);await buffer.mapAsync(GPUMapMode.READ);const bytes=new Uint8Array(buffer.getMappedRange()),out=new Float32Array(w*h*4);for(let y=0;y<h;y++)out.set(new Float32Array(bytes.buffer,bytes.byteOffset+y*pitch,w*4),y*w*4);buffer.unmap();return out;}finally{buffer.destroy();}}
  try{
    for(const nvn of [0x42,0x46]){
      const fixture=packet(nvn),{value:handle}=await b.request(14,{bytes:fixture.bytes}),record=b.textures.get(handle);
      const {value:viewId}=await b.request(18,{handle,bytes:new Uint8Array(18)}),source=b.textures.getView(viewId);
      const copy=await copies.create(record,source);resources.push(copy.texture);
      check(copy.texture.width===7&&copy.texture.height===5&&copy.texture.mipLevelCount===2,'Logical size/mip chain changed');
      for(const p of fixture.parts){const actual=await read(copy.texture,p.mip,p.layer,p.w,p.h);for(let y=0;y<p.h;y++)for(let x=0;x<p.w;x++)for(let c=0;c<4;c++)
        check(Math.abs(actual[(y*p.w+x)*4+c]-colors[(Math.floor(x/4)+Math.floor(y/4)*2+p.layer+p.mip)%4][c])<1e-6,'Decoded logical BC texel differs');}
      const selected=await copies.create(record,{...source,descriptor:{...source.descriptor,dimension:'2d',baseMipLevel:1,mipLevelCount:1,baseArrayLayer:1,arrayLayerCount:1}});resources.push(selected.texture);
      check(selected.texture.width===3&&selected.texture.height===2&&selected.texture.depthOrArrayLayers===1,'Selected mip/layer was not rebased');
      const selectedPixels=await read(selected.texture,0,0,3,2);for(let i=0;i<selectedPixels.length;i++)check(selectedPixels[i]===colors[2][i%4],'Selected mip/layer texels differ');
      if(nvn===0x42){
        const code=`@group(0) @binding(0) var image:texture_2d<f32>; @group(0) @binding(1) var s:sampler; @group(0) @binding(2) var<storage,read_write> results:array<vec4<f32>>;
          @compute @workgroup_size(1) fn cs(){results[0]=textureSampleLevel(image,s,vec2<f32>(4.0/7.0,0.2),0);results[1]=textureSampleLevel(image,s,vec2<f32>(1,0.2),0);results[2]=vec4<f32>(vec2<f32>(textureDimensions(image)),0,1);results[3]=textureSampleLevel(image,s,vec2<f32>(0.5),1);}`;
        const module=device.createShaderModule({code}),pipeline=await device.createComputePipelineAsync({layout:'auto',compute:{module,entryPoint:'cs'}});
        for(const [mode,edge] of [['repeat',[0.5,0.5,0,1]],['clamp-to-edge',[0,1,0,1]],['mirror-repeat',[0,1,0,1]]]){
          const output=device.createBuffer({size:64,usage:GPUBufferUsage.STORAGE|GPUBufferUsage.COPY_SRC}),readback=device.createBuffer({size:64,usage:GPUBufferUsage.COPY_DST|GPUBufferUsage.MAP_READ});
          try{const group=device.createBindGroup({layout:pipeline.getBindGroupLayout(0),entries:[{binding:0,resource:copy.texture.createView({dimension:'2d',baseArrayLayer:0,arrayLayerCount:1})},{binding:1,resource:device.createSampler({addressModeU:mode,addressModeV:mode,minFilter:'linear',magFilter:'linear'})},{binding:2,resource:{buffer:output}}]});
            const e=device.createCommandEncoder(),pass=e.beginComputePass();pass.setPipeline(pipeline);pass.setBindGroup(0,group);pass.dispatchWorkgroups(1);pass.end();e.copyBufferToBuffer(output,0,readback,0,64);device.queue.submit([e.finish()]);await readback.mapAsync(GPUMapMode.READ);const actual=new Float32Array(readback.getMappedRange());const expected=[0.5,0.5,0,1,...edge,7,5,0,1,0,1,0,1];for(let i=0;i<16;i++)check(Math.abs(actual[i]-expected[i])<1e-5,'Logical dimensions, wrap/filter boundary or mip sample differs');readback.unmap();
          }finally{output.destroy();readback.destroy();}
        }
      }
      await b.request(19,{handle:viewId});await b.request(15,{handle});
    }
    // BC1 RGB must retain implicit alpha1, unlike its RGBA transparent selector.
    for(const nvn of [0x41,0x42]){const f=packet(nvn,3,3,1,1);f.bytes.fill(0,60);new DataView(f.bytes.buffer).setUint32(64,0xffffffff,true);
      const {value:handle}=await b.request(14,{bytes:f.bytes}),record=b.textures.get(handle);const copy=await copies.create(record,{descriptor:{format:record.format.format,dimension:'2d',baseMipLevel:0,mipLevelCount:1,baseArrayLayer:0,arrayLayerCount:1}});resources.push(copy.texture);
      const actual=await read(copy.texture,0,0,3,3);for(let i=0;i<actual.length;i++)check(actual[i]===(i%4===3&&nvn===0x41?1:0),'BC1 RGB/RGBA alpha differs');await b.request(15,{handle});}
    const signed=packet(0x4a,3,3,1,1);signed.bytes.fill(0,60);signed.bytes[60]=signed.bytes[61]=129;
    const {value:signedHandle}=await b.request(14,{bytes:signed.bytes}),signedRecord=b.textures.get(signedHandle);
    const signedCopy=await copies.create(signedRecord,{descriptor:{format:signedRecord.format.format,dimension:'2d',baseMipLevel:0,mipLevelCount:1,baseArrayLayer:0,arrayLayerCount:1}});resources.push(signedCopy.texture);
    const signedValues=await read(signedCopy.texture,0,0,3,3);for(let i=0;i<signedValues.length;i++)check(signedValues[i]===[-1,0,0,1][i%4],'Signed compressed value was clamped');await b.request(15,{handle:signedHandle});
    const validation=await device.popErrorScope();if(validation)throw Error(validation.message);
    return {ok:true,checks,...copies.statistics,scope:'Actual BC1/sRGB GPU decoding, NPOT extents, mip/layer rebasing, alpha and wrap/filter samples; not game pixel equivalence'};
  }finally{for(const r of resources)r.destroy();b.close();}
}
