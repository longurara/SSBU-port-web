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
    this.decoder=new VideoDecoder({output:frame=>{
      if(this.closed){frame.close();return;}
      if(this.frames.length+this.copying>=this.capacity){frame.close();this.fail(Error('Movie output queue is full'));return;}
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
  get available(){return !this.closed&&!this.failure&&this.frames.length+this.copying+this.decoder.decodeQueueSize<this.capacity;}
  submit(sample){
    if(this.failure)throw this.failure;if(!this.available)throw Error('Movie input queue is full or closed');
    this.decoder.decode(new EncodedVideoChunk({type:sample.key?'key':'delta',timestamp:sample.timestamp,...(sample.duration===undefined?{}:{duration:sample.duration}),data:sample.data}));
  }
  take(){if(this.failure)throw this.failure;return this.frames.shift()??null;}
  async flush(){if(this.closed)throw Error('Movie decoder is closed');await this.decoder.flush();await this.chain;if(this.failure)throw this.failure;}
  close(){if(this.closed)return;this.closed=true;this.frames.length=0;if(this.decoder.state!=='closed')this.decoder.close();}
}
