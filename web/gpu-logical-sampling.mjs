import {textureFormat} from './gpu-textures.mjs';

/** Decode padded compressed storage on GPU into exact logical mip extents.
 * Preserve decoded f32 texels (including signed/HDR/sRGB-linear values) and
 * keep the guest's shader coordinates, address modes and filtering unchanged. */
export class WebGpuLogicalSamples {
  constructor(backend){this.backend=backend;this.pipelines=new Map();this.statistics={logicalSamplingCopies:0,logicalSamplingSubresources:0};}
  async create(record,source) {
    const b=this.backend,d=b.device,v=source.descriptor;
    if(!d.features.has('float32-filterable'))throw new Error('Logical compressed sampling requires float32-filterable');
    if(record.dimension!=='2d'||!['2d','2d-array','cube','cube-array'].includes(v.dimension)||record.format.bw<=1)
      throw new Error('Unsupported logical compressed sampling view');
    const native=source.native,viewFormat=native&&(native[0]&0x84)===0x84?new DataView(native.buffer,native.byteOffset,native.byteLength).getUint16(4,true):record.nvn;
    const alphaOne=!!textureFormat(viewFormat).alphaOne;
    let pipeline=this.pipelines.get(alphaOne);
    if(!pipeline){
      const code=`@group(0) @binding(0) var image:texture_2d<f32>;
        @vertex fn vs(@builtin(vertex_index) i:u32)->@builtin(position) vec4<f32>{let p=array<vec2<f32>,3>(vec2<f32>(-1,-1),vec2<f32>(3,-1),vec2<f32>(-1,3));return vec4<f32>(p[i],0,1);}
        @fragment fn fs(@builtin(position) p:vec4<f32>)->@location(0) vec4<f32>{let c=textureLoad(image,vec2<i32>(p.xy),0);return ${alphaOne?'vec4<f32>(c.rgb,1)':'c'};}`;
      pipeline=await b.checked(()=>{const module=d.createShaderModule({code});return d.createRenderPipelineAsync({layout:'auto',vertex:{module,entryPoint:'vs'},fragment:{module,entryPoint:'fs',targets:[{format:'rgba32float'}]},primitive:{topology:'triangle-list'}});});
      this.pipelines.set(alphaOne,pipeline);
    }
    const width=Math.max(1,record.width>>v.baseMipLevel),height=Math.max(1,record.height>>v.baseMipLevel),layers=v.arrayLayerCount||1;
    let texture;
    try {
      texture=await b.checked(()=>d.createTexture({size:[width,height,layers],mipLevelCount:v.mipLevelCount,format:'rgba32float',usage:GPUTextureUsage.RENDER_ATTACHMENT|GPUTextureUsage.TEXTURE_BINDING|GPUTextureUsage.COPY_SRC}));
      const view=texture.createView({dimension:v.dimension,mipLevelCount:v.mipLevelCount,arrayLayerCount:layers});
      await b.checked(()=>{
        const encoder=d.createCommandEncoder();
        for(let layer=0;layer<layers;layer++)for(let mip=0;mip<v.mipLevelCount;mip++){
          const input=record.texture.createView({format:v.format,dimension:'2d',baseMipLevel:v.baseMipLevel+mip,mipLevelCount:1,baseArrayLayer:(v.baseArrayLayer||0)+layer,arrayLayerCount:1});
          const target=texture.createView({dimension:'2d',baseMipLevel:mip,mipLevelCount:1,baseArrayLayer:layer,arrayLayerCount:1});
          const group=d.createBindGroup({layout:pipeline.getBindGroupLayout(0),entries:[{binding:0,resource:input}]});
          const pass=encoder.beginRenderPass({colorAttachments:[{view:target,loadOp:'clear',storeOp:'store'}]});pass.setPipeline(pipeline);pass.setBindGroup(0,group);pass.draw(3);pass.end();
        }
        d.queue.submit([encoder.finish()]);
      });
      this.statistics.logicalSamplingCopies++;this.statistics.logicalSamplingSubresources+=layers*v.mipLevelCount;
      Object.assign(b.statistics,this.statistics);return {texture,view};
    }catch(error){texture?.destroy();throw error;}
  }
}
