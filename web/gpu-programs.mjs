/** Immutable shader source snapshots. Compile variants only once the renderer
 * supplies topology and integer resource/attribute interpretation. */
export class WebGpuPrograms {
  constructor(device,checked,compilerFactory=async()=> (await import('./game-shaders.mjs')).createGameShaderCompiler(),{snapshotLimit=65536}={}) {
    if(!Number.isInteger(snapshotLimit)||snapshotLimit<1||snapshotLimit>1048576)throw new Error('Invalid program snapshot limit');
    this.device=device;this.checked=checked;this.compilerFactory=compilerFactory;
    this.records=new Map();this.nextHandle=1;this.compilations=0;
    this.snapshots=new Map();this.nextSnapshot=1;this.closed=false;
    this.snapshotLimit=snapshotLimit;
  }
  get(handle) {
    const record=this.records.get(handle);if(!record)throw new Error('Invalid or stale GPU program handle');return record;
  }
  register(handle,bytes) {
    if(this.closed)throw new Error('GPU program backend closed');
    if(handle)this.get(handle);
    if(!(bytes instanceof Uint8Array)||bytes.length<4||bytes.length>61*1024*1024)throw new Error('Invalid program source packet');
    const view=new DataView(bytes.buffer,bytes.byteOffset,bytes.byteLength),count=view.getUint32(0,true);
    if(!count||count>6)throw new Error('Invalid shader count');
    const stages=[];const types=new Set();let offset=4;
    for(let i=0;i<count;i++) {
      if(offset+8>bytes.length)throw new Error('Truncated shader packet header');
      const controlSize=view.getUint32(offset,true),dataSize=view.getUint32(offset+4,true);offset+=8;
      if(controlSize<0x720||controlSize>2*1024*1024||dataSize>8*1024*1024||controlSize+dataSize>bytes.length-offset)
        throw new Error('Shader packet sizes exceed storage');
      const control=bytes.slice(offset,offset+controlSize),data=bytes.slice(offset+controlSize,offset+controlSize+dataSize);
      const ctl=new DataView(control.buffer),code=ctl.getUint32(0x6f8,true),constant=ctl.getUint32(0x6fc,true),constantOffset=ctl.getUint32(0x700,true);
      const compute=ctl.getUint32(0x714,true)===5,prefix=compute?0:0x30;
      if(ctl.getUint32(0,true)!==0x98761234||code<32||code>4*1024*1024||prefix+code>dataSize||
         (constant&&(constantOffset>dataSize||constant>dataSize-constantOffset))||(compute&&controlSize<0x75c))
        throw new Error('Invalid shader code or constants');
      const type=compute?6:(new DataView(data.buffer).getUint32(prefix,true)>>>10)&15;
      if(!compute&&(new DataView(data.buffer).getUint32(0,true)!==0x12345678||type<1||type>5))throw new Error('Invalid Maxwell stage header');
      if(types.has(type))throw new Error('Duplicate shader stage');types.add(type);
      stages.push({control,data,type});offset+=controlSize+dataSize;
    }
    if(offset!==bytes.length)throw new Error('Trailing shader packet bytes');
    if(!handle){if(this.nextHandle>0x7fffffff)throw new Error('GPU program handle space exhausted');handle=this.nextHandle++;}
    this.records.set(handle,{stages,variants:new Map()});return handle;
  }
  release(handle) {this.get(handle);this.records.delete(handle);}
  retain(handle) {
    const record=this.get(handle);
    if(this.snapshots.size>=this.snapshotLimit||this.nextSnapshot>0x7fffffff)throw new Error('GPU program snapshot space exhausted');
    const snapshot=this.nextSnapshot++;this.snapshots.set(snapshot,record);return snapshot;
  }
  releaseSnapshot(snapshot) {
    if(!this.snapshots.delete(snapshot))throw new Error('Invalid or stale GPU program snapshot');
  }
  async compile(handle,options={}) {
    const record=this.get(handle);
    return this.compileRecord(record,options,()=>this.records.get(handle)===record);
  }
  async compileSnapshot(snapshot,options={}) {
    const record=this.snapshots.get(snapshot);
    if(!record)throw new Error('Invalid or stale GPU program snapshot');
    return this.compileRecord(record,options,()=>this.snapshots.get(snapshot)===record);
  }
  async compileRecord(record,options,valid) {
    if(this.closed||!valid())throw new Error('GPU program source released or replaced during compilation');
    const key=JSON.stringify(Object.fromEntries(Object.entries(options).sort(([a],[b])=>a.localeCompare(b))));
    if(record.variants.has(key))return record.variants.get(key);
    this.compiler??=await this.compilerFactory();const stages=[];
    for(const source of record.stages) {
      if(this.closed||!valid())throw new Error('GPU program source released or replaced during compilation');
      let translated;
      try {translated=this.compiler.compile(source.control,source.data,options);}
      catch(error){
        const base64=bytes=>{let text='';for(let offset=0;offset<bytes.length;offset+=8192)text+=String.fromCharCode(...bytes.subarray(offset,offset+8192));return btoa(text);};
        this.lastFailure={error:String(error),options,sourceType:source.type,
          ...(source.control.length+source.data.length<=512*1024?{control:base64(source.control),data:base64(source.data)}:{sourceOmitted:'Capture exceeds the bounded report budget'}),
          scope:'Actual retained Maxwell shader that failed compilation, encoded as base64; no synthetic replacement'};
        throw error;
      }
      let module;
      await this.checked(async()=> {
        module=this.device.createShaderModule({code:translated.code});
        const info=await module.getCompilationInfo();
        const errors=info.messages.filter(message=>message.type==='error');
        if(errors.length)throw new Error(errors.map(message=>message.message).join('\n'));
      });
      stages.push({...translated,module});
    }
    if(this.closed||!valid())throw new Error('GPU program source released or replaced during compilation');
    if(options.uniformWords&&record.variants.size>=256)record.variants.delete(record.variants.keys().next().value);
    record.variants.set(key,stages);this.compilations+=stages.length;return stages;
  }
  close(){this.closed=true;this.records.clear();this.snapshots.clear();}
}
