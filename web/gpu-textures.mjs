import {WebGpuDepth} from './gpu-depth.mjs';
/** NVN format -> WebGPU storage. Missing formats fail explicitly; integer and
 * floating data is never clamped through a preview RGBA8 conversion. */
export function textureFormat(nvn) {
  if(nvn>=0x31&&nvn<=0x36){
    const formats=['stencil8','depth16unorm','depth24plus','depth32float','depth24plus-stencil8','depth32float-stencil8'];
    const bpp=[1,2,4,4,4,8][nvn-0x31];return {format:formats[nvn-0x31],bw:1,bh:1,bpb:bpp,sourceBpb:bpp,
      depth:nvn!==0x31,stencil:[0x31,0x35,0x36].includes(nvn),renderable:true,feature:nvn===0x36?'depth32float-stencil8':null};
  }
  if((nvn>=1&&nvn<=0x30)||(nvn>=0x6d&&nvn<=0x78)) {
    const channels=nvn>=0x6d?4:Math.floor((nvn-1)/12)+1,index=nvn>=0x6d?nvn-0x6d:(nvn-1)%12;
    const suffix=['8unorm','8snorm','8uint','8sint','16float','16unorm','16snorm','16uint','16sint','32float','32uint','32sint'][index];
    const bytes=[1,1,1,1,2,2,2,2,2,4,4,4][index],expand=channels===3;
    const format=(channels===1?'r':channels===2?'rg':'rgba')+suffix;
    const alpha=[255,127,1,1,0x3c00,65535,32767,1,1,0x3f800000,1,1][index];
    return {format,bw:1,bh:1,bpb:bytes*(expand?4:channels),sourceBpb:bytes*channels,expand,componentBytes:bytes,alpha,
      feature:index===5||index===6?'texture-formats-tier1':null,renderable:true,
      renderableFeature:[1,5,6].includes(index)?'texture-formats-tier1':null};
  }
  const special={0x37:'rgba8unorm-srgb',0x38:'rgba8unorm-srgb',0x3d:'rgb10a2unorm',0x3e:'rgb10a2uint',
    0x3f:'rg11b10ufloat',0x40:'rgb9e5ufloat',0x99:'bgra8unorm',0x9a:'bgra8unorm',0x9b:'bgra8unorm-srgb',0x9c:'bgra8unorm-srgb'};
  if(special[nvn])return {format:special[nvn],bw:1,bh:1,bpb:4,sourceBpb:4,renderable:nvn!==0x40,
    renderableFeature:nvn===0x3f?'rg11b10ufloat-renderable':null};
  if((nvn>=0x39&&nvn<=0x3c)||(nvn>=0x95&&nvn<=0x98))
    return {format:'rgba8unorm',bw:1,bh:1,bpb:4,sourceBpb:2,packed16:nvn,renderable:true};
  if(nvn>=0x41&&nvn<=0x50) {
    const formats=['bc1-rgba-unorm','bc1-rgba-unorm','bc2-rgba-unorm','bc3-rgba-unorm',
      'bc1-rgba-unorm-srgb','bc1-rgba-unorm-srgb','bc2-rgba-unorm-srgb','bc3-rgba-unorm-srgb',
      'bc4-r-unorm','bc4-r-snorm','bc5-rg-unorm','bc5-rg-snorm','bc7-rgba-unorm','bc7-rgba-unorm-srgb','bc6h-rgb-float','bc6h-rgb-ufloat'];
    const bpb=[0x41,0x42,0x45,0x46,0x49,0x4a].includes(nvn)?8:16;
    return {format:formats[nvn-0x41],bw:4,bh:4,bpb,sourceBpb:bpb,feature:'texture-compression-bc',alphaOne:nvn===0x41||nvn===0x45};
  }
  if(nvn>=0x79&&nvn<=0x94) {
    const footprints=[[4,4],[5,4],[5,5],[6,5],[6,6],[8,5],[8,6],[8,8],[10,5],[10,6],[10,8],[10,10],[12,10],[12,12]];
    const [bw,bh]=footprints[(nvn-0x79)%14];
    return {format:`astc-${bw}x${bh}-unorm${nvn>=0x87?'-srgb':''}`,bw,bh,bpb:16,sourceBpb:16,feature:'texture-compression-astc'};
  }
  throw new Error(`Unsupported NVN texture format 0x${nvn.toString(16)}`);
}

function expandRgb(bytes,format) {
  if(format.packed16) {
    const count=bytes.length>>>1,output=new Uint8Array(count*4),nvn=format.packed16;
    const rep4=x=>(x&15)*17,rep5=x=>((x&31)<<3)|((x&31)>>2),rep6=x=>((x&63)<<2)|((x&63)>>4);
    for(let i=0;i<count;i++) {
      const v=bytes[i*2]|(bytes[i*2+1]<<8),o=i*4;
      if(nvn===0x39){output[o]=rep4(v>>12);output[o+1]=rep4(v>>8);output[o+2]=rep4(v>>4);output[o+3]=rep4(v);}
      else if(nvn===0x3a||nvn===0x3b){output[o]=rep5(v>>11);output[o+1]=rep5(v>>6);output[o+2]=rep5(v>>1);output[o+3]=nvn===0x3b?((v&1)?255:0):255;}
      else if(nvn===0x3c){output[o]=rep5(v>>11);output[o+1]=rep6(v>>5);output[o+2]=rep5(v);output[o+3]=255;}
      else if(nvn===0x95){output[o]=rep5(v);output[o+1]=rep6(v>>5);output[o+2]=rep5(v>>11);output[o+3]=255;}
      else if(nvn===0x96||nvn===0x97){output[o]=rep5(v>>1);output[o+1]=rep5(v>>6);output[o+2]=rep5(v>>11);output[o+3]=nvn===0x97?((v&1)?255:0):255;}
      else{output[o]=rep5(v);output[o+1]=rep5(v>>5);output[o+2]=rep5(v>>10);output[o+3]=(v>>15)?255:0;}
    }
    return output;
  }
  if(!format.expand)return bytes;
  const count=bytes.length/format.sourceBpb,output=new Uint8Array(count*format.bpb),view=new DataView(output.buffer);
  for(let i=0;i<count;i++) {
    output.set(bytes.subarray(i*format.sourceBpb,(i+1)*format.sourceBpb),i*format.bpb);
    const offset=i*format.bpb+format.sourceBpb;
    if(format.componentBytes===1)view.setUint8(offset,format.alpha);
    else if(format.componentBytes===2)view.setUint16(offset,format.alpha,true);
    else view.setUint32(offset,format.alpha,true);
  }
  return output;
}

function padCompressedBlocks(bytes,wb,hb,d,gpuWb,gpuHb,bpb) {
  if(wb===gpuWb&&hb===gpuHb)return bytes;
  const out=new Uint8Array(gpuWb*gpuHb*d*bpb),srcRow=wb*bpb,dstRow=gpuWb*bpb;
  for(let z=0;z<d;z++)for(let y=0;y<gpuHb;y++) {
    const sy=Math.min(y,hb-1),srcOff=(z*hb+sy)*srcRow,dstOff=(z*gpuHb+y)*dstRow;
    out.set(bytes.subarray(srcOff,srcOff+srcRow),dstOff);
    const last=bytes.subarray(srcOff+(wb-1)*bpb,srcOff+srcRow);
    for(let x=wb;x<gpuWb;x++)out.set(last,dstOff+x*bpb);
  }
  return out;
}

export function parseTexturePacket(bytes,device) {
  if(!(bytes instanceof Uint8Array)||bytes.length<36||bytes.length>128*1024*1024)throw new Error('Invalid texture packet');
  const view=new DataView(bytes.buffer,bytes.byteOffset,bytes.byteLength),get=offset=>view.getUint32(offset,true);
  if(get(0)!==0x5854564e)throw new Error('Invalid texture packet magic');
  const nvn=get(4),target=get(8),width=get(12),height=get(16),depth=get(20),levels=get(24),samples=get(28),count=get(32);
  const format=textureFormat(nvn),dimension=target===0?'1d':target===2?'3d':'2d';
  if(![0,1,2,3,4,7,8,9].includes(target)||samples>1)throw new Error('Unsupported texture target/multisampling');
  const layers=target===8?6:[3,4,9].includes(target)?depth:1,is3d=dimension==='3d';
  const limit=dimension==='1d'?device.limits.maxTextureDimension1D:is3d?device.limits.maxTextureDimension3D:device.limits.maxTextureDimension2D;
  const gpuWidth=Math.ceil(width/format.bw)*format.bw,gpuHeight=Math.ceil(height/format.bh)*format.bh;
  if(!width||!height||!depth||gpuWidth>limit||gpuHeight>limit||(is3d?depth>limit:layers>device.limits.maxTextureArrayLayers)||
     (dimension==='1d'&&(height!==1||depth!==1))||!levels||levels>Math.floor(Math.log2(Math.max(width,height,is3d?depth:1)))+1||count!==layers*levels)
    throw new Error('Texture dimensions/mips exceed device limits');
  if(format.feature&&!device.features.has(format.feature))throw new Error(`Texture format requires ${format.feature}`);
  if((format.depth||format.stencil)&&dimension!=='2d')throw new Error('Depth/stencil storage requires a 2D texture');
  if(format.bw>1&&is3d)throw new Error('Compressed 3D texture needs emulation');
  const subresources=[],seen=new Set();let offset=36;
  for(let i=0;i<count;i++) {
    if(offset+24>bytes.length)throw new Error('Truncated texture subresource');
    const mip=get(offset),layer=get(offset+4),w=get(offset+8),h=get(offset+12),d=get(offset+16),length=get(offset+20);offset+=24;
    const wb=Math.ceil(w/format.bw),hb=Math.ceil(h/format.bh),key=`${mip}/${layer}`;
    const gpuWb=Math.ceil(Math.max(1,gpuWidth>>mip)/format.bw),gpuHb=Math.ceil(Math.max(1,gpuHeight>>mip)/format.bh);
    if(mip>=levels||layer>=layers||seen.has(key)||w!==Math.max(1,width>>mip)||h!==Math.max(1,height>>mip)||
       d!==(is3d?Math.max(1,depth>>mip):1)||length!==wb*hb*d*format.sourceBpb||length>bytes.length-offset)
      throw new Error('Texture subresource dimensions/length mismatch');
    seen.add(key);
    const expanded=expandRgb(bytes.slice(offset,offset+length),format);
    subresources.push({mip,layer,w,h,d,wb:gpuWb,hb:gpuHb,bytes:padCompressedBlocks(expanded,wb,hb,d,gpuWb,gpuHb,format.bpb)});offset+=length;
  }
  if(offset!==bytes.length)throw new Error('Trailing texture packet bytes');
  return {nvn,target,width,height,gpuWidth,gpuHeight,depth,layers,levels,dimension,format,subresources};
}

export class WebGpuTextures {
  constructor(device,checked) {this.device=device;this.checked=checked;this.records=new Map();this.nextHandle=1;this.closed=false;this.depth=new WebGpuDepth(device,checked);
    this.views=new Map();this.nextView=1;
    this.statistics={texturesCreated:0,texturesDestroyed:0,liveTextures:0,textureWrites:0,textureBytesWritten:0,textureReads:0,
      commandSubmissions:0,bufferTextureCopies:0,textureViewsCreated:0,textureViewsReleased:0,liveTextureViews:0,...this.depth.statistics};}
  get(handle){const record=this.records.get(handle);if(!record)throw new Error('Invalid or stale GPU texture handle');return record;}
  async create(bytes) {
    if(this.closed)throw new Error('Texture backend closed');
    const record=parseTexturePacket(bytes,this.device);let texture;
    try {
      await this.checked(async()=> {
        let usage=GPUTextureUsage.COPY_SRC|GPUTextureUsage.COPY_DST|GPUTextureUsage.TEXTURE_BINDING;
        const renderable=record.format.renderable&&(!record.format.renderableFeature||this.device.features.has(record.format.renderableFeature));
        if(record.dimension!=='1d'&&renderable)usage|=GPUTextureUsage.RENDER_ATTACHMENT;
        const base=record.format.format,srgbPair=base.endsWith('-srgb')?base.slice(0,-5):
          (base==='rgba8unorm'||base==='bgra8unorm'||/^bc[1237].*-unorm$/.test(base))?base+'-srgb':null;
        texture=this.device.createTexture({size:[record.gpuWidth,record.gpuHeight,record.dimension==='3d'?record.depth:record.layers],
          mipLevelCount:record.levels,sampleCount:1,dimension:record.dimension,format:base,viewFormats:srgbPair?[srgbPair]:[],usage});
        for(const item of record.subresources)if(record.format.depth||record.format.stencil)await this.depth.upload(record,texture,item);
        else this.device.queue.writeTexture({texture,mipLevel:item.mip,origin:[0,0,item.layer]},item.bytes,
            {bytesPerRow:item.wb*record.format.bpb,rowsPerImage:item.hb},[item.wb*record.format.bw,item.hb*record.format.bh,item.d]);
      });
      if(this.closed)throw new Error('Texture backend closed during creation');
      if(this.nextHandle>0x7fffffff)throw new Error('GPU texture handle space exhausted');
      const handle=this.nextHandle++;record.texture=texture;
      this.statistics.textureWrites+=record.subresources.length;
      this.statistics.textureBytesWritten+=record.subresources.reduce((sum,item)=>sum+item.bytes.length,0);
      // GPU owns the texels after writeTexture; do not retain duplicate images.
      for(const item of record.subresources)delete item.bytes;
      Object.assign(this.statistics,this.depth.statistics);
      this.records.set(handle,record);this.statistics.texturesCreated++;this.statistics.liveTextures++;return handle;
    } catch(error){texture?.destroy();throw error;}
  }
  destroy(handle){const record=this.get(handle);record.texture.destroy();this.records.delete(handle);this.statistics.texturesDestroyed++;this.statistics.liveTextures--;}
  async createView(handle,bytes) {
    if(this.closed)throw new Error('Texture backend closed');
    const record=this.get(handle);
    if(!(bytes instanceof Uint8Array)||bytes.length!==18)throw new Error('Invalid native texture view');
    const data=new DataView(bytes.buffer,bytes.byteOffset,bytes.byteLength),set=data.getUint8(0);
    const target=(set&0xa0)===0xa0?data.getUint8(1):record.target;
    const dimensions={0:'1d',1:'2d',2:'3d',3:'2d-array',4:'2d-array',7:'2d',8:'cube',9:'cube-array'},dimension=dimensions[target];
    if(!dimension)throw new Error('Unsupported texture view target');
    const baseMipLevel=(set&0x81)===0x81?data.getInt16(6,true):0,nlvl=data.getInt16(8,true);
    const mipLevelCount=(set&0x81)===0x81&&nlvl>0?nlvl:record.levels-baseMipLevel;
    const baseArrayLayer=(set&0x82)===0x82?data.getInt16(10,true):0,nlay=data.getInt16(12,true);
    const arrayLayerCount=(set&0x82)===0x82&&nlay>0?nlay:dimension==='2d'?1:record.layers-baseArrayLayer;
    const format=(set&0x84)===0x84?textureFormat(data.getUint16(4,true)).format:record.format.format;
    if(baseMipLevel<0||baseMipLevel>=record.levels||mipLevelCount<1||mipLevelCount>record.levels-baseMipLevel||baseArrayLayer<0||
       baseArrayLayer>=record.layers||arrayLayerCount<1||arrayLayerCount>record.layers-baseArrayLayer)
      throw new Error('Texture view mip/layer range exceeds storage');
    let aspect='all',viewFormat=format;
    if(record.format.depth&&record.format.stencil){
      const stencil=(set&0x90)===0x90&&data.getUint8(2)===1;aspect=stencil?'stencil-only':'depth-only';
      viewFormat=stencil?'stencil8':record.nvn===0x35?'depth24plus':'depth32float';
      if((set&0x84)===0x84&&format!==record.format.format&&format!==viewFormat)throw new Error('Unsupported depth/stencil view format');
    }
    const descriptor={format:viewFormat,dimension,baseMipLevel,mipLevelCount,aspect};
    if(dimension==='2d'||dimension==='2d-array'||dimension==='cube'||dimension==='cube-array')Object.assign(descriptor,{baseArrayLayer,arrayLayerCount});
    else if(baseArrayLayer)throw new Error('Non-array texture view has a layer override');
    let view;await this.checked(()=>{view=record.texture.createView(descriptor);});
    if(this.closed)throw new Error('Texture backend closed during view creation');
    if(this.nextView>0x7fffffff)throw new Error('GPU texture view handle space exhausted');
    const id=this.nextView++;this.views.set(id,{view,texture:handle,descriptor,native:bytes.slice()});
    this.statistics.textureViewsCreated++;this.statistics.liveTextureViews++;return id;
  }
  releaseView(handle){if(!this.views.delete(handle))throw new Error('Invalid GPU texture view handle');this.statistics.textureViewsReleased++;this.statistics.liveTextureViews--;}
  getView(handle){const record=this.views.get(handle);if(!record)throw new Error('Invalid GPU texture view handle');this.get(record.texture);return record;}
  async submitCopies(bytes) {
    if(this.closed)throw new Error('Texture backend closed');
    if(!(bytes instanceof Uint8Array)||bytes.length<4||bytes.length>128*1024*1024)throw new Error('Invalid copy submission packet');
    const view=new DataView(bytes.buffer,bytes.byteOffset,bytes.byteLength),get=offset=>view.getUint32(offset,true),count=get(0);
    if(count>65536)throw new Error('Too many submitted copies');
    const copies=[];let offset=4;
    for(let i=0;i<count;i++) {
      if(offset+36>bytes.length)throw new Error('Truncated copy submission header');
      const handle=get(offset),mip=get(offset+4),x=get(offset+8),y=get(offset+12),z=get(offset+16),w=get(offset+20),h=get(offset+24),d=get(offset+28),length=get(offset+32);
      offset+=36;const record=this.get(handle),format=record.format;
      const mw=Math.max(1,record.width>>mip),mh=Math.max(1,record.height>>mip),md=record.dimension==='3d'?Math.max(1,record.depth>>mip):record.layers;
      const wb=Math.ceil(w/format.bw),hb=Math.ceil(h/format.bh);
      if(mip>=record.levels||!w||!h||!d||x>mw||w>mw-x||y>mh||h>mh-y||z>md||d>md-z||
         x%format.bw||y%format.bh||(w%format.bw&&x+w!==mw)||(h%format.bh&&y+h!==mh)||
         length!==wb*hb*d*format.sourceBpb||length>bytes.length-offset)
        throw new Error('Copy submission region/length mismatch');
      let source=expandRgb(bytes.slice(offset,offset+length),format);
      if(format.depth||format.stencil)this.depth.validate(record.nvn,source);
      let copyWb=wb,copyHb=hb;
      if(format.bw>1&&x===0&&y===0&&w===mw&&h===mh) {
        copyWb=Math.ceil(Math.max(1,record.gpuWidth>>mip)/format.bw);
        copyHb=Math.ceil(Math.max(1,record.gpuHeight>>mip)/format.bh);
        source=padCompressedBlocks(source,wb,hb,d,copyWb,copyHb,format.bpb);
      }
      copies.push({record,mip,x,y,z,w:copyWb*format.bw,h:copyHb*format.bh,d,wb:copyWb,hb:copyHb,bytes:source});offset+=length;
    }
    if(offset!==bytes.length)throw new Error('Trailing copy submission bytes');
    // Validate the entire batch before making any writes. Queue ordering is
    // preserved by WebGpuBackend, including the final empty GPU submission.
    await this.checked(async()=> {
      for(const copy of copies)if(copy.record.format.depth||copy.record.format.stencil){
        const size=copy.w*copy.h*copy.record.format.sourceBpb;
        for(let layer=0;layer<copy.d;layer++)await this.depth.upload(copy.record,copy.record.texture,{mip:copy.mip,layer:copy.z+layer,w:copy.w,h:copy.h,x:copy.x,y:copy.y,bytes:copy.bytes.subarray(layer*size,(layer+1)*size)});
      }else this.device.queue.writeTexture({texture:copy.record.texture,mipLevel:copy.mip,origin:[copy.x,copy.y,copy.z]},copy.bytes,
          {bytesPerRow:copy.wb*copy.record.format.bpb,rowsPerImage:copy.hb},[copy.w,copy.h,copy.d]);
      this.device.queue.submit([]);
    });
    this.statistics.commandSubmissions++;this.statistics.bufferTextureCopies+=copies.length;
    this.statistics.textureWrites+=copies.length;this.statistics.textureBytesWritten+=copies.reduce((sum,copy)=>sum+copy.bytes.length,0);
    Object.assign(this.statistics,this.depth.statistics);
  }
  async read(handle,mip=0,layer=0) {
    const record=this.get(handle),item=record.subresources.find(item=>item.mip===mip&&item.layer===layer);
    if(!item)throw new Error('Invalid texture subresource');
    if(record.format.depth||record.format.stencil){const bytes=await this.depth.read(record,item,layer);this.statistics.textureReads++;Object.assign(this.statistics,this.depth.statistics);return bytes;}
    const row=item.wb*record.format.bpb,pitch=Math.ceil(row/256)*256,size=pitch*item.hb*item.d;
    const buffer=this.device.createBuffer({size,usage:GPUBufferUsage.COPY_DST|GPUBufferUsage.MAP_READ});
    try {
      await this.checked(()=> {
        const encoder=this.device.createCommandEncoder();
        encoder.copyTextureToBuffer({texture:record.texture,mipLevel:mip,origin:[0,0,layer]},
          {buffer,bytesPerRow:pitch,rowsPerImage:item.hb},[item.wb*record.format.bw,item.hb*record.format.bh,item.d]);
        this.device.queue.submit([encoder.finish()]);
      });
      await buffer.mapAsync(GPUMapMode.READ);const source=new Uint8Array(buffer.getMappedRange()),bytes=new Uint8Array(row*item.hb*item.d);
      for(let i=0;i<item.hb*item.d;i++)bytes.set(source.subarray(i*pitch,i*pitch+row),i*row);
      buffer.unmap();this.statistics.textureReads++;return bytes;
    } finally {buffer.destroy();}
  }
  close(){this.closed=true;this.depth.close();this.views.clear();this.statistics.liveTextureViews=0;for(const {texture} of this.records.values())texture.destroy();this.records.clear();this.statistics.liveTextures=0;}
}
