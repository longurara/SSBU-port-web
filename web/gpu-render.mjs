import {textureFormat} from './gpu-textures.mjs';
/** Ordered color clears of physical render-target subresources. Coordinates
 * use WebGPU's top-left origin; native viewport-origin translation belongs to
 * the NVN command frontend. The complete packet is validated before rendering. */
export class WebGpuRender {
  constructor(device,textures,checked,{canvas,diagnosticFrames=false}={}) {
    this.device=device;this.textures=textures;this.checked=checked;
    this.pipelines=new Map();this.closed=false;
    this.canvas=canvas;
    if(diagnosticFrames)this.frameDiagnostics={samples:[],scope:'Raw GPU presentation-source pixels in the selected crop; not proof of canvas visibility or game input'};
    this.statistics={colorClearSubmissions:0,colorClears:0,maskedColorClears:0,clearPipelines:0,framesPresented:0,
      depthClearSubmissions:0,depthStencilClears:0,maskedDepthStencilClears:0,depthClearPipelines:0};
  }
  async present(handle,bytes) {
    if(this.closed)throw new Error('Render backend closed');
    if(!this.canvas)throw new Error('Game presentation canvas is unavailable');
    if(!(bytes instanceof Uint8Array)||bytes.length!==16)throw new Error('Invalid presentation crop packet');
    const record=this.textures.get(handle),format=record.format.format;
    if(record.dimension!=='2d'||record.layers!==1||record.format.depth||record.format.stencil||
       !['rgba8unorm','rgba8unorm-srgb','bgra8unorm','bgra8unorm-srgb','rgb10a2unorm'].includes(format))
      throw new Error(`Unsupported window texture format ${format}`);
    const data=new DataView(bytes.buffer,bytes.byteOffset,bytes.byteLength);
    let [x,y,width,height]=Array.from({length:4},(_,i)=>data.getInt32(i*4,true));
    if(width===0&&height===0){x=0;y=0;width=record.width;height=record.height;}
    if(x<0||y<0||width<=0||height<=0||x+width>record.width||y+height>record.height)
      throw new Error('Presentation crop exceeds the window texture');
    await this.checked(async()=>{
      if(!this.presentPipeline) {
        this.presentFormat=navigator.gpu.getPreferredCanvasFormat();
        this.presentViewFormat=this.presentFormat+'-srgb';
        this.presentContext=this.canvas.getContext('webgpu');
        if(!this.presentContext)throw new Error('Game WebGPU canvas context is unavailable');
        this.presentContext.configure({device:this.device,format:this.presentFormat,viewFormats:[this.presentViewFormat],alphaMode:'opaque',usage:GPUTextureUsage.RENDER_ATTACHMENT|GPUTextureUsage.COPY_SRC});
        const shader=this.device.createShaderModule({code:`
          struct Crop {rect:vec4<f32>};
          @group(0) @binding(0) var image:texture_2d<f32>;
          @group(0) @binding(1) var<uniform> crop:Crop;
          @vertex fn present_vertex(@builtin(vertex_index) index:u32)->@builtin(position) vec4<f32> {
            let p=array<vec2<f32>,3>(vec2<f32>(-1,-1),vec2<f32>(3,-1),vec2<f32>(-1,3));
            return vec4<f32>(p[index],0,1);
          }
          @fragment fn present_fragment(@builtin(position) p:vec4<f32>)->@location(0) vec4<f32> {
            return textureLoad(image,vec2<i32>(crop.rect.xy+p.xy),0);
          }
        `});
        this.presentPipeline=await this.device.createRenderPipelineAsync({layout:'auto',vertex:{module:shader,entryPoint:'present_vertex'},
          fragment:{module:shader,entryPoint:'present_fragment',targets:[{format:this.presentViewFormat}]},primitive:{topology:'triangle-list'}});
        this.presentCrop=this.device.createBuffer({size:16,usage:GPUBufferUsage.UNIFORM|GPUBufferUsage.COPY_DST});
      }
      if(this.closed)throw new Error('Render backend closed during presentation');
      if(this.canvas.width!==width)this.canvas.width=width;
      if(this.canvas.height!==height)this.canvas.height=height;
      this.device.queue.writeBuffer(this.presentCrop,0,new Float32Array([x,y,width,height]));
      const group=this.device.createBindGroup({layout:this.presentPipeline.getBindGroupLayout(0),entries:[
        {binding:0,resource:record.texture.createView({dimension:'2d',baseMipLevel:0,mipLevelCount:1,baseArrayLayer:0,arrayLayerCount:1})},
        {binding:1,resource:{buffer:this.presentCrop}}]});
      const encoder=this.device.createCommandEncoder();
      const pass=encoder.beginRenderPass({colorAttachments:[{view:this.presentContext.getCurrentTexture().createView({format:this.presentViewFormat}),loadOp:'clear',storeOp:'store',clearValue:[0,0,0,1]}]});
      pass.setPipeline(this.presentPipeline);pass.setBindGroup(0,group);pass.draw(3);pass.end();
      this.device.queue.submit([encoder.finish()]);
    });
    this.statistics.framesPresented++;
    const frame=this.statistics.framesPresented;
    if(this.frameDiagnostics&&(frame<=4||frame%60===0)) {
      try {
        // Read only after the presentation submission, before the next ordered
        // backend request can modify this texture. Keep diagnostic errors out
        // of normal game execution and bound both readback and retained data.
        if(record.width*record.height*4>67108864)throw new Error('Presentation diagnostic exceeds 64 MiB');
        const pixels=await this.textures.read(handle,0,0),view=new DataView(pixels.buffer,pixels.byteOffset,pixels.byteLength);
        let nonBlackPixels=0,hash=2166136261,left=width,top=height,right=-1,bottom=-1;
        const minimum=[Infinity,Infinity,Infinity],maximum=[0,0,0],packed=format==='rgb10a2unorm';
        for(let row=0;row<height;row++)for(let col=0;col<width;col++) {
          const at=((y+row)*record.width+x+col)*4;
          const value=packed?view.getUint32(at,true):0;
          const rgb=packed?[value&1023,(value>>>10)&1023,(value>>>20)&1023]:Array.from(pixels.subarray(at,at+3));
          if(!packed&&format.startsWith('bgra'))[rgb[0],rgb[2]]=[rgb[2],rgb[0]];
          if(rgb.some(v=>v!==0)){nonBlackPixels++;left=Math.min(left,col);top=Math.min(top,row);right=Math.max(right,col);bottom=Math.max(bottom,row);}
          for(let c=0;c<3;c++){minimum[c]=Math.min(minimum[c],rgb[c]);maximum[c]=Math.max(maximum[c],rgb[c]);}
          for(let c=0;c<4;c++)hash=Math.imul(hash^pixels[at+c],16777619)>>>0;
        }
        const sample={frame,handle,format,crop:[x,y,width,height],nonBlackPixels,minimum,maximum,encodedChannelMaximum:packed?1023:255,
          bounds:nonBlackPixels?[left,top,right-left+1,bottom-top+1]:null,hash:hash.toString(16).padStart(8,'0')};
        this.frameDiagnostics.samples.push(sample);
        if(this.frameDiagnostics.samples.length>8)this.frameDiagnostics.samples.splice(1,1);
      }catch(error){this.frameDiagnostics.lastError=String(error);}
    }
  }
  parse(bytes) {
    if(!(bytes instanceof Uint8Array)||bytes.length<4||bytes.length>4+4096*60)throw new Error('Invalid color clear packet');
    const data=new DataView(bytes.buffer,bytes.byteOffset,bytes.byteLength),count=data.getUint32(0,true);
    const stride=bytes.length===4+count*56?56:60;
    if(count>4096||bytes.length!==4+count*stride)throw new Error('Invalid color clear packet length');
    const commands=[];
    for(let i=0;i<count;i++) {
      const at=4+i*stride,get=offset=>data.getUint32(at+offset,true);
      const handle=get(0),mip=get(4),layer=get(8),kind=get(12),mask=get(16),scissor=get(20);
      const record=this.textures.get(handle),override=stride===60?get(56):0;
      const viewFormat=override?textureFormat(override):record.format,format=viewFormat.format,base=record.format.format;
      const srgbPair=(base==='rgba8unorm'||base==='bgra8unorm')?base+'-srgb':base.endsWith('-srgb')?base.slice(0,-5):null;
      if((format!==base&&format!==srgbPair)||viewFormat.sourceBpb!==record.format.sourceBpb||!!viewFormat.expand!==!!record.format.expand)
        throw new Error('Render-target view format reinterpretation is unsupported');
      const targetRenderable=record.format.renderable&&(!record.format.renderableFeature||this.device.features.has(record.format.renderableFeature));
      const viewRenderable=viewFormat.renderable&&(!viewFormat.renderableFeature||this.device.features.has(viewFormat.renderableFeature));
      if(record.dimension!=='2d'||record.format.depth||record.format.stencil||!targetRenderable||!viewRenderable||mip>=record.levels||layer>=record.layers)
        throw new Error(`Color clear requires a renderable 2D subresource (nvn=0x${record.nvn.toString(16)} fmt=${record.format.format} viewFmt=${viewFormat.format} dim=${record.dimension} mip=${mip}/${record.levels} layer=${layer}/${record.layers})`);
      const expected=format.endsWith('uint')?2:format.endsWith('sint')?1:0;
      if(kind!==expected||mask>15||scissor>1)throw new Error('Invalid color clear value type, mask or scissor');
      const width=Math.max(1,record.width>>mip),height=Math.max(1,record.height>>mip);
      const rect=scissor?[get(24),get(28),get(32),get(36)]:[0,0,width,height];
      if(rect[0]>width||rect[2]>width-rect[0]||rect[1]>height||rect[3]>height-rect[1])throw new Error('Color clear scissor exceeds the target');
      const value=Array.from({length:4},(_,channel)=>data[kind===0?'getFloat32':kind===1?'getInt32':'getUint32'](at+40+channel*4,true));
      if(value.some(item=>!Number.isFinite(item)))throw new Error('Non-finite color clear value');
      // RGB storage is expanded to RGBA physically. Its synthetic alpha must
      // retain the format's implicit one, even if NVN requests alpha writes.
      const writeMask=record.format.expand?mask&7:mask;
      commands.push({record,format,mip,layer,kind,mask:writeMask,rect,value,
        full:writeMask===15&&rect[0]===0&&rect[1]===0&&rect[2]===width&&rect[3]===height});
    }
    return commands;
  }
  async pipeline(format,kind,mask) {
    if(this.closed)throw new Error('Render backend closed');
    const key=`${format}/${kind}/${mask}`;
    if(this.pipelines.has(key)){const pipeline=this.pipelines.get(key);this.pipelines.delete(key);this.pipelines.set(key,pipeline);return pipeline;}
    const type=['f32','i32','u32'][kind];
    const shader=this.device.createShaderModule({code:`
      struct Color {value:vec4<${type}>}
      @group(0) @binding(0) var<uniform> color:Color;
      @vertex fn clear_vertex(@builtin(vertex_index) index:u32)->@builtin(position) vec4<f32> {
        let positions=array<vec2<f32>,3>(vec2<f32>(-1.0,-1.0),vec2<f32>(3.0,-1.0),vec2<f32>(-1.0,3.0));
        return vec4<f32>(positions[index],0.0,1.0);
      }
      @fragment fn clear_fragment()->@location(0) vec4<${type}> {return color.value;}
    `});
    const pipeline=await this.device.createRenderPipelineAsync({layout:'auto',
      vertex:{module:shader,entryPoint:'clear_vertex'},fragment:{module:shader,entryPoint:'clear_fragment',targets:[{format,writeMask:mask}]},
      primitive:{topology:'triangle-list'}});
    if(this.closed)throw new Error('Render backend closed during clear pipeline creation');
    if(this.pipelines.size>=256)this.pipelines.delete(this.pipelines.keys().next().value);
    this.pipelines.set(key,pipeline);this.statistics.clearPipelines++;return pipeline;
  }
  async clearColors(bytes) {
    if(this.closed)throw new Error('Render backend closed');
    const commands=this.parse(bytes).filter(command=>command.mask&&command.rect[2]&&command.rect[3]),buffers=[];
    try {
      await this.checked(async()=>{
        const encoder=this.device.createCommandEncoder();
        for(const command of commands) {
          const {record,format,mip,layer,kind,mask,value,rect,full}=command;
          let pipeline,group;
          if(!full) {
            pipeline=await this.pipeline(format,kind,mask);
            if(this.closed)throw new Error('Render backend closed during color clear');
            const data=kind===0?new Float32Array(value):kind===1?new Int32Array(value):new Uint32Array(value);
            const buffer=this.device.createBuffer({size:16,usage:GPUBufferUsage.UNIFORM|GPUBufferUsage.COPY_DST});buffers.push(buffer);
            this.device.queue.writeBuffer(buffer,0,data);
            group=this.device.createBindGroup({layout:pipeline.getBindGroupLayout(0),entries:[{binding:0,resource:{buffer}}]});
          }
          if(this.closed)throw new Error('Render backend closed during color clear');
          const view=record.texture.createView({format,dimension:'2d',baseMipLevel:mip,mipLevelCount:1,baseArrayLayer:layer,arrayLayerCount:1});
          const pass=encoder.beginRenderPass({colorAttachments:[{view,loadOp:full?'clear':'load',storeOp:'store',clearValue:value}]});
          if(!full){pass.setPipeline(pipeline);pass.setBindGroup(0,group);pass.setScissorRect(...rect);pass.draw(3);}
          pass.end();
        }
        if(this.closed)throw new Error('Render backend closed before clear submission');
        this.device.queue.submit([encoder.finish()]);
      });
      this.statistics.colorClearSubmissions++;this.statistics.colorClears+=commands.length;
      this.statistics.maskedColorClears+=commands.filter(command=>!command.full).length;
    }finally{for(const buffer of buffers)buffer.destroy();}
  }
  async clearDepthStencil(bytes) {
    if(this.closed)throw new Error('Render backend closed');
    if(!(bytes instanceof Uint8Array)||bytes.length<4||bytes.length>4+4096*48)throw new Error('Invalid depth/stencil clear packet');
    const data=new DataView(bytes.buffer,bytes.byteOffset,bytes.byteLength),count=data.getUint32(0,true);
    if(count>4096||bytes.length!==4+count*48)throw new Error('Invalid depth/stencil clear packet length');
    const commands=[];
    for(let i=0;i<count;i++) {
      const at=4+i*48,get=off=>data.getUint32(at+off,true);
      const record=this.textures.get(get(0)),mip=get(4),layer=get(8),writeDepth=get(12),depth=data.getFloat32(at+16,true),stencil=get(20),mask=get(24);
      if(record.dimension!=='2d'||(!record.format.depth&&!record.format.stencil)||get(44)!==record.nvn||mip>=record.levels||layer>=record.layers)
        throw new Error('Depth/stencil clear requires a matching 2D subresource');
      if(writeDepth>1||!Number.isFinite(depth)||depth<0||depth>1||stencil>255||mask>255)throw new Error('Invalid depth/stencil clear value or mask');
      const width=Math.max(1,record.width>>mip),height=Math.max(1,record.height>>mip),rect=[get(28),get(32),get(36),get(40)];
      if(rect[0]>width||rect[1]>height||rect[2]>width-rect[0]||rect[3]>height-rect[1])throw new Error('Depth/stencil clear scissor exceeds the target');
      const depthMask=!!writeDepth&&!!record.format.depth,stencilMask=record.format.stencil?mask:0;
      if((!depthMask&&!stencilMask)||!rect[2]||!rect[3])continue;
      const full=rect[0]===0&&rect[1]===0&&rect[2]===width&&rect[3]===height&&(!stencilMask||stencilMask===255);
      commands.push({record,mip,layer,depthMask,depth,stencil,stencilMask,rect,full});
    }
    const buffers=[];
    try {
      await this.checked(async()=>{
        const encoder=this.device.createCommandEncoder();
        for(const {record,mip,layer,depthMask,depth,stencil,stencilMask,rect,full} of commands) {
          let pipeline,group;
          if(!full) {
            const format=record.format.format,key=`depth/${format}/${depthMask}/${stencilMask}`;
            pipeline=this.pipelines.get(key);
            if(!pipeline) {
              const shader=this.device.createShaderModule({code:`
                @group(0) @binding(0) var<uniform> clearDepth:vec4<f32>;
                @vertex fn clear_vertex(@builtin(vertex_index) i:u32)->@builtin(position) vec4<f32> {
                  let p=array<vec2<f32>,3>(vec2<f32>(-1,-1),vec2<f32>(3,-1),vec2<f32>(-1,3));
                  return vec4<f32>(p[i],0,1);
                }
                ${record.format.depth?'@fragment fn clear_fragment()->@builtin(frag_depth) f32 {return clearDepth.x;}':'@fragment fn clear_fragment() {}'}
              `});
              const ds={format};
              if(record.format.depth){ds.depthWriteEnabled=depthMask;ds.depthCompare='always';}
              if(record.format.stencil){
                ds.stencilFront=ds.stencilBack={compare:'always',failOp:'keep',depthFailOp:'keep',passOp:'replace'};
                ds.stencilReadMask=255;ds.stencilWriteMask=stencilMask;
              }
              pipeline=await this.device.createRenderPipelineAsync({layout:'auto',vertex:{module:shader,entryPoint:'clear_vertex'},
                fragment:{module:shader,entryPoint:'clear_fragment',targets:[]},primitive:{topology:'triangle-list'},depthStencil:ds});
              if(this.pipelines.size>=256)this.pipelines.delete(this.pipelines.keys().next().value);
              this.pipelines.set(key,pipeline);this.statistics.depthClearPipelines++;
            }
            if(record.format.depth){
              const buffer=this.device.createBuffer({size:16,usage:GPUBufferUsage.UNIFORM|GPUBufferUsage.COPY_DST});buffers.push(buffer);
              this.device.queue.writeBuffer(buffer,0,new Float32Array([depth,0,0,0]));
              group=this.device.createBindGroup({layout:pipeline.getBindGroupLayout(0),entries:[{binding:0,resource:{buffer}}]});
            }
          }
          if(this.closed)throw new Error('Render backend closed during depth/stencil clear');
          const attachment={view:record.texture.createView({dimension:'2d',baseMipLevel:mip,mipLevelCount:1,baseArrayLayer:layer,arrayLayerCount:1})};
          if(record.format.depth)Object.assign(attachment,{depthLoadOp:full&&depthMask?'clear':'load',depthStoreOp:'store',depthClearValue:depth});
          if(record.format.stencil)Object.assign(attachment,{stencilLoadOp:full&&stencilMask===255?'clear':'load',stencilStoreOp:'store',stencilClearValue:stencil});
          const pass=encoder.beginRenderPass({colorAttachments:[],depthStencilAttachment:attachment});
          if(!full){pass.setPipeline(pipeline);if(group)pass.setBindGroup(0,group);pass.setStencilReference(stencil);pass.setScissorRect(...rect);pass.draw(3);}
          pass.end();
        }
        this.device.queue.submit([encoder.finish()]);
      });
      this.statistics.depthClearSubmissions++;this.statistics.depthStencilClears+=commands.length;
      this.statistics.maskedDepthStencilClears+=commands.filter(command=>!command.full).length;
    }finally{for(const buffer of buffers)buffer.destroy();}
  }
  close(){this.closed=true;this.pipelines.clear();this.presentCrop?.destroy();this.presentContext?.unconfigure();}
}
