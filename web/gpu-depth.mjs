/** Depth/stencil transfers using the native backend's source encodings from
 * runtime/tex_decode.c. WebGPU forbids buffer uploads to D24/D32 depth aspects;
 * write frag_depth instead, and sample opaque D24 storage for readback.
 * https://gpuweb.github.io/gpuweb/#depth-formats */
export class WebGpuDepth {
  constructor(device,checked){this.device=device;this.checked=checked;this.closed=false;this.pipelines=new Map();this.readPipeline=null;
    this.statistics={depthUploadPasses:0,depthReadPasses:0,stencilUploadWrites:0};}
  validate(nvn,bytes) {
    const step=nvn===0x31?1:nvn===0x32?2:nvn===0x36?8:4,count=bytes.length/step;
    if(nvn<0x31||nvn>0x36||!Number.isInteger(count))throw new Error('Invalid depth/stencil source length or format');
    if(nvn===0x34||nvn===0x36){const view=new DataView(bytes.buffer,bytes.byteOffset,bytes.byteLength);
      for(let at=0;at<bytes.length;at+=step){const value=view.getFloat32(at,true);if(!Number.isFinite(value)||value<0||value>1)throw new Error('Depth source outside the supported finite [0,1] range');}}
  }
  decode(nvn,bytes) {
    this.validate(nvn,bytes);
    const step=nvn===0x31?1:nvn===0x32?2:nvn===0x36?8:4,count=bytes.length/step;
    const view=new DataView(bytes.buffer,bytes.byteOffset,bytes.byteLength);
    const depth=nvn===0x31?null:new Float32Array(count),stencil=[0x31,0x35,0x36].includes(nvn)?new Uint8Array(count):null;
    for(let i=0;i<count;i++) {
      const at=i*step;
      if(depth){
        const value=nvn===0x32?view.getUint16(at,true)/65535:nvn===0x33?view.getUint32(at,true)/4294967295:
          nvn===0x35?(view.getUint32(at,true)>>>8)/16777215:view.getFloat32(at,true);
        depth[i]=value;
      }
      if(stencil)stencil[i]=nvn===0x31?view.getUint8(at):view.getUint8(at+(nvn===0x36?4:0));
    }
    return {depth,stencil};
  }
  async uploadPipeline(format) {
    if(this.pipelines.has(format))return this.pipelines.get(format);
    const shader=this.device.createShaderModule({code:`
      @group(0) @binding(0) var source:texture_2d<f32>;
      struct Origin {value:vec4<i32>}
      @group(0) @binding(1) var<uniform> origin:Origin;
      @vertex fn vertex_main(@builtin(vertex_index) index:u32)->@builtin(position) vec4<f32> {
        let p=array<vec2<f32>,3>(vec2<f32>(-1,-1),vec2<f32>(3,-1),vec2<f32>(-1,3));return vec4<f32>(p[index],0,1);
      }
      @fragment fn fragment_main(@builtin(position) p:vec4<f32>)->@builtin(frag_depth) f32 {
        return textureLoad(source,vec2<i32>(p.xy)-origin.value.xy,0).x;
      }`});
    const groupLayout=this.device.createBindGroupLayout({entries:[
      {binding:0,visibility:GPUShaderStage.FRAGMENT,texture:{sampleType:'unfilterable-float',viewDimension:'2d'}},
      {binding:1,visibility:GPUShaderStage.FRAGMENT,buffer:{type:'uniform'}}]});
    const pipeline=await this.device.createRenderPipelineAsync({layout:this.device.createPipelineLayout({bindGroupLayouts:[groupLayout]}),vertex:{module:shader,entryPoint:'vertex_main'},
      fragment:{module:shader,entryPoint:'fragment_main',targets:[]},primitive:{topology:'triangle-list'},
      depthStencil:{format,depthWriteEnabled:true,depthCompare:'always',stencilWriteMask:0}});
    if(this.closed)throw new Error('Depth backend closed during pipeline creation');
    this.pipelines.set(format,pipeline);return pipeline;
  }
  async upload(record,texture,{mip,layer,w,h,bytes,x=0,y=0}) {
    if(this.closed)throw new Error('Depth backend closed');
    const {depth,stencil}=this.decode(record.nvn,bytes),destination={texture,mipLevel:mip,origin:[x,y,layer]};
    if(bytes.length!==w*h*record.format.sourceBpb)throw new Error('Depth source region/length mismatch');
    const width=Math.max(1,record.width>>mip),height=Math.max(1,record.height>>mip),full=x===0&&y===0&&w===width&&h===height;
    if(stencil){
      let data=stencil;
      if(!full){
        data=await this.aspect({...record,texture},{mip,w:width,h:height},layer,'stencil-only',1);
        for(let row=0;row<h;row++)data.set(stencil.subarray(row*w,(row+1)*w),(y+row)*width+x);
      }
      if(this.closed)throw new Error('Depth backend closed during stencil staging');
      this.device.queue.writeTexture({texture,mipLevel:mip,origin:[0,0,layer],aspect:'stencil-only'},data,{bytesPerRow:width,rowsPerImage:height},[width,height,1]);this.statistics.stencilUploadWrites++;
    }
    if(!depth)return;
    if(record.nvn===0x32&&full){this.device.queue.writeTexture({...destination,aspect:'depth-only'},bytes,{bytesPerRow:w*2,rowsPerImage:h},[w,h,1]);return;}
    const pipeline=await this.uploadPipeline(record.format.format);let source,origin;
    try {
      if(this.closed)throw new Error('Depth backend closed during upload');
      source=this.device.createTexture({size:[w,h],format:'r32float',usage:GPUTextureUsage.TEXTURE_BINDING|GPUTextureUsage.COPY_DST});
      this.device.queue.writeTexture({texture:source},depth,{bytesPerRow:w*4,rowsPerImage:h},[w,h]);
      origin=this.device.createBuffer({size:16,usage:GPUBufferUsage.UNIFORM|GPUBufferUsage.COPY_DST});this.device.queue.writeBuffer(origin,0,new Int32Array([x,y,0,0]));
      const group=this.device.createBindGroup({layout:pipeline.getBindGroupLayout(0),entries:[{binding:0,resource:source.createView()},{binding:1,resource:{buffer:origin}}]});
      const attachment={view:texture.createView({dimension:'2d',baseMipLevel:mip,mipLevelCount:1,baseArrayLayer:layer,arrayLayerCount:1}),depthLoadOp:'load',depthStoreOp:'store'};
      if(record.format.stencil)attachment.stencilReadOnly=true;
      const encoder=this.device.createCommandEncoder(),pass=encoder.beginRenderPass({colorAttachments:[],depthStencilAttachment:attachment});
      pass.setPipeline(pipeline);pass.setBindGroup(0,group);pass.setScissorRect(x,y,w,h);pass.draw(3);pass.end();
      if(this.closed)throw new Error('Depth backend closed before upload submission');
      this.device.queue.submit([encoder.finish()]);this.statistics.depthUploadPasses++;
    }finally{source?.destroy();origin?.destroy();}
  }
  async aspect(record,item,layer,aspect,bpp) {
    const row=item.w*bpp,pitch=Math.ceil(row/256)*256,size=pitch*item.h;
    const buffer=this.device.createBuffer({size,usage:GPUBufferUsage.COPY_DST|GPUBufferUsage.MAP_READ});
    try {
      await this.checked(()=>{const encoder=this.device.createCommandEncoder();
        encoder.copyTextureToBuffer({texture:record.texture,mipLevel:item.mip,origin:[0,0,layer],aspect},{buffer,bytesPerRow:pitch,rowsPerImage:item.h},[item.w,item.h,1]);
        this.device.queue.submit([encoder.finish()]);});
      await buffer.mapAsync(GPUMapMode.READ);const mapped=new Uint8Array(buffer.getMappedRange()),output=new Uint8Array(row*item.h);
      for(let y=0;y<item.h;y++)output.set(mapped.subarray(y*pitch,y*pitch+row),y*row);
      buffer.unmap();return output;
    }finally{buffer.destroy();}
  }
  async opaqueDepth(record,item,layer) {
    let output,readback;
    try {
      const size=item.w*item.h*4;
      await this.checked(async()=>{
        if(!this.readPipeline){
          const module=this.device.createShaderModule({code:`
            @group(0) @binding(0) var source:texture_depth_2d;
            @group(0) @binding(1) var<storage,read_write> result:array<f32>;
            @compute @workgroup_size(8,8) fn main(@builtin(global_invocation_id) id:vec3<u32>) {
              let size=textureDimensions(source,0);if(id.x<size.x&&id.y<size.y){result[id.y*size.x+id.x]=textureLoad(source,vec2<i32>(id.xy),0);}
            }`});
          const pipeline=await this.device.createComputePipelineAsync({layout:'auto',compute:{module,entryPoint:'main'}});
          if(this.closed)throw new Error('Depth backend closed during read pipeline creation');this.readPipeline=pipeline;
        }
        if(this.closed)throw new Error('Depth backend closed during readback');
        output=this.device.createBuffer({size,usage:GPUBufferUsage.STORAGE|GPUBufferUsage.COPY_SRC});
        readback=this.device.createBuffer({size,usage:GPUBufferUsage.COPY_DST|GPUBufferUsage.MAP_READ});
        const view=record.texture.createView({format:'depth24plus',dimension:'2d',aspect:'depth-only',baseMipLevel:item.mip,mipLevelCount:1,baseArrayLayer:layer,arrayLayerCount:1});
        const group=this.device.createBindGroup({layout:this.readPipeline.getBindGroupLayout(0),entries:[{binding:0,resource:view},{binding:1,resource:{buffer:output}}]});
        const encoder=this.device.createCommandEncoder(),pass=encoder.beginComputePass();pass.setPipeline(this.readPipeline);pass.setBindGroup(0,group);
        pass.dispatchWorkgroups(Math.ceil(item.w/8),Math.ceil(item.h/8));pass.end();encoder.copyBufferToBuffer(output,0,readback,0,size);this.device.queue.submit([encoder.finish()]);
      });
      await readback.mapAsync(GPUMapMode.READ);const result=new Float32Array(readback.getMappedRange().slice(0));readback.unmap();this.statistics.depthReadPasses++;return result;
    }finally{output?.destroy();readback?.destroy();}
  }
  async read(record,item,layer) {
    if(this.closed)throw new Error('Depth backend closed');
    if(record.nvn===0x31)return this.aspect(record,item,layer,'stencil-only',1);
    if(record.nvn===0x32||record.nvn===0x34)return this.aspect(record,item,layer,'depth-only',record.format.sourceBpb);
    const depth=record.nvn===0x36?new Float32Array((await this.aspect(record,item,layer,'depth-only',4)).buffer):await this.opaqueDepth(record,item,layer);
    const stencil=record.format.stencil?await this.aspect(record,item,layer,'stencil-only',1):null;
    const bytes=new Uint8Array(item.w*item.h*record.format.sourceBpb),v=new DataView(bytes.buffer);
    for(let i=0;i<depth.length;i++) {
      if(record.nvn===0x33)v.setUint32(i*4,Math.round(depth[i]*4294967295),true);
      else if(record.nvn===0x35)v.setUint32(i*4,((Math.round(depth[i]*16777215)<<8)|stencil[i])>>>0,true);
      else{v.setFloat32(i*8,depth[i],true);v.setUint32(i*8+4,stencil[i],true);}
    }
    return bytes;
  }
  close(){this.closed=true;this.pipelines.clear();this.readPipeline=null;}
}
