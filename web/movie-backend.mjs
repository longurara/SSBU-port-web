import {WebmDemuxer} from './movie-webm.mjs';
import {MovieVideoDecoder} from './movie-video.mjs';

// Fixed-width bridge headers are mirrored in host_movie_bridge.h. SDK results,
// not boolean "success", cross the bridge. END is only actual extractor EOF.
export const MovieStatus=Object.freeze({OK:0,FAILED:0x1000,INVALID:0x100b,UNSUPPORTED:0x1006,END:0x2000,NO_INPUT:0x1011,NO_OUTPUT:0x100e});
export const MovieOp=Object.freeze({CREATE:1,APPEND:2,OPEN:3,TRACK:4,SELECT:5,PEEK:6,READ:7,ADVANCE:8,SEEK:9,CLOSE:10,
  CONFIGURE:20,SUBMIT:21,ACQUIRE:22,COPY:23,RELEASE:24,RESET:25,FLUSH:26,DECODER_CLOSE:27,EOS:28});
class StatusError extends Error {constructor(status,message){super(message);this.status=status;}}
const invalid=message=>{throw new StatusError(MovieStatus.INVALID,message);};
const unsupported=message=>{throw new StatusError(MovieStatus.UNSUPPORTED,message);};
const codecType=t=>({V_VP8:2,V_VP9:3,A_VORBIS:5,A_OPUS:6}[t.codecId]??0);
const supported=t=>t.type===1&&[2,3].includes(codecType(t))&&!t.codecPrivate.length;
function header(size,write){const bytes=new Uint8Array(size);write(new DataView(bytes.buffer));return bytes;}

export class MovieBackend {
  constructor({notify=()=>{}}={}){this.entries=new Map();this.next=1;this.notify=notify;this.closed=false;this.pending=new Set();}
  add(entry){if(this.next>0x7fffffff)throw Error('Movie handle space exhausted');const id=this.next++;Object.assign(entry,{tail:Promise.resolve(),closed:false});this.entries.set(id,entry);return id;}
  async request(request){
    if(this.closed)invalid('Movie backend closed');
    const {operation,handle=0}=request;
    if(operation===MovieOp.CREATE)return {status:0,value:this.add({kind:'extractor',parts:[],bytes:0,demux:null,selected:-1,pending:null,iterator:null,eof:false,start:null,keys:new Map()})};
    const entry=this.entries.get(handle);if(!entry)invalid('Unknown movie handle');
    const run=entry.tail.then(async()=>{
      if(entry.closed)invalid('Movie handle closed');const reply=await this.perform(entry,request);
      if(this.closed){if(operation===MovieOp.CONFIGURE){const d=this.entries.get(reply.value);d?.decoder.close();this.entries.delete(reply.value);}invalid('Movie backend closed during request');}
      return reply;
    });
    entry.tail=run.catch(()=>{});return run;
  }
  async perform(e,{operation,handle,index=0,flags=0,time=0,bytes=new Uint8Array()}){
    if(!Number.isInteger(index)||!Number.isInteger(flags)||!Number.isSafeInteger(time))invalid('Invalid movie request arguments');
    if(!(bytes instanceof Uint8Array))invalid('Movie input must contain bytes');
    if(e.kind==='extractor'){
      const track=()=>{if(!e.demux||index<0||index>=e.demux.tracks.length)invalid('Unknown movie track');return e.demux.tracks[index];};
      switch(operation){
        case MovieOp.APPEND:
          if(e.demux||!bytes.length||e.bytes+bytes.length>512*1024*1024)invalid('Movie source exceeds limit or is already open');
          e.parts.push(bytes.slice());e.bytes+=bytes.length;return {status:0};
        case MovieOp.OPEN:{
          if(e.demux||!e.bytes)invalid('Movie source is empty or already open');
          const source=new Blob(e.parts);e.parts=[];e.demux=await new WebmDemuxer(source).open();
          if(e.demux.tracks.length>8)unsupported('Too many movie tracks');
          e.maximum=new Map();
          for(const t of e.demux.tracks)if(supported(t)){
            let maximum=0;const keys=[];
            for await(const sample of e.demux.samples({trackNumber:t.number})){maximum=Math.max(maximum,sample.data.length);if(sample.key)keys.push({timestamp:sample.timestamp,clusterOffset:sample.clusterOffset,blockOffset:sample.blockOffset,laceIndex:sample.laceIndex});}
            if(!keys.length)unsupported('Movie video has no key frames');e.keys.set(t.number,keys);e.maximum.set(t.number,maximum);
          }
          return {status:0,value:e.demux.tracks.length};
        }
        case MovieOp.TRACK:{const t=track();return {status:0,bytes:header(64,v=>{
          for(const [offset,value] of [[0,codecType(t)],[4,t.type],[8,t.width??0],[12,t.height??0],[16,t.sampleRate??0],[20,t.channels??0],[24,e.maximum.get(t.number)??0],[28,t.defaultDurationNs?Math.round(1e9/t.defaultDurationNs):0],[40,t.number],[44,+supported(t)]])v.setUint32(offset,value,true);
          v.setBigInt64(32,BigInt(Math.round(e.demux.durationUs)),true);
        })};}
        case MovieOp.SELECT:{const t=track();
          const previous=e.selected;
          if(flags){if(!supported(t))unsupported(`Movie track codec is unsupported: ${t.codecId}`);if(e.selected>=0&&e.selected!==index)unsupported('Multiple selected video tracks are unsupported');e.selected=index;}
          else if(e.selected===index)e.selected=-1;
          if(previous!==e.selected){e.iterator=null;e.pending=null;e.eof=false;e.start=null;}return {status:0};
        }
        case MovieOp.PEEK:{
          if(!e.demux)invalid('Movie source is not open');
          if(e.selected<0)return {status:MovieStatus.END};
          const t=e.demux.tracks[e.selected];
          if(!e.iterator)e.iterator=e.demux.samples({trackNumber:t.number,startOffset:e.start?.clusterOffset})[Symbol.asyncIterator]();
          while(!e.pending){const next=await e.iterator.next();if(next.done){e.eof=true;return {status:MovieStatus.END};}
            if(e.start){if(next.value.blockOffset!==e.start.blockOffset||next.value.laceIndex!==e.start.laceIndex)continue;e.start=null;}
            e.pending=next.value;
          }
          const p=e.pending;return {status:0,value:p.data.length,bytes:header(24,v=>{v.setUint32(0,e.selected,true);v.setUint32(4,p.data.length,true);v.setBigInt64(8,BigInt(p.timestamp),true);v.setUint32(16,+p.key,true);v.setUint32(20,p.duration??0,true);})};
        }
        case MovieOp.READ:if(!e.pending)invalid('No current movie sample');return {status:0,bytes:e.pending.data};
        case MovieOp.ADVANCE:if(!e.pending)invalid('No current movie sample');e.pending=null;return {status:0};
        case MovieOp.SEEK:{
          if(!e.demux||e.selected<0||time<0)invalid('Invalid movie seek');
          const keys=e.keys.get(e.demux.tracks[e.selected].number);e.start=keys[0];
          for(const key of keys){if(key.timestamp>time)break;e.start=key;}
          e.pending=null;e.iterator=null;e.eof=false;return {status:0};
        }
        case MovieOp.CLOSE:e.closed=true;e.parts=[];e.demux=null;e.pending=null;e.iterator=null;e.keys.clear();this.entries.delete(handle);return {status:0};
        case MovieOp.CONFIGURE:{
          const t=track();if(!supported(t))unsupported(`Movie decoder codec is unsupported: ${t.codecId}`);
          const d={kind:'decoder',source:e,track:index,owner:flags,held:new Map(),nextOutput:1,eos:false,eosDelivered:false,decoder:null};
          const changed=()=>{if(!d.closed)this.notify(d.owner,1|(d.decoder?.frames.length?2:0)|(d.decoder?.failure?8:0));};
          d.decoder=await MovieVideoDecoder.create(t,{onChange:changed,capacity:48});
          return {status:0,value:this.add(d)};
        }
      }
    }else if(e.kind==='decoder'){
      switch(operation){
        case MovieOp.SUBMIT:{
          if(e.eos)invalid('Movie input submitted after EOS');
          if(e.source.closed)invalid('Movie source was closed');
          if(!e.decoder.available||e.held.size+e.decoder.frames.length+e.decoder.copying+e.decoder.decoder.decodeQueueSize>=48)return {status:MovieStatus.NO_INPUT};
          const t=e.source.demux.tracks[e.track],key=e.source.keys.get(t.number).some(k=>k.timestamp===time);
          e.decoder.submit({timestamp:time,key,data:bytes});return {status:0};
        }
        case MovieOp.FLUSH:await e.decoder.flush();return {status:0};
        case MovieOp.EOS:
          if(!e.source.eof)invalid('Movie EOS requires actual extractor EOF');
          await e.decoder.flush();e.eos=true;this.notify(e.owner,2);return {status:0};
        case MovieOp.ACQUIRE:{
          const frame=e.decoder.take();
          if(!frame){if(e.eos&&!e.eosDelivered){e.eosDelivered=true;return {status:0,value:0,bytes:header(40,v=>{v.setUint32(20,4,true);})};}return {status:MovieStatus.NO_OUTPUT};}
          if(e.nextOutput>0x7fffffff)throw Error('Movie output handle space exhausted');
          const slot=e.nextOutput++;e.held.set(slot,frame);
          return {status:0,value:slot,bytes:header(40,v=>{v.setUint32(0,slot,true);v.setUint32(4,frame.data.length,true);v.setBigInt64(8,BigInt(frame.timestamp),true);v.setUint32(16,e.decoder.frames.length,true);v.setUint32(24,frame.width,true);v.setUint32(28,frame.height,true);v.setUint32(32,frame.yStride,true);v.setUint32(36,frame.uvOffset,true);})};
        }
        case MovieOp.COPY:{const frame=e.held.get(index);if(!frame)invalid('Unknown movie output slot');return {status:0,bytes:frame.data};}
        case MovieOp.RELEASE:if(index===0&&e.eosDelivered)return {status:0};if(!e.held.delete(index))invalid('Unknown movie output slot');this.notify(e.owner,1);return {status:0};
        case MovieOp.RESET:{
          e.decoder.close();e.held.clear();e.eos=false;e.eosDelivered=false;
          if(e.source.closed)invalid('Movie source was closed');
          e.decoder=await MovieVideoDecoder.create(e.source.demux.tracks[e.track],{capacity:48,onChange:()=>{if(!e.closed)this.notify(e.owner,1|(e.decoder.frames.length?2:0)|(e.decoder.failure?8:0));}});return {status:0};
        }
        case MovieOp.DECODER_CLOSE:e.closed=true;e.decoder.close();e.held.clear();this.entries.delete(handle);return {status:0};
      }
    }
    invalid('Unknown movie operation for this handle');
  }
  async dispatch(module,request){
    const {token,pointer=0,length=0}=request;
    if(!Number.isInteger(token)||token<=0||this.pending.has(token))throw Error('Invalid or duplicate movie completion token');
    this.pending.add(token);let status=MovieStatus.FAILED,value=0;
    try{
      if(!Number.isInteger(pointer)||!Number.isInteger(length)||pointer<0||length<0||length>32*1024*1024||pointer+length>module.HEAPU8.length)invalid('Movie staging range exceeds Wasm memory');
      const minimum=({[MovieOp.TRACK]:64,[MovieOp.PEEK]:24,[MovieOp.ACQUIRE]:40})[request.operation]??0;
      if(length<minimum)invalid('Movie reply header staging buffer is too small');
      const input=[MovieOp.APPEND,MovieOp.SUBMIT].includes(request.operation)?module.HEAPU8.slice(pointer,pointer+length):new Uint8Array();
      const reply=await this.request({...request,bytes:input});status=reply.status??0;value=reply.value??0;
      if(reply.bytes){if(reply.bytes.length>length)invalid('Movie staging buffer is too small');module.HEAPU8.set(reply.bytes,pointer);}
    }catch(error){status=error.status??MovieStatus.FAILED;value=0;module.webmovieError=String(error);}
    finally{this.pending.delete(token);module._movie_complete(token,status,value);}
  }
  async close(){for(const e of this.entries.values()){e.closed=true;if(e.kind==='decoder')e.decoder.close();}this.entries.clear();this.closed=true;}
}
