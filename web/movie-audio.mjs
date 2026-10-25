// Actual Vorbis output converted by WebCodecs to the SDK's interleaved s16 PCM.
// https://www.w3.org/TR/webcodecs/#audiodata-interface
export function audioTrackConfig(track){
  if(track.type!==2||track.codecId!=='A_VORBIS')throw Error('Unsupported movie audio codec');
  const data=track.codecPrivate;
  if(!(data instanceof Uint8Array)||data.length<4||data.length>1048576||data[0]!==2)throw Error('Invalid Vorbis CodecPrivate');
  let at=1;const sizes=[];
  for(let i=0;i<2;i++){
    let size=0,byte;do{if(at>=data.length)throw Error('Truncated Vorbis header sizes');byte=data[at++];size+=byte;}while(byte===255);
    sizes.push(size);
  }
  sizes.push(data.length-at-sizes[0]-sizes[1]);let identification;
  for(let i=0;i<3;i++){
    const size=sizes[i];if(size<7||at+size>data.length||data[at]!==1+2*i||String.fromCharCode(...data.subarray(at+1,at+7))!=='vorbis')throw Error('Invalid Vorbis header');
    if(!i)identification=data.subarray(at,at+size);at+=size;
  }
  if(identification.length!==30)throw Error('Invalid Vorbis identification length');
  const view=new DataView(identification.buffer,identification.byteOffset,identification.byteLength),channels=identification[11],rate=view.getUint32(12,true),small=identification[28]&15,large=identification[28]>>4;
  if(view.getUint32(7,true)!==0||!(identification[29]&1)||channels<1||channels>8||rate<8000||rate>192000||small<6||large>13||small>large||channels!==track.channels||rate!==track.sampleRate)throw Error('Unsupported or inconsistent Vorbis identification');
  return {codec:'vorbis',sampleRate:rate,numberOfChannels:channels,description:data};
}

export function audioDataS16(frame){
  const samples=frame.numberOfFrames,channels=frame.numberOfChannels,rate=frame.sampleRate;
  if(!Number.isInteger(samples)||samples<1||samples>65536||!Number.isInteger(channels)||channels<1||channels>8||!Number.isFinite(rate)||rate<8000||rate>192000||!Number.isSafeInteger(frame.timestamp))throw Error('Invalid movie PCM dimensions or timestamp');
  const options={format:'s16',planeIndex:0},size=samples*channels*2;
  if(frame.allocationSize(options)!==size)throw Error('Unexpected movie PCM allocation size');
  const data=new Uint8Array(size);frame.copyTo(data,options);
  return {data,samples,channels,sampleRate:rate,timestamp:frame.timestamp,duration:frame.duration};
}

export class MovieAudioDecoder {
  static async create(track,options={}){
    if(typeof AudioDecoder==='undefined')throw Error('WebCodecs AudioDecoder is unavailable');
    const config=audioTrackConfig(track),support=await AudioDecoder.isConfigSupported(config);
    if(!support.supported)throw Error('Browser cannot decode Vorbis');
    return new MovieAudioDecoder(support.config,options);
  }
  constructor(config,{capacity=64,onChange=()=>{}}={}){
    if(!Number.isInteger(capacity)||capacity<2||capacity>64)throw Error('Invalid movie audio queue capacity');
    this.config=config;this.capacity=capacity;this.onChange=onChange;this.frames=[];this.leases=new Set();this.occupied=0;this.awaitingOutput=0;this.submitted=0;this.copying=0;this.closed=false;this.failure=null;
    this.decoder=new AudioDecoder({output:frame=>{
      try{
        if(this.closed)return;
        if(!this.awaitingOutput||this.frames.length+this.leases.size>=this.capacity)throw Error('Movie PCM exceeds reserved output storage');
        this.awaitingOutput--;
        const pcm=audioDataS16(frame);
        if(pcm.sampleRate!==config.sampleRate||pcm.channels!==config.numberOfChannels)throw Error('Movie PCM format changed unexpectedly');
        this.frames.push(pcm);
      }catch(error){this.fail(error);}finally{frame.close();if(!this.closed)this.onChange();}
    },error:error=>this.fail(error)});
    this.decoder.addEventListener('dequeue',()=>{if(!this.closed)this.onChange();});this.decoder.configure(config);
  }
  fail(error){if(this.closed)return;this.failure??=error;this.onChange();}
  canSubmit(sample){
    if(this.closed||this.failure)return false;
    if(!(sample.data instanceof Uint8Array)||!sample.data.length||(sample.data[0]&1))throw Error('Expected a Vorbis audio packet');
    // Vorbis synthesis needs the first block for overlap; it produces PCM from
    // the second packet onward. Reserve each subsequent packet's output before
    // decode, including work dequeued internally but not yet delivered.
    return this.decoder.decodeQueueSize<this.capacity&&this.occupied+(this.submitted?1:0)<=this.capacity;
  }
  get available(){return !this.closed&&!this.failure&&this.occupied<this.capacity&&this.decoder.decodeQueueSize<this.capacity;}
  submit(sample){
    if(this.failure)throw this.failure;if(!this.canSubmit(sample))throw Error('Movie audio queue is full or closed');
    const outputs=this.submitted?1:0;this.occupied+=outputs;this.awaitingOutput+=outputs;
    try{this.decoder.decode(new EncodedAudioChunk({type:'key',timestamp:sample.timestamp,data:sample.data}));this.submitted++;}
    catch(error){this.occupied-=outputs;this.awaitingOutput-=outputs;throw error;}
  }
  take({retain=false}={}){if(this.failure)throw this.failure;const frame=this.frames.shift()??null;if(frame){if(retain)this.leases.add(frame);else{this.occupied--;this.onChange();}}return frame;}
  release(frame){if(!this.leases.delete(frame))throw Error('Unknown movie audio output lease');this.occupied--;this.onChange();}
  async flush(){if(this.closed)throw Error('Movie audio decoder is closed');await this.decoder.flush();if(this.failure)throw this.failure;if(this.awaitingOutput)throw Error('Movie audio decoder did not produce expected PCM');}
  close(){if(this.closed)return;this.closed=true;this.frames.length=0;this.leases.clear();this.occupied=0;this.awaitingOutput=0;if(this.decoder.state!=='closed')this.decoder.close();}
}
