import {stageInterface} from './wgsl-interface.mjs';
import {textureFormat} from './gpu-textures.mjs';
import {WebGpuDepthSamples} from './gpu-depth-sampling.mjs';
import {WebGpuLogicalSamples} from './gpu-logical-sampling.mjs';

/** Native frontend value encodings from runtime/nvn_gl.c:vfmt_get. */
export function vertexFormat(nvn) {
  if(!Number.isInteger(nvn))throw new Error('Invalid NVN vertex format');
  if((nvn>=1&&nvn<=0x30)||(nvn>=0x6d&&nvn<=0x78)) {
    const count=nvn>=0x6d?4:Math.floor((nvn-1)/12)+1,index=nvn>=0x6d?nvn-0x6d:(nvn-1)%12;
    const names=['unorm8','snorm8','uint8','sint8','float16',null,null,'uint16','sint16','float32','uint32','sint32'];
    const integer=[2,3,7,8,10,11].includes(index),signed=[3,8,11].includes(index);
    const direct=names[index]&&(index>=9||count===2||count===4);
    return {format:direct?names[index]+(count===1?'':`x${count}`):null,integer,signed,count,index,bytes:count*[1,1,1,1,2,2,2,2,2,4,4,4][index]};
  }
  if(nvn>=0x51&&nvn<=0x68){const index=(nvn-0x51)%6,count=Math.floor((nvn-0x51)/6)+1;return {format:null,integer:false,signed:false,count,index,scaled:true,bytes:count*[1,1,2,2,4,4][index]};}
  if([0x3d,0x3e,0x69,0x6a,0x6b,0x6c].includes(nvn))return {format:nvn===0x3d?'unorm10-10-10-2':null,integer:nvn===0x3e||nvn===0x6a,signed:nvn===0x6a,count:4,packed:true,packedSigned:[0x69,0x6a,0x6c].includes(nvn),normalized:nvn===0x3d||nvn===0x69,bytes:4};
  throw new Error(`Unknown NVN vertex format 0x${nvn.toString(16)}`);
}

/** Expand a single native fetch to its shader-visible four components. Missing
 * channels are (0,0,0,1); signed normalized minima clamp exactly to -1. */
export function decodeVertex(data,offset,nvn,f=vertexFormat(nvn)) {
  const out=[0,0,0,1];
  if(f.packed){const bits=data.getUint32(offset,true);let shift=0;for(let i=0;i<4;i++){const width=i===3?2:10,mask=(1<<width)-1;let v=(bits>>>shift)&mask;if(f.packedSigned&&(v&(1<<(width-1))))v-=1<<width;out[i]=f.normalized?(f.packedSigned?Math.max(-1,v/((1<<(width-1))-1)):v/mask):v;shift+=width;}return out;}
  const sizes=f.scaled?[1,1,2,2,4,4]:[1,1,1,1,2,2,2,2,2,4,4,4],size=sizes[f.index];
  for(let i=0;i<f.count;i++){
    const at=offset+i*size;let v;
    if(f.scaled)v=[()=>data.getUint8(at),()=>data.getInt8(at),()=>data.getUint16(at,true),()=>data.getInt16(at,true),()=>data.getUint32(at,true),()=>data.getInt32(at,true)][f.index]();
    else if(f.index===4){const h=data.getUint16(at,true),sign=h&0x8000?-1:1,exp=(h>>>10)&31,mant=h&1023;v=exp===31?(mant?NaN:sign*Infinity):sign*(exp?1+mant/1024:mant/1024)*2**(exp?exp-15:-14);}
    else {v=[()=>data.getUint8(at)/255,()=>Math.max(-1,data.getInt8(at)/127),()=>data.getUint8(at),()=>data.getInt8(at),null,()=>data.getUint16(at,true)/65535,()=>Math.max(-1,data.getInt16(at,true)/32767),()=>data.getUint16(at,true),()=>data.getInt16(at,true),()=>data.getFloat32(at,true),()=>data.getUint32(at,true),()=>data.getInt32(at,true)][f.index]();}
    out[i]=v;
  }
  return out;
}
const compare=n=>{if(n<1||n>8)throw new Error('Invalid NVN comparison');return ['never','less','equal','less-equal','greater','not-equal','greater-equal','always'][n-1];};
const stencilOp=n=>{if(n<1||n>8)throw new Error('Invalid NVN stencil operation');return ['keep','zero','replace','increment-clamp','decrement-clamp','invert','increment-wrap','decrement-wrap'][n-1];};
const blendFactor=n=>{
  const value={1:'zero',2:'one',3:'src',4:'one-minus-src',5:'src-alpha',6:'one-minus-src-alpha',7:'dst-alpha',8:'one-minus-dst-alpha',9:'dst',10:'one-minus-dst',11:'src-alpha-saturated',97:'constant',98:'one-minus-constant'}[n];
  if(!value)throw new Error(`NVN blend factor ${n} needs lowering`);return value;
};
const blendOp=n=>{if(n<1||n>5)throw new Error('Invalid NVN blend equation');return ['add','subtract','reverse-subtract','min','max'][n-1];};
const uint=(n,label)=>{if(!Number.isSafeInteger(n)||n<0||n>0xffffffff)throw new Error(`Invalid ${label}`);return n;};
const array=(a,length,label)=>{if(!Array.isArray(a)||a.length!==length)throw new Error(`Invalid ${label}`);return a;};

/** NVN uses the last vertex for flat values; WebGPU uses the first. Cyclic
 * rotation preserves winding, and strip expansion preserves alternating faces.
 * Restart discards incomplete primitives and resets strip parity. */
export function triangleIndices(indices,mode,{flat=false,restart=null}={}) {
  if(![4,5].includes(mode))throw new Error('Unsupported triangle assembly mode');
  const capacity=mode===4?Math.floor(indices.length/3)*3:Math.max(0,indices.length-2)*3;
  if(capacity>16777216)throw new Error('Triangle conversion exceeds 64 MiB');
  const result=new Uint32Array(capacity);let used=0,a=0,b=0,vertices=0;
  const emit=(x,y,z)=>{if(flat){result[used++]=z;result[used++]=x;result[used++]=y;}else{result[used++]=x;result[used++]=y;result[used++]=z;}};
  for(const index of indices) {
    uint(index,'vertex index');
    if(restart!==null&&index===restart){vertices=0;continue;}
    if(mode===4){if(vertices===0)a=index;else if(vertices===1)b=index;else{emit(a,b,index);vertices=-1;}vertices++;}
    else{if(vertices===0)a=index;else if(vertices===1)b=index;else{if((vertices&1)===0)emit(a,b,index);else emit(b,a,index);a=b;b=index;}vertices++;}
  }
  return result.subarray(0,used);
}

/** One real draw from immutable frontend state. Queue order is provided by
 * WebGpuBackend.request; shaders always come from retained program snapshots. */
export class WebGpuDraw {
  constructor(backend){this.backend=backend;this.device=backend.device;this.cache=new Map();this.modules=new WeakMap();this.nextModule=1;this.closed=false;
    this.depthSamples=new WebGpuDepthSamples(backend);
    this.logicalSamples=new WebGpuLogicalSamples(backend);
    if(backend.render.frameDiagnostics){this.diagnosticDraws={records:[],byteLimit:196608,scope:'Bounded actual draw inputs; buffer prefixes may be incomplete and are explicitly marked. Not a complete draw replay or proof of game pixels'};this.diagnosticKeys=new Set();}
    this.statistics={drawSubmissions:0,draws:0,indexedDraws:0,graphicsPipelines:0,drawBufferCopies:0,convertedIndexDraws:0,convertedIndexBytes:0,convertedVertexStreams:0,convertedVertexBytes:0,arraySamplingViews:0,uniformSpecializedDraws:0,uniformBranchesRemoved:0,...this.logicalSamples.statistics};}
  moduleId(module){if(!this.modules.has(module))this.modules.set(module,this.nextModule++);return this.modules.get(module);}
  async captureInputs(key,packet,shaders,options,failure=null) {
    const diagnostic=this.diagnosticDraws;
    if(!diagnostic||(!failure&&(this.diagnosticKeys.has(key)||this.diagnosticKeys.size>=12)))return;
    this.diagnosticKeys.add(key);
    try {
      const encode=bytes=>{let text='';for(let at=0;at<bytes.length;at+=8192)text+=String.fromCharCode(...bytes.subarray(at,at+8192));return btoa(text);};
      const record={draw:this.statistics.draws,packet,shaders:[],buffers:[],...(failure?{failure:String(failure)}:{})};
      // Preserve the actual failing draw even after the ordinary capture window
      // fills. Evict older successes, keeping the same total report budget.
      if(failure&&(diagnostic.records.length>=12||JSON.stringify(diagnostic).length>diagnostic.byteLimit/2)){diagnostic.evictedForFailure=(diagnostic.evictedForFailure??0)+diagnostic.records.length;diagnostic.records=[];}
      const fits=()=>JSON.stringify({...diagnostic,records:[...diagnostic.records,record]}).length<=diagnostic.byteLimit;
      if(!fits()){diagnostic.omitted='Diagnostic byte limit reached';return;}
      for(let stage=0;stage<2;stage++) {
        const source=this.backend.programs.snapshots.get(packet.programs[stage])?.stages.find(s=>s.type===(stage?5:1));
        if(!source)continue;
        const item={stage,options:options[stage],metadata:shaders[stage].metadata,...(failure?{layout:shaders[stage].layout,activeBindings:shaders[stage].activeBindings}:{})};
        if(source.control.length+source.data.length<=32768){item.control=encode(source.control);item.data=encode(source.data);}
        else item.sourceOmitted='Shader source exceeds 32 KiB capture limit';
        record.shaders.push(item);
        if(!fits()){delete item.control;delete item.data;item.sourceOmitted='Diagnostic byte limit reached';}
      }
      const ranges=[];
      for(let stream=0;stream<packet.streams.length;stream++)if(packet.attributes.some(a=>(a&255)&&((a>>>8)&31)===stream))ranges.push({kind:'vertex',slot:stream,range:packet.streams[stream].slice(2)});
      const primitive=packet.primitive;
      if(primitive[1]>=0)ranges.push({kind:'index',range:[primitive[6],primitive[7],primitive[2]*(1<<primitive[1])]});
      for(let stage=0;stage<2;stage++)for(const [kind,values,base]of [['uniform',packet.ubo[stage],stage*40+3],['storage',packet.ssbo[stage],stage*40+20]])
        for(let slot=0;slot<values.length;slot++)if(!shaders[stage].activeBindings||shaders[stage].activeBindings.some(b=>b[0]===0&&b[1]===base+slot))ranges.push({kind,stage,slot,range:values[slot]});
      for(const item of ranges) {
        const [handle,offset,size]=item.range;if(!handle||!size)continue;
        if(failure&&this.backend.buffers){const buffer=this.backend.buffers.get(handle);if(!buffer||offset>buffer.size||size>buffer.size-offset){record.buffersOmitted='Failed draw contains unavailable buffer ranges';continue;}}
        // Prefix capture avoids expanding a large guest range in diagnostics.
        // It must not be mistaken for a complete vertex/uniform/storage image.
        const length=Math.min(size,4096),begin=Math.floor(offset/4)*4,end=Math.ceil((offset+length)/4)*4;
        if(JSON.stringify(diagnostic).length+JSON.stringify(record).length+Math.ceil(length/3)*4+256>diagnostic.byteLimit){record.buffersOmitted='Diagnostic byte limit reached';break;}
        const {bytes}=await this.backend.perform(3,{handle,offset:begin,size:end-begin});
        record.buffers.push({...item,capturedBytes:length,complete:length===size,data:encode(bytes.subarray(offset-begin,offset-begin+length))});
      }
      if(fits())diagnostic.records.push(record);else diagnostic.omitted='Diagnostic byte limit reached';
    }catch(error){diagnostic.lastError=String(error);}
  }
  async draw(bytes) {
    if(this.closed)throw new Error('Draw backend closed');
    if(!(bytes instanceof Uint8Array)||bytes.length>131072)throw new Error('Invalid graphics packet');
    const p=JSON.parse(new TextDecoder('utf-8',{fatal:true}).decode(bytes));
    if(p.version!==1)throw new Error('Unknown graphics packet version');
    this.lastPacket=p;
    const [mode,indexType,count,first,baseInstance,instances,indexHandle,indexOffset]=array(p.primitive,8,'draw parameters');
    for(const n of [mode,count,baseInstance,instances,indexHandle,indexOffset])uint(n,'draw parameter');
    if(!Number.isInteger(first)||first< -0x80000000||first>0x7fffffff)throw new Error('Invalid first/base vertex');
    if(![4,5].includes(mode)||![ -1,0,1,2].includes(indexType))throw new Error('Draw topology/index type needs lowering');
    if(indexType===-1&&first<0)throw new Error('Negative first vertex');
    if(!count||!instances)return;
    const b=this.backend,d=this.device;
    array(p.programs,2,'graphics programs');array(p.attributes,32,'vertex attributes');array(p.streams,32,'vertex streams');
    array(p.ubo,2,'uniform stages');array(p.ssbo,2,'storage stages');array(p.textures,2,'texture stages');
    array(p.blend,8,'blend states');array(p.color,4,'color state');array(p.polygon,4,'polygon state');array(p.ds,8,'depth/stencil state');
    array(p.stencil,6,'stencil masks/reference');array(p.viewport,6,'viewport');array(p.scissor,4,'scissor');array(p.blendColor,4,'blend constant');
    array(p.colors,8,'color targets');array(p.flags,5,'raster flags');
    if(p.flags.some((n,i)=>i===2?![0,1].includes(n):n!==0)||p.polygon[2]!==2||p.polygon[3]!==0||p.polygon[0]===3||p.color[1]!==3)
      throw new Error('Raster discard, depth clamp, polygon offset/mode or logic operation needs lowering');
    let integerAttributes=0,signedAttributes=0;
    for(let i=0;i<32;i++)if(p.attributes[i]&255){const f=vertexFormat(p.attributes[i]&255);if(f.integer)integerAttributes=(integerAttributes|1<<i)>>>0;if(f.signed)signedAttributes=(signedAttributes|1<<i)>>>0;}
    const textureFlags=stage=>{
      let integerTextures=0,signedTextures=0;const textureSwizzles={},lodQuerySamplers={};
      array(p.textures[stage],32,'texture bindings');
      for(let i=0;i<32;i++)if(p.textures[stage][i]){const tex=b.textures.get(p.textures[stage][i].texture),fmt=tex.format.format;
        lodQuerySamplers[`1/${stage*40+i}`]=p.textures[stage][i].sampler.slice();
        if(fmt.endsWith('uint')||fmt.endsWith('sint'))integerTextures=(integerTextures|1<<i)>>>0;
        if(fmt.endsWith('sint'))signedTextures=(signedTextures|1<<i)>>>0;
        const swizzle=p.textures[stage][i].swizzle;if(swizzle){if(!Array.isArray(swizzle)||swizzle.length!==4||swizzle.some(n=>!Number.isInteger(n)||n<0||n>5))throw new Error('Invalid NVN texture swizzle');if(swizzle.some((n,c)=>n!==c+2))textureSwizzles[`1/${stage*40+i}`]=swizzle.slice();}}
      return {integerTextures,signedTextures,lodQuerySamplers,...(Object.keys(textureSwizzles).length?{textureSwizzles}:{})};
    };
    const options={topology:'triangles',integerAttributes,signedAttributes,...(instances===1?{singleInstanceBase:baseInstance}:{})};
    const shaderOptions=[{...options,...textureFlags(0)},{...options,...textureFlags(1)}];
    const compileStage=async stage=>{
      const compile=async()=> (await b.programs.compileSnapshot(p.programs[stage],shaderOptions[stage])).find(s=>s.metadata.stage===stage);
      let shader=await compile();if(!shader)return shader;
      const missing=shader.layout.samplers.some(s=>!p.textures[stage][s.imageBinding-stage*40]&&(!shader.activeBindings||shader.activeBindings.some(([g,n])=>g===s.group&&(n===s.imageBinding||n===s.samplerBinding))));
      if(!missing)return shader;
      // WebGPU layouts include resources in runtime-disabled branches. Snapshot
      // the real read-only CB data in queue order; a conservative SPIR-V proof
      // can remove those branches. A genuinely sampled missing slot still fails.
      const uniformWords={};let budget=65536;
      for(let slot=0;slot<15;slot++)if((shader.metadata.cbufs>>>(slot+3))&1){
        const [handle,offset,size]=p.ubo[stage][slot];if(!handle||!size||size%4||offset%4||size>budget)continue;
        const record=b.buffer(handle);if(offset>record.size||size>record.size-offset)throw new Error('Uniform specialization range exceeds storage');
        const {bytes}=await b.perform(3,{handle,offset,size});
        const data=new DataView(bytes.buffer,bytes.byteOffset,bytes.byteLength);
        uniformWords[`0/${stage*40+slot+3}`]=Array.from({length:size/4},(_,i)=>data.getUint32(i*4,true));budget-=size;
      }
      if(Object.keys(uniformWords).length){shaderOptions[stage]={...shaderOptions[stage],uniformWords};shader=await compile();}
      return shader;
    };
    const fs=await compileStage(1);
    if(!fs)throw new Error('Draw lacks a retained fragment shader');
    shaderOptions[0].forceOutputs=fs.metadata.inputAttributes>>>0;
    const vs=await compileStage(0);
    if(!vs)throw new Error('Draw lacks a retained vertex shader');
    // Naga's entry call-graph usage, rather than native declaration masks,
    // determines the resources present in WebGPU's automatic pipeline layout.
    const bindingUsed=(shader,group,binding)=>!shader.activeBindings||shader.activeBindings.some(value=>value[0]===group&&value[1]===binding);
    const flat=/@interpolate\s*\(\s*flat\b/.test(fs.code);
    const convertIndices=flat||indexType===0||(indexType!==-1&&(mode===5||p.flags[2]||indexOffset%4!==0));
    b.statistics.shadersCompiled=b.programs.compilations;
    if([vs,fs].some(s=>s.metadata.images||s.metadata.ssbosWritten))throw new Error('Graphics storage writes/images need guest writeback support');
    const temporary=[];
    const upload=async(data,usage)=>{
      const buffer=await b.checked(()=>d.createBuffer({size:Math.max(16,Math.ceil(data.byteLength/4)*4),usage:usage|GPUBufferUsage.COPY_DST}));temporary.push(buffer);
      const padded=new Uint8Array(Math.ceil(data.byteLength/4)*4);padded.set(new Uint8Array(data.buffer,data.byteOffset,data.byteLength));
      if(padded.length)d.queue.writeBuffer(buffer,0,padded);return {buffer,offset:0,size:Math.max(16,padded.length)};
    };
    const range=async(value,usage,alignment)=>{
      array(value,3,'GPU buffer range');const [handle,offset,size]=value;uint(handle,'buffer handle');uint(offset,'buffer offset');uint(size,'buffer size');
      if(!handle||!size)throw new Error('Draw reads an unbound GPU range');
      const record=b.buffer(handle);if(offset>record.size||size>record.size-offset||size%4)throw new Error('Draw buffer range exceeds storage or is not aligned');
      if(offset%alignment===0)return {buffer:record.buffer,offset,size};
      if(offset%4)throw new Error('Unaligned NVN buffer address needs fetch conversion');
      const buffer=await b.checked(()=>d.createBuffer({size,usage:usage|GPUBufferUsage.COPY_DST}));temporary.push(buffer);
      await b.checked(()=>{const e=d.createCommandEncoder();e.copyBufferToBuffer(record.buffer,offset,buffer,0,size);d.queue.submit([e.finish()]);});
      this.statistics.drawBufferCopies++;return {buffer,offset:0,size};
    };
    try {
      let sourceIndices;
      const readIndices=async()=>{
        if(sourceIndices)return sourceIndices;
        if(count>16777216)throw new Error('Index conversion exceeds staging limit');
        const bytesPerIndex=1<<indexType,size=count*bytesPerIndex,record=b.buffer(indexHandle);
        if(indexOffset%bytesPerIndex||indexOffset>record.size||size>record.size-indexOffset)throw new Error('Invalid NVN index range');
        const begin=Math.floor(indexOffset/4)*4,end=Math.ceil((indexOffset+size)/4)*4;
        const {bytes:raw}=await b.perform(3,{handle:indexHandle,offset:begin,size:end-begin});
        const data=new DataView(raw.buffer,raw.byteOffset+indexOffset-begin,size);sourceIndices=new Uint32Array(count);
        for(let i=0;i<count;i++)sourceIndices[i]=indexType===0?data.getUint8(i):indexType===1?data.getUint16(i*2,true):data.getUint32(i*4,true);
        return sourceIndices;
      };
      let index,drawCount=count,indexFormat=indexType===2?'uint32':'uint16';
      if(convertIndices) {
        if(count>16777216)throw new Error('Index conversion exceeds staging limit');
        let input;
        if(indexType===-1){input=new Uint32Array(count);for(let i=0;i<count;i++)input[i]=i;}
        else input=await readIndices();
        const indices=triangleIndices(input,mode,{flat,restart:indexType!==-1&&p.flags[2]?[255,65535,4294967295][indexType]:null});
        drawCount=indices.length;
        if(!drawCount)return;
        index=await upload(indices,GPUBufferUsage.INDEX);indexFormat='uint32';
        this.statistics.convertedIndexDraws++;this.statistics.convertedIndexBytes+=indices.byteLength;
      }else if(indexType!==-1){index=await range([indexHandle,indexOffset,Math.ceil(count*(1<<indexType)/4)*4],GPUBufferUsage.INDEX,4);}
      const vertex=stageInterface(vs.code,'vertex'),fragment=stageInterface(fs.code,'fragment'),layouts=[],vertexBuffers=[];
      const fetchGroups=new Map();
      for(const input of vertex.locations){const encoded=p.attributes[input.shaderLocation]>>>0,nvn=encoded&255,stream=(encoded>>>8)&31,s=p.streams[stream];array(s,5,'vertex stream');
        if(nvn&&s[2]&&s[4]){if(!fetchGroups.has(stream))fetchGroups.set(stream,[]);fetchGroups.get(stream).push({input,encoded,nvn,f:vertexFormat(nvn)});}}
      const converted=new Map();
      for(const [stream,attributes] of fetchGroups){const s=p.streams[stream];
        if(![0,1].includes(s[1]))throw new Error('Vertex instance divisor needs fetch lowering');
        if(attributes.every(a=>a.f.format&&(a.encoded>>>14)%Math.min(4,a.f.bytes)===0)&&s[0]%4===0&&s[3]%4===0&&s[4]%4===0)continue;
        const stride=uint(s[0],'vertex stride'),[handle,offset,size]=s.slice(2),record=b.buffer(handle);
        uint(offset,'vertex offset');uint(size,'vertex size');if(offset>record.size||size>record.size-offset)throw new Error('Vertex range exceeds storage');
        let vertices=1;
        if(stride){if(s[1])vertices=baseInstance+instances;else if(indexType===-1)vertices=first+count;else{let maximum=-1;const restart=p.flags[2]?[255,65535,4294967295][indexType]:null;for(const value of await readIndices())if(value!==restart)maximum=Math.max(maximum,value);vertices=maximum+first+1;}}
        const packedStride=attributes.length*16,outputBytes=vertices*packedStride;
        if(vertices<1||!Number.isSafeInteger(outputBytes)||outputBytes>67108864)throw new Error('Vertex conversion exceeds staging limit');
        const extent=Math.max(...attributes.map(a=>(a.encoded>>>14)+a.f.bytes)),sourceBytes=(vertices-1)*stride+extent;
        if(sourceBytes>size)throw new Error('Vertex fetch exceeds bound NVN range');
        if(sourceBytes>67108864)throw new Error('Vertex source exceeds staging limit');
        const begin=Math.floor(offset/4)*4,end=Math.ceil((offset+sourceBytes)/4)*4;
        const {bytes:raw}=await b.perform(3,{handle,offset:begin,size:end-begin});
        const data=new DataView(raw.buffer,raw.byteOffset+offset-begin,sourceBytes),output=new ArrayBuffer(outputBytes),view=new DataView(output);
        for(let v=0;v<vertices;v++)for(let a=0;a<attributes.length;a++){const attr=attributes[a],values=decodeVertex(data,v*stride+(attr.encoded>>>14),attr.nvn,attr.f);
          for(let c=0;c<4;c++){const at=v*packedStride+a*16+c*4;if(attr.f.integer){if(attr.f.signed)view.setInt32(at,values[c],true);else view.setUint32(at,values[c],true);}else view.setFloat32(at,values[c],true);}}
        converted.set(stream,{buffer:await upload(new Uint8Array(output),GPUBufferUsage.VERTEX),stride:stride?packedStride:0,attributes});
        this.statistics.convertedVertexStreams++;this.statistics.convertedVertexBytes+=outputBytes;
      }
      const groups=new Map();
      for(const input of vertex.locations) {
        const encoded=p.attributes[input.shaderLocation]>>>0,nvn=encoded&255,stream=(encoded>>>8)&31,s=p.streams[stream];array(s,5,'vertex stream');
        const enabled=nvn&&s[2]&&s[4];
        const key=enabled?stream:`constant/${input.kind}`;
        if(!groups.has(key)) {
          let layout,buffer;
          if(enabled){const fetch=converted.get(stream);
            layout={arrayStride:fetch?fetch.stride:uint(s[0],'vertex stride'),stepMode:s[1]?'instance':'vertex',attributes:[]};buffer=fetch?fetch.buffer:await range(s.slice(2),GPUBufferUsage.VERTEX,4);}
          else{layout={arrayStride:0,stepMode:'vertex',attributes:[]};buffer=await upload(input.kind==='i32'?new Int32Array([0,0,0,1]):input.kind==='u32'?new Uint32Array([0,0,0,1]):new Float32Array([0,0,0,1]),GPUBufferUsage.VERTEX);}
          groups.set(key,layouts.length);layouts.push(layout);vertexBuffers.push(buffer);
        }
        const fetch=enabled&&converted.get(stream),f=enabled&&vertexFormat(nvn);
        const format=fetch?(f.integer?(f.signed?'sint32x4':'uint32x4'):'float32x4'):enabled?f.format:({f32:'float32x4',i32:'sint32x4',u32:'uint32x4'})[input.kind];
        layouts[groups.get(key)].attributes.push({shaderLocation:input.shaderLocation,offset:fetch?fetch.attributes.findIndex(a=>a.input.shaderLocation===input.shaderLocation)*16:enabled?encoded>>>14:0,format});
      }
      if(layouts.length>d.limits.maxVertexBuffers)throw new Error('NVN streams exceed WebGPU vertex binding count');
      const attachment=value=>{
        if(!value)return null;
        const record=b.textures.get(value.handle),format=value.format?textureFormat(value.format).format:record.format.format;
        if(value.layers!==1||record.dimension!=='2d')throw new Error('Layered graphics target needs multiview lowering');
        const view=record.texture.createView({format,dimension:'2d',baseMipLevel:value.mip,mipLevelCount:1,baseArrayLayer:value.layer,arrayLayerCount:1});
        return {record,format,view,width:Math.max(1,record.width>>value.mip),height:Math.max(1,record.height>>value.mip)};
      };
      const colors=p.colors.map(attachment),depth=attachment(p.depth),firstTarget=colors.find(Boolean)||depth;
      if(!firstTarget)throw new Error('Draw has no render target');
      const {width,height}=firstTarget;
      if([...colors,depth].some(t=>t&&(t.width!==width||t.height!==height)))throw new Error('Mismatched graphics attachment sizes');
      const targets=colors.map((t,i)=>{
        if(!t)return null;const state=p.blend[i];array(state,8,'blend value');
        const target={format:t.format,writeMask:fragment.outputs.some(output=>output.shaderLocation===i)?((p.channelMask>>>0)>>>(i*4))&15:0};
        if(target.writeMask&&(p.color[0]&(1<<i))){if(state[7])throw new Error('Advanced blending needs shader lowering');
          const component=(src,dst,op)=>{const operation=blendOp(op);return {operation,srcFactor:['min','max'].includes(operation)?'one':blendFactor(src),dstFactor:['min','max'].includes(operation)?'one':blendFactor(dst)};};
          target.blend={color:component(state[1],state[2],state[5]),alpha:component(state[3],state[4],state[6])};}
        return target;
      });
      while(targets.length&&targets.at(-1)===null)targets.pop();
      const descriptor={vertex:{module:vs.module,entryPoint:vertex.entryPoint,buffers:layouts},fragment:{module:fs.module,entryPoint:fragment.entryPoint,targets},
        primitive:{topology:mode===4||convertIndices?'triangle-list':'triangle-strip',frontFace:p.polygon[1]?'ccw':'cw',cullMode:['none','front','back'][p.polygon[0]]},
        multisample:{count:1,mask:uint(p.sampleMask,'sample mask'),alphaToCoverageEnabled:false}};
      const ds=p.ds;
      let bits=0n;ds.forEach((v,i)=>{bits|=BigInt(uint(v,'depth/stencil byte'))<<BigInt(i*8);});
      if(depth) {
        const state={format:depth.format};
        if(depth.record.format.depth){state.depthWriteEnabled=!!(bits&1n)&&!!(bits&2n);state.depthCompare=bits&1n?compare(Number(bits>>4n&15n)):'always';}
        if(depth.record.format.stencil&&bits&4n){
          if(p.stencil[0]!==p.stencil[1]||p.stencil[2]!==p.stencil[3]||p.stencil[4]!==p.stencil[5])throw new Error('Separate front/back stencil masks/reference need draw splitting');
          const face=(func,fail,dfail,pass)=>({compare:compare(Number(bits>>BigInt(func)&15n)),failOp:stencilOp(Number(bits>>BigInt(fail)&15n)),depthFailOp:stencilOp(Number(bits>>BigInt(dfail)&15n)),passOp:stencilOp(Number(bits>>BigInt(pass)&15n))});
          state.stencilFront=face(8,16,20,24);state.stencilBack=face(12,28,32,36);state.stencilReadMask=p.stencil[2]&255;state.stencilWriteMask=p.stencil[4]&255;
        }
        descriptor.depthStencil=state;
      }
      // Native tests pass when their attachment is absent. Keep the pipeline
      // depth/stencil state absent too; do not invent a buffer or reject draw.
      const key=JSON.stringify({...descriptor,vertex:{...descriptor.vertex,module:this.moduleId(vs.module)},fragment:{...descriptor.fragment,module:this.moduleId(fs.module)}});
      let pipeline=this.cache.get(key);
      if(!pipeline){pipeline=await b.checked(()=>d.createRenderPipelineAsync({layout:'auto',...descriptor}));if(this.cache.size>=256)this.cache.delete(this.cache.keys().next().value);this.cache.set(key,pipeline);this.statistics.graphicsPipelines++;}
      const bindings=Array.from({length:4},()=>new Map());
      const depthCopies=new Map();
      const logicalCopies=new Map(),arrayViews=new Map();
      const add=(group,binding,resource)=>{if(bindings[group].has(binding))throw new Error('Conflicting graphics binding');bindings[group].set(binding,{binding,resource});};
      for(const [stage,shader] of [vs,fs].entries()) {
        array(p.ubo[stage],15,'uniform buffers');array(p.ssbo[stage],16,'storage buffers');
        const base=stage*40,meta=shader.metadata,source=b.programs.snapshots.get(p.programs[stage])?.stages.find(s=>s.type===(stage?5:1));
        for(let n=0;n<18;n++)if(((meta.cbufs>>>n)&1)&&bindingUsed(shader,0,base+n)) {
          let resource;
          if(n===1){if(!source)throw new Error('Missing retained program constants');const control=new DataView(source.control.buffer,source.control.byteOffset,source.control.byteLength),size=control.getUint32(0x6fc,true),offset=control.getUint32(0x700,true);
            if(!size)throw new Error('Shader reads missing program constants');resource=await upload(source.data.subarray(offset,offset+size),GPUBufferUsage.STORAGE);}
          else if(n===2){const c2=new Uint32Array(224);for(let unit=0;unit<32;unit++){const t=p.textures[stage][unit];if(!t)continue;const tex=b.textures.get(t.texture),view=b.textures.getView(t.view).descriptor,mip=view.baseMipLevel;
              c2.set([Math.max(1,tex.width>>mip),Math.max(1,tex.height>>mip),tex.dimension==='3d'?Math.max(1,tex.depth>>mip):view.arrayLayerCount/(view.dimension==='cube-array'?6:1),view.mipLevelCount|((tex.dimension==='3d'?1:0)<<16)],96+unit*4);}
            resource=await upload(c2,GPUBufferUsage.STORAGE);}
          else if(n>=3)resource=await range(p.ubo[stage][n-3],GPUBufferUsage.STORAGE,d.limits.minStorageBufferOffsetAlignment);
          else throw new Error('Dynamic graphics constant buffer zero needs lowering');
          add(0,base+n,resource);
        }
        for(let n=0;n<16;n++)if(((meta.ssbos>>>n)&1)&&bindingUsed(shader,0,base+20+n))add(0,base+20+n,await range(p.ssbo[stage][n],GPUBufferUsage.STORAGE,d.limits.minStorageBufferOffsetAlignment));
        for(const sampler of shader.layout.samplers) {
          const imageUsed=bindingUsed(shader,sampler.group,sampler.imageBinding),samplerUsed=bindingUsed(shader,sampler.group,sampler.samplerBinding);
          if(!imageUsed&&!samplerUsed)continue;
          const unit=sampler.imageBinding-base,t=p.textures[stage][unit];if(!t)throw new Error(`Unbound shader texture ${stage}/${unit}`);
          const source=b.textures.get(t.texture);
          let view=b.textures.getView(t.view);if(view.texture!==t.texture)throw new Error('Texture binding view mismatch');
          const expectedDimension=shader.layout.textureDimensions?.[`${sampler.group}/${sampler.imageBinding}`];
          if(imageUsed&&expectedDimension&&expectedDimension!==view.descriptor.dimension){
            // A native 2D image is also a one-layer 2D array in Maxwell sampling.
            // Keep the native mip/layer range; do not invent extra layers.
            if(expectedDimension!=='2d-array'||view.descriptor.dimension!=='2d'||source.dimension!=='2d'||view.descriptor.arrayLayerCount!==1)throw new Error('Shader/texture view dimensionality needs explicit lowering');
            if(!arrayViews.has(t.view)){const descriptor={...view.descriptor,dimension:'2d-array'};const sampleView=await b.checked(()=>source.texture.createView(descriptor));arrayViews.set(t.view,{...view,descriptor,view:sampleView});this.statistics.arraySamplingViews++;}
            view=arrayViews.get(t.view);
          }
          let sampledView=view.view;
          if(imageUsed&&(source.width!==source.gpuWidth||source.height!==source.gpuHeight)){
            if(!logicalCopies.has(t.view)){const copy=await this.logicalSamples.create(source,view);temporary.push(copy.texture);logicalCopies.set(t.view,copy.view);Object.assign(this.statistics,this.logicalSamples.statistics);}
            sampledView=logicalCopies.get(t.view);
          }
          if(imageUsed&&source.format.depth&&!sampler.depth){
            if(!depthCopies.has(t.view)){const copy=await this.depthSamples.create(source,view);temporary.push(copy.texture);depthCopies.set(t.view,copy.view);}
            sampledView=depthCopies.get(t.view);
          }
          if(imageUsed)add(sampler.group,sampler.imageBinding,sampledView);
          if(samplerUsed)add(sampler.group,sampler.samplerBinding,await b.samplers.get(Uint8Array.from(t.sampler),{mipLevelCount:view.descriptor.mipLevelCount}));
        }
      }
      if([vs,fs].some(shader=>shader.layout.pushConstant&&bindingUsed(shader,3,0))){const pc=new Uint8Array(16),v=new DataView(pc.buffer);v.setUint32(0,p.color[2],true);v.setFloat32(4,p.alphaRef,true);v.setFloat32(8,p.pointSize,true);add(3,0,await upload(pc,GPUBufferUsage.UNIFORM));}
      if([vs,fs].some(shader=>shader.layout.drawParameters&&bindingUsed(shader,3,1)))add(3,1,await upload(new Uint32Array([first>>>0,baseInstance,0,0]),GPUBufferUsage.UNIFORM));
      const bindGroups=[];for(let i=0;i<bindings.length;i++)if(bindings[i].size)bindGroups.push([i,await b.checked(()=>d.createBindGroup({layout:pipeline.getBindGroupLayout(i),entries:[...bindings[i].values()]}))]);
      const viewport=p.viewport,scissor=p.scissor;
      if(viewport.some(n=>!Number.isFinite(n))||viewport[0]<0||viewport[1]<0||viewport[2]<0||viewport[3]<0||viewport[0]+viewport[2]>width||viewport[1]+viewport[3]>height||viewport[4]<0||viewport[5]>1||viewport[4]>viewport[5])throw new Error('NVN viewport needs clipping/lowering');
      if(scissor.some(n=>!Number.isInteger(n))||scissor[0]<0||scissor[1]<0||scissor[2]<0||scissor[3]<0||scissor[0]+scissor[2]>width||scissor[1]+scissor[3]>height)throw new Error('Invalid graphics scissor');
      await b.checked(()=>{
        const e=d.createCommandEncoder(),attachment={colorAttachments:colors.slice(0,targets.length).map(t=>t?{view:t.view,loadOp:'load',storeOp:'store'}:null)};
        if(depth){const t={view:depth.view};if(depth.record.format.depth)Object.assign(t,{depthLoadOp:'load',depthStoreOp:'store'});if(depth.record.format.stencil)Object.assign(t,{stencilLoadOp:'load',stencilStoreOp:'store'});attachment.depthStencilAttachment=t;}
        const pass=e.beginRenderPass(attachment);pass.setPipeline(pipeline);for(const [i,group]of bindGroups)pass.setBindGroup(i,group);
        vertexBuffers.forEach((buffer,i)=>pass.setVertexBuffer(i,buffer.buffer,buffer.offset,buffer.size));pass.setViewport(...viewport);pass.setScissorRect(...scissor);pass.setBlendConstant(p.blendColor);pass.setStencilReference(p.stencil[0]);
        if(index){pass.setIndexBuffer(index.buffer,indexFormat,index.offset,index.size);pass.drawIndexed(drawCount,instances,0,first,baseInstance);}else pass.draw(count,instances,first,baseInstance);
        pass.end();d.queue.submit([e.finish()]);
      });
      this.statistics.draws++;this.statistics.drawSubmissions++;if(index)this.statistics.indexedDraws++;
      const removed=[vs,fs].reduce((n,s)=>n+(s.layout.uniformSpecialization?.branches??0),0);if(removed){this.statistics.uniformSpecializedDraws++;this.statistics.uniformBranchesRemoved+=removed;}
      if(this.diagnosticDraws)await this.captureInputs(key,p,[vs,fs],shaderOptions);
    }catch(error){
      if(this.diagnosticDraws)await this.captureInputs(`failure:${this.statistics.draws}`,p,[vs,fs],shaderOptions,error);
      throw error;
    }finally{for(const buffer of temporary)buffer.destroy();Object.assign(b.statistics,this.statistics,b.samplers.statistics);}
  }
  close(){this.closed=true;this.cache.clear();}
}
