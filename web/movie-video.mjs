// Real WebCodecs output, converted to the SDK's tightly packed NV12 planes.
// https://www.w3.org/TR/webcodecs/#videoframe-interface
export async function videoFrameNv12(frame){
  const r=frame.visibleRect,w=r.width,h=r.height,cw=Math.ceil(w/2),ch=Math.ceil(h/2),ySize=w*h,uvSize=2*cw*ch;
  if(!Number.isSafeInteger(ySize)||w<1||h<1||w>8192||h>8192)throw Error('Movie frame dimensions exceed limits');
  const data=new Uint8Array(ySize+uvSize),rect={x:r.x,y:r.y,width:w,height:h};
  if(frame.format==='NV12')await frame.copyTo(data,{rect,layout:[{offset:0,stride:w},{offset:ySize,stride:2*cw}]});
  else if(frame.format==='I420'){
    const planar=new Uint8Array(ySize+uvSize),planeSize=cw*ch;
    await frame.copyTo(planar,{rect,layout:[{offset:0,stride:w},{offset:ySize,stride:cw},{offset:ySize+planeSize,stride:cw}]});
    data.set(planar.subarray(0,ySize));
    for(let i=0;i<planeSize;i++){data[ySize+2*i]=planar[ySize+i];data[ySize+2*i+1]=planar[ySize+planeSize+i];}
  }else throw Error(`Unsupported movie pixel format: ${frame.format}`);
  return {data,width:w,height:h,yStride:w,uvStride:2*cw,yOffset:0,uvOffset:ySize,timestamp:frame.timestamp,duration:frame.duration,colorSpace:frame.colorSpace?.toJSON()};
}

export function videoTrackConfig(track){
  const codecs={V_VP8:'vp8',V_VP9:'vp09.00.10.08'};
  if(track.type!==1||!codecs[track.codecId])throw Error(`Unsupported movie video codec: ${track.codecId}`);
  // VP9 CodecPrivate may declare a different profile/bit depth. Do not decode
  // that data under a guessed profile-zero configuration.
  if(track.codecPrivate?.length)throw Error('VP9 CodecPrivate configuration is not implemented');
  return {codec:codecs[track.codecId],codedWidth:track.width,codedHeight:track.height,hardwareAcceleration:'prefer-software'};
}

// Reserve output storage before submitting compressed input. A VP9 packet can
// contain invisible reference frames or several indexed frames, so neither
// decodeQueueSize nor one-output-per-packet describes the pending allocation.
// https://downloads.webmproject.org/docs/vp9/vp9-bitstream-specification-v0.6-20160331-draft.pdf
export function videoPacketFrames(codec,data){
  if(!(data instanceof Uint8Array)||!data.length)throw Error('Empty movie video packet');
  if(codec==='vp8'){
    if(data.length<3)throw Error('Truncated VP8 frame tag');
    return [{data,visible:(data[0]>>4)&1}];
  }
  if(!codec.startsWith('vp09.00.'))throw Error('Unsupported movie packet codec');
  const visible=frame=>{
    if(!frame.length||(frame[0]>>6)!==2)throw Error('Invalid VP9 frame marker');
    if(frame[0]&0x30)throw Error('Unsupported VP9 bitstream profile');
    return (frame[0]&8)?1:(frame[0]>>1)&1;
  };
  const marker=data.at(-1);
  if((marker&0xe0)!==0xc0)return [{data,visible:visible(data)}];
  const count=(marker&7)+1,magnitude=((marker>>3)&3)+1,start=data.length-2-count*magnitude;
  if(start<0||data[start]!==marker)throw Error('Invalid VP9 superframe index');
  let cursor=start+1,offset=0;const frames=[];
  for(let i=0;i<count;i++){
    let size=0;for(let j=0;j<magnitude;j++)size+=data[cursor++]*2**(8*j);
    if(!size||offset+size>start)throw Error('Invalid VP9 superframe size');
    const frame=data.subarray(offset,offset+size);frames.push({data:frame,visible:visible(frame)});offset+=size;
  }
  if(offset!==start)throw Error('VP9 superframe contains unindexed data');
  return frames;
}
export function videoPacketOutputs(codec,data){return videoPacketFrames(codec,data).reduce((n,frame)=>n+frame.visible,0);}

export class MovieVideoDecoder {
  static async create(track,options={}){
    if(typeof VideoDecoder==='undefined')throw Error('WebCodecs VideoDecoder is unavailable');
    const config=videoTrackConfig(track),support=await VideoDecoder.isConfigSupported(config);
    if(!support.supported)throw Error(`Browser cannot decode ${config.codec}`);
    return new MovieVideoDecoder(support.config,options);
  }
  constructor(config,{onChange=()=>{},capacity=16}={}){
    if(!Number.isInteger(capacity)||capacity<2||capacity>64)throw Error('Invalid movie queue capacity');
    this.config=config;this.capacity=capacity;this.frames=[];this.copying=0;this.failure=null;this.closed=false;this.chain=Promise.resolve();this.onChange=onChange;
    this.occupied=0;this.awaitingOutput=0;this.leases=new Set();
    this.decoder=new VideoDecoder({output:frame=>{
      if(this.closed){frame.close();return;}
      if(!this.awaitingOutput||this.frames.length+this.copying+this.leases.size>=this.capacity){frame.close();this.fail(Error('Movie output exceeds reserved storage'));return;}
      this.awaitingOutput--;
      this.copying++;
      this.chain=this.chain.then(async()=>{
        try{const result=await videoFrameNv12(frame);if(!this.closed)this.frames.push(result);}
        catch(error){this.fail(error);}
        finally{frame.close();this.copying--;this.onChange();}
      });
    },error:error=>this.fail(error)});
    this.decoder.addEventListener('dequeue',()=>this.onChange());this.decoder.configure(config);
  }
  fail(error){this.failure??=error;this.onChange();}
  get available(){return !this.closed&&!this.failure&&this.occupied<this.capacity&&this.decoder.decodeQueueSize<this.capacity;}
  canSubmit(sample){
    if(this.closed||this.failure)return false;
    const frames=videoPacketFrames(this.config.codec,sample.data);
    if(frames.length>this.capacity)throw Error('Movie packet exceeds decoder input queue capacity');
    return this.decoder.decodeQueueSize+frames.length<=this.capacity&&this.occupied+frames.reduce((n,f)=>n+f.visible,0)<=this.capacity;
  }
  submit(sample){
    if(this.failure)throw this.failure;if(!this.canSubmit(sample))throw Error('Movie input queue is full or closed');
    const frames=videoPacketFrames(this.config.codec,sample.data),outputs=frames.reduce((n,f)=>n+f.visible,0);
    this.occupied+=outputs;this.awaitingOutput+=outputs;
    let accepted=0,acceptedOutputs=0;
    try{
      // WebCodecs VP9 input is one coded frame. Chromium can retain only the
      // final displayable image if a multi-output superframe is submitted as
      // one chunk. Preserve each indexed frame, including invisible references.
      for(const frame of frames){
        this.decoder.decode(new EncodedVideoChunk({type:sample.key&&!accepted?'key':'delta',timestamp:sample.timestamp,...(sample.duration===undefined?{}:{duration:sample.duration}),data:frame.data}));
        accepted++;acceptedOutputs+=frame.visible;
      }
    }catch(error){
      this.occupied-=outputs-acceptedOutputs;this.awaitingOutput-=outputs-acceptedOutputs;
      if(accepted)this.fail(error);throw error;
    }
  }
  take({retain=false}={}){if(this.failure)throw this.failure;const frame=this.frames.shift()??null;if(frame){if(retain)this.leases.add(frame);else{this.occupied--;this.onChange();}}return frame;}
  release(frame){if(!this.leases.delete(frame))throw Error('Unknown movie output lease');this.occupied--;this.onChange();}
  async flush(){if(this.closed)throw Error('Movie decoder is closed');await this.decoder.flush();await this.chain;if(this.failure)throw this.failure;if(this.awaitingOutput)throw Error('Movie decoder did not produce all visible frames');}
  close(){if(this.closed)return;this.closed=true;this.frames.length=0;this.leases.clear();this.occupied=0;this.awaitingOutput=0;if(this.decoder.state!=='closed')this.decoder.close();}
}
