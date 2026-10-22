/** Decode the frontend's 48-byte sampler value, not an NVN hardware descriptor.
 * Unsupported sampling modes need shader lowering; never silently substitute
 * edge clamp, zero bias, weighted reduction or a different anisotropy value.
 * WebGPU descriptor reference: https://gpuweb.github.io/types/interfaces/GPUSamplerDescriptor.html
 */
export function samplerDescriptor(bytes,{mipLevelCount}={}) {
  if(!(bytes instanceof Uint8Array)||bytes.byteLength!==48)throw new Error('Sampler value must contain exactly 48 bytes');
  if(!Number.isInteger(mipLevelCount)||mipLevelCount<1||mipLevelCount>32)throw new Error('Sampler requires the bound texture mip count');
  const v=new DataView(bytes.buffer,bytes.byteOffset,48);
  const min=v.getUint8(0),mag=v.getUint8(1),cmode=v.getUint8(5),cfunc=v.getUint8(6),reduction=v.getUint8(7);
  const nativeMinLod=v.getFloat32(8,true),nativeMaxLod=v.getFloat32(12,true),bias=v.getFloat32(16,true);
  const maxAnisotropy=v.getFloat32(20,true),snap=v.getFloat32(44,true);
  if(min>5||mag>1||cmode>1||(cmode&&(cfunc<1||cfunc>8)))throw new Error('Invalid sampler filter/compare enum');
  if(!Number.isFinite(nativeMinLod)||!Number.isFinite(nativeMaxLod)||nativeMaxLod<0||nativeMaxLod<nativeMinLod)
    throw new Error('Sampler LOD range requires unsupported clamp semantics');
  // A negative lower clamp does not exclude any positive LOD; nonpositive
  // values still select the base mip and magnification. Negative upper clamps
  // can instead force magnification for positive input LODs and need separate
  // filter lowering, so do not normalize those here.
  const lodMinClamp=Math.max(0,nativeMinLod),lodMaxClamp=nativeMaxLod;
  if(bias!==0)throw new Error('Sampler LOD bias requires shader lowering');
  if(snap!==0)throw new Error('Sampler LOD snap requires shader lowering');
  if(reduction!==0)throw new Error('Sampler min/max reduction requires shader lowering');
  if(!Number.isInteger(maxAnisotropy)||maxAnisotropy<1||maxAnisotropy>16)throw new Error('Sampler anisotropy is outside the exact WebGPU range');
  // A non-mip filter cannot be represented by merely setting lodMaxClamp=0:
  // that can change minification/magnification selection. A one-level texture
  // has no extra levels to select; multi-level textures need shader support.
  if(min<2&&mipLevelCount!==1)throw new Error('Non-mip sampler on a multi-level texture requires shader lowering');
  const modes={1:'repeat',6:'mirror-repeat',7:'clamp-to-edge'};
  const addressModes=[2,3,4].map(offset=>{
    const mode=modes[v.getUint8(offset)];
    if(!mode)throw new Error('Sampler border/mirror-clamp mode requires shader lowering');
    return mode;
  });
  const minFilter=min%2?'linear':'nearest',magFilter=mag?'linear':'nearest';
  const mipmapFilter=min>=4||(min<2&&maxAnisotropy>1)?'linear':'nearest';
  if(maxAnisotropy>1&&(minFilter!=='linear'||magFilter!=='linear'||mipmapFilter!=='linear'))
    throw new Error('Sampler anisotropy with non-linear filters requires shader lowering');
  const descriptor={addressModeU:addressModes[0],addressModeV:addressModes[1],addressModeW:addressModes[2],
    minFilter,magFilter,mipmapFilter,lodMinClamp,lodMaxClamp,maxAnisotropy};
  if(cmode)descriptor.compare=['never','less','equal','less-equal','greater','not-equal','greater-equal','always'][cfunc-1];
  return descriptor;
}

/** Immutable device samplers cached by effective descriptor, with a bounded
 * LRU. Dropping a cache entry does not invalidate samplers in live bind groups.
 * The owner serializes checked() calls with other WebGPU error scopes.
 */
export class WebGpuSamplers {
  constructor(device,checked,{capacity=512}={}) {
    if(!Number.isInteger(capacity)||capacity<1||capacity>4096)throw new Error('Invalid sampler cache capacity');
    this.device=device;this.checked=checked;this.capacity=capacity;this.cache=new Map();this.closed=false;
    this.statistics={samplersCreated:0,samplerCacheHits:0,samplerCacheEvictions:0,cachedSamplers:0};
  }
  async get(bytes,context) {
    if(this.closed)throw new Error('Sampler backend closed');
    const descriptor=samplerDescriptor(bytes,context),key=JSON.stringify(descriptor);
    let promise=this.cache.get(key);
    if(promise){this.cache.delete(key);this.cache.set(key,promise);this.statistics.samplerCacheHits++;return promise;}
    promise=(async()=>{
      const sampler=await this.checked(()=>this.device.createSampler(descriptor));
      if(this.closed)throw new Error('Sampler backend closed');
      this.statistics.samplersCreated++;return sampler;
    })();
    this.cache.set(key,promise);
    if(this.cache.size>this.capacity){this.cache.delete(this.cache.keys().next().value);this.statistics.samplerCacheEvictions++;}
    this.statistics.cachedSamplers=this.cache.size;
    try{return await promise;}
    catch(error){if(this.cache.get(key)===promise)this.cache.delete(key);this.statistics.cachedSamplers=this.cache.size;throw error;}
  }
  close(){this.closed=true;this.cache.clear();this.statistics.cachedSamplers=0;}
}
