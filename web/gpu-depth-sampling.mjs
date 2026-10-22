/** Preserve ordinary (non-shadow) depth sampling, including the guest's linear
 * filters, using a GPU-only, lossless f32 snapshot of each sampled subresource.
 * Comparison shaders continue to use the original depth view. */
export class WebGpuDepthSamples {
  constructor(backend){this.backend=backend;this.pipeline=null;this.statistics={depthSamplingCopies:0,depthSamplingSubresources:0};}
  async create(record,source) {
    const b=this.backend,d=b.device,v=source.descriptor;
    if(!d.features.has('float32-filterable'))throw new Error('Filtered ordinary depth sampling requires float32-filterable');
    if(record.dimension!=='2d'||v.aspect==='stencil-only')throw new Error('Unsupported ordinary depth sampling view');
    if(!this.pipeline){
      const code='@group(0) @binding(0) var image:texture_depth_2d; @vertex fn vs(@builtin(vertex_index) i:u32)->@builtin(position) vec4<f32>{let p=array<vec2<f32>,3>(vec2<f32>(-1,-1),vec2<f32>(3,-1),vec2<f32>(-1,3));return vec4<f32>(p[i],0,1);} @fragment fn fs(@builtin(position) p:vec4<f32>)->@location(0) f32{return textureLoad(image,vec2<i32>(p.xy),0);}';
      this.pipeline=await b.checked(()=>{const module=d.createShaderModule({code});return d.createRenderPipelineAsync({layout:'auto',vertex:{module,entryPoint:'vs'},fragment:{module,entryPoint:'fs',targets:[{format:'r32float'}]},primitive:{topology:'triangle-list'}});});
    }
    const width=Math.max(1,record.width>>v.baseMipLevel),height=Math.max(1,record.height>>v.baseMipLevel),layers=v.arrayLayerCount||1;
    let texture;
    try {
      texture=await b.checked(()=>d.createTexture({size:[width,height,layers],mipLevelCount:v.mipLevelCount,format:'r32float',usage:GPUTextureUsage.RENDER_ATTACHMENT|GPUTextureUsage.TEXTURE_BINDING}));
      const view=texture.createView({dimension:v.dimension,mipLevelCount:v.mipLevelCount,...(['2d','2d-array','cube','cube-array'].includes(v.dimension)?{arrayLayerCount:layers}:{})});
      await b.checked(()=>{
        const encoder=d.createCommandEncoder();
        for(let layer=0;layer<layers;layer++)for(let mip=0;mip<v.mipLevelCount;mip++){
          const depth=record.texture.createView({format:v.format,dimension:'2d',aspect:'depth-only',baseMipLevel:v.baseMipLevel+mip,mipLevelCount:1,baseArrayLayer:(v.baseArrayLayer||0)+layer,arrayLayerCount:1});
          const target=texture.createView({dimension:'2d',baseMipLevel:mip,mipLevelCount:1,baseArrayLayer:layer,arrayLayerCount:1});
          const group=d.createBindGroup({layout:this.pipeline.getBindGroupLayout(0),entries:[{binding:0,resource:depth}]});
          const pass=encoder.beginRenderPass({colorAttachments:[{view:target,loadOp:'clear',storeOp:'store'}]});pass.setPipeline(this.pipeline);pass.setBindGroup(0,group);pass.draw(3);pass.end();
        }
        d.queue.submit([encoder.finish()]);
      });
      this.statistics.depthSamplingCopies++;this.statistics.depthSamplingSubresources+=layers*v.mipLevelCount;
      Object.assign(b.statistics,this.statistics);
      return {texture,view};
    }catch(error){texture?.destroy();throw error;}
  }
}
