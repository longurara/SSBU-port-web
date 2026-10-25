// WebM demuxing for the movie import adapter. Reads container bytes on demand;
// sample data and timestamps always come from the caller's actual media file.
// Container rules: https://www.webmproject.org/docs/container/
// Block/lacing rules: https://www.matroska.org/technical/notes.html
const ID={ebml:0x1a45dfa3,segment:0x18538067,info:0x1549a966,tracks:0x1654ae6b,
  cluster:0x1f43b675,entry:0xae,simple:0xa3,group:0xa0,block:0xa1,time:0xe7};
const text=new TextDecoder();
function vint(bytes,offset,{id=false,signed=false}={}){
  const first=bytes[offset];if(!first)throw Error('Invalid or truncated EBML integer');
  let width=1,mask=128;while(!(first&mask)){width++;mask>>=1;}
  if(width>(id?4:8)||offset+width>bytes.length)throw Error('Truncated EBML integer');
  let value=BigInt(id?first:first&(mask-1));
  for(let i=1;i<width;i++)value=(value<<8n)|BigInt(bytes[offset+i]);
  const unknown=!id&&value===(1n<<BigInt(7*width))-1n;
  if(signed)value-=(1n<<BigInt(7*width-1))-1n;
  if(!unknown&&(value>BigInt(Number.MAX_SAFE_INTEGER)||value<BigInt(Number.MIN_SAFE_INTEGER)))throw Error('EBML integer exceeds safe range');
  return {value:unknown?null:Number(value),width};
}
function uint(b){let n=0;for(const x of b){n=n*256+x;if(!Number.isSafeInteger(n))throw Error('WebM integer exceeds safe range');}return n;}
function float(b){const v=new DataView(b.buffer,b.byteOffset,b.byteLength);if(b.length===4)return v.getFloat32(0);if(b.length===8)return v.getFloat64(0);throw Error('Invalid WebM float size');}
function children(bytes){
  const out=[];let pos=0;
  while(pos<bytes.length){const id=vint(bytes,pos,{id:true}),size=vint(bytes,pos+id.width),start=pos+id.width+size.width;
    if(size.value===null||start+size.value>bytes.length)throw Error('Truncated or unknown-size WebM metadata');
    out.push({id:id.value,data:bytes.subarray(start,start+size.value)});pos=start+size.value;
  }return out;
}
function track(bytes){
  const t={number:0,type:0,codecId:'',codecPrivate:new Uint8Array(),defaultDurationNs:0};
  for(const e of children(bytes))switch(e.id){
    case 0xd7:t.number=uint(e.data);break;case 0x83:t.type=uint(e.data);break;
    case 0x86:t.codecId=text.decode(e.data);break;case 0x63a2:t.codecPrivate=e.data.slice();break;
    case 0x23e383:t.defaultDurationNs=uint(e.data);break;
    case 0x6d80:throw Error('WebM content encodings are unsupported');
    case 0xe0:for(const p of children(e.data)){if(p.id===0xb0)t.width=uint(p.data);if(p.id===0xba)t.height=uint(p.data);}break;
    case 0xe1:for(const p of children(e.data)){if(p.id===0xb5)t.sampleRate=float(p.data);if(p.id===0x9f)t.channels=uint(p.data);}break;
  }
  if(!t.number||!t.type||!t.codecId)throw Error('Incomplete WebM track');return t;
}
function splitBlock(bytes){
  const tn=vint(bytes,0),head=tn.width;if(head+3>bytes.length)throw Error('Truncated WebM block');
  const relative=new DataView(bytes.buffer,bytes.byteOffset+head,2).getInt16(0),flags=bytes[head+2],lace=(flags>>1)&3;
  let pos=head+3;const sizes=[];
  if(lace){
    if(pos>=bytes.length)throw Error('Missing WebM lace count');const count=bytes[pos++]+1;
    if(count<2)throw Error('Invalid WebM lace count');
    if(lace===2){const size=(bytes.length-pos)/count;if(!Number.isInteger(size))throw Error('Invalid fixed lacing');for(let i=0;i<count-1;i++)sizes.push(size);}
    else if(lace===1){for(let i=0;i<count-1;i++){let size=0,n;do{if(pos>=bytes.length)throw Error('Truncated Xiph lace');n=bytes[pos++];size+=n;}while(n===255);sizes.push(size);}}
    else {const first=vint(bytes,pos);if(first.value===null)throw Error('Invalid EBML lace size');pos+=first.width;sizes.push(first.value);
      for(let i=1;i<count-1;i++){const delta=vint(bytes,pos,{signed:true});if(delta.value===null)throw Error('Invalid EBML lace delta');pos+=delta.width;sizes.push(sizes.at(-1)+delta.value);}}
    sizes.push(bytes.length-pos-sizes.reduce((a,b)=>a+b,0));
  }else sizes.push(bytes.length-pos);
  const frames=[];for(const size of sizes){if(!Number.isSafeInteger(size)||size<0||pos+size>bytes.length)throw Error('Invalid WebM lace sizes');frames.push(bytes.subarray(pos,pos+size));pos+=size;}
  return {track:tn.value,relative,flags,frames};
}

export class WebmDemuxer {
  constructor(source){
    const randomAccess=source&&Number.isSafeInteger(source.size)&&source.size>=0&&typeof source.read==='function';
    if(!(source instanceof Blob)&&!(source instanceof Uint8Array)&&!randomAccess)throw TypeError('Expected Blob, Uint8Array, or bounded random-access reader');
    this.source=source;this.size=source instanceof Uint8Array?source.byteLength:source.size;
    this.tracks=[];this.timestampScaleNs=1000000;this.durationUs=0;this.segment=null;this.cache=null;
  }
  async read(offset,length){
    if(!Number.isSafeInteger(offset)||!Number.isSafeInteger(length)||offset<0||length<0||offset+length>this.size)throw Error('Truncated WebM file');
    if(this.source instanceof Uint8Array)return this.source.subarray(offset,offset+length);
    if(this.cache&&offset>=this.cache.offset&&offset+length<=this.cache.offset+this.cache.bytes.length)return this.cache.bytes.subarray(offset-this.cache.offset,offset-this.cache.offset+length);
    const end=Math.min(this.size,offset+Math.max(length,65536));
    let bytes;
    if(this.source instanceof Blob)bytes=new Uint8Array(await this.source.slice(offset,end).arrayBuffer());
    else {const result=await this.source.read(offset,end-offset);if(!(result instanceof Uint8Array)||result.length!==end-offset)throw Error('Short WebM source read');bytes=Uint8Array.from(result);}
    this.cache={offset,bytes};return bytes.subarray(0,length);
  }
  async element(offset,limit=this.size){
    const b=await this.read(offset,Math.min(12,limit-offset)),id=vint(b,0,{id:true}),size=vint(b,id.width),start=offset+id.width+size.width;
    if(start>limit||(size.value!==null&&start+size.value>limit))throw Error('WebM element exceeds parent');
    return {id:id.value,start,end:size.value===null?limit:start+size.value,unknown:size.value===null};
  }
  async open(){
    const header=await this.element(0);if(header.id!==ID.ebml||header.unknown||header.end-header.start>65536)throw Error('Invalid WebM header');
    const type=children(await this.read(header.start,header.end-header.start)).find(e=>e.id===0x4282);
    if(!type||text.decode(type.data)!=='webm')throw Error('Expected WebM document');
    let pos=header.end;
    while(pos<this.size){const e=await this.element(pos);if(e.id===ID.segment){this.segment=e;break;}if(e.unknown)throw Error('Unknown-size top-level element');pos=e.end;}
    if(!this.segment)throw Error('Missing WebM segment');
    pos=this.segment.start;let haveInfo=false,haveTracks=false;
    while(pos<this.segment.end){const e=await this.element(pos,this.segment.end);
      if(e.id===ID.info||e.id===ID.tracks){if(e.unknown||e.end-e.start>4*1024*1024)throw Error('WebM metadata exceeds limit');
        const data=children(await this.read(e.start,e.end-e.start));
        if(e.id===ID.info){let duration=0;for(const p of data){if(p.id===0x2ad7b1)this.timestampScaleNs=uint(p.data);if(p.id===0x4489)duration=float(p.data);}this.durationUs=duration*this.timestampScaleNs/1000;haveInfo=true;}
        else {this.tracks=data.filter(p=>p.id===ID.entry).map(p=>track(p.data));haveTracks=true;}
      }
      if(haveInfo&&haveTracks)break;if(e.unknown)throw Error('Tracks must precede unknown-size clusters');pos=e.end;
    }
    if(!haveInfo||!haveTracks||!this.tracks.length||this.tracks.length>256||!(this.timestampScaleNs>0)||!Number.isFinite(this.durationUs))throw Error('Invalid WebM metadata');
    if(new Set(this.tracks.map(t=>t.number)).size!==this.tracks.length)throw Error('Duplicate WebM track number');
    return this;
  }
  async *samples({trackNumber,trackNumbers,startOffset}={}){
    if(!this.segment)await this.open();let pos=startOffset??this.segment.start;
    if(!Number.isSafeInteger(pos)||pos<this.segment.start||pos>=this.segment.end)throw Error('Invalid WebM sample start offset');
    while(pos<this.segment.end){const cluster=await this.element(pos,this.segment.end);
      if(cluster.id!==ID.cluster){if(cluster.unknown)throw Error('Unknown-size non-cluster element');pos=cluster.end;continue;}
      let at=cluster.start,time=0;
      while(at<cluster.end){const e=await this.element(at,cluster.end);
        if(cluster.unknown&&[ID.cluster,ID.info,ID.tracks,0x1c53bb6b,0x114d9b74,0x1254c367,0x1043a770].includes(e.id))break;
        if(e.unknown)throw Error('Unknown-size cluster child');
        if(e.id===ID.time)time=uint(await this.read(e.start,e.end-e.start));
        if(e.id===ID.simple||e.id===ID.group){
          if(e.end-e.start>32*1024*1024)throw Error('WebM block exceeds limit');
          const data=await this.read(e.start,e.end-e.start);let block=data,key=true,duration;
          if(e.id===ID.group){const group=children(data),b=group.find(g=>g.id===ID.block);if(!b)throw Error('Missing Block in BlockGroup');block=b.data;key=!group.some(g=>g.id===0xfb);const d=group.find(g=>g.id===0x9b);if(d)duration=uint(d.data)*this.timestampScaleNs;}
          const parsed=splitBlock(block),t=this.tracks.find(t=>t.number===parsed.track);if(!t)throw Error('Block references missing track');
          if(e.id===ID.simple)key=!!(parsed.flags&128);
          if((trackNumber===undefined||trackNumber===parsed.track)&&(!trackNumbers||trackNumbers.has(parsed.track))){
            const step=duration!==undefined?duration/parsed.frames.length:t.defaultDurationNs;
            if(parsed.frames.length>1&&!step)throw Error('Laced sample timestamps require a duration');
            for(let i=0;i<parsed.frames.length;i++)yield {trackNumber:parsed.track,timestamp:Math.round(((time+parsed.relative)*this.timestampScaleNs+i*step)/1000),duration:step?Math.round(step/1000):undefined,key,clusterOffset:pos,blockOffset:at,laceIndex:i,data:parsed.frames[i].slice()};
          }
        }
        at=e.end;
      }
      pos=cluster.unknown?at:cluster.end;
    }
  }
}
