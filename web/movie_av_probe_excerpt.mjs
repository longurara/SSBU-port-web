// Test-only finite WebM containing unchanged, interleaved real media packets.
// All original track numbers and Vorbis headers are preserved. Never used by game.
import {WebmDemuxer} from './movie-webm.mjs';
function join(...parts){const out=new Uint8Array(parts.reduce((n,p)=>n+p.length,0));let at=0;for(const p of parts){out.set(p,at);at+=p.length;}return out;}
function size(n){for(let width=1;width<=8;width++)if(BigInt(n)<(1n<<BigInt(width*7))-1n){let value=BigInt(n)|(1n<<BigInt(width*7));const out=new Uint8Array(width);for(let i=width-1;i>=0;i--){out[i]=Number(value&255n);value>>=8n;}return out;}throw Error('Excerpt element too large');}
function el(id,data){const hex=id.toString(16).padStart(Math.ceil(id.toString(16).length/2)*2,'0');return join(Uint8Array.from(hex.match(/../g),s=>parseInt(s,16)),size(data.length),data);}
function uint(id,n){let width=1;while(BigInt(n)>=(1n<<BigInt(8*width)))width++;const bytes=new Uint8Array(width);let value=BigInt(n);for(let i=width-1;i>=0;i--){bytes[i]=Number(value&255n);value>>=8n;}return el(id,bytes);}
const str=(id,s)=>el(id,new TextEncoder().encode(s));
function float(id,n){const bytes=new Uint8Array(8);new DataView(bytes.buffer).setFloat64(0,n);return el(id,bytes);}
export async function movieAvExcerpt(source,limit=120){
  if(!Number.isInteger(limit)||limit<2||limit>128)throw Error('Invalid AV excerpt count');
  const demux=await new WebmDemuxer(source).open(),video=demux.tracks.find(t=>t.type===1),audio=demux.tracks.filter(t=>t.codecId==='A_VORBIS').at(-1);
  if(video?.codecId!=='V_VP9'||!audio||demux.tracks.some(t=>t.number>126))throw Error('AV fixture needs VP9/Vorbis tracks');
  const samples=[];let videos=0;
  for await(const sample of demux.samples()){samples.push(sample);if(sample.trackNumber===video.number&&++videos===limit)break;}
  if(videos!==limit||!samples.find(s=>s.trackNumber===video.number)?.key)throw Error('Missing complete key-frame-led video sequence');
  const header=el(0x1a45dfa3,join(uint(0x4286,1),uint(0x42f7,1),uint(0x42f2,4),uint(0x42f3,8),str(0x4282,'webm'),uint(0x4287,2),uint(0x4285,2)));
  const duration=Math.max(...samples.map(s=>s.timestamp+(s.duration??0)))*1000;
  const info=el(0x1549a966,join(uint(0x2ad7b1,1),float(0x4489,duration)));
  const tracks=el(0x1654ae6b,join(...demux.tracks.map(t=>el(0xae,join(uint(0xd7,t.number),uint(0x73c5,t.number),uint(0x83,t.type),str(0x86,t.codecId),
    ...(t.codecPrivate.length?[el(0x63a2,t.codecPrivate)]:[]),...(t.defaultDurationNs?[uint(0x23e383,t.defaultDurationNs)]:[]),
    t.type===1?el(0xe0,join(uint(0xb0,t.width),uint(0xba,t.height))):el(0xe1,join(float(0xb5,t.sampleRate),uint(0x9f,t.channels))))))));
  const clusters=samples.map(s=>el(0x1f43b675,join(uint(0xe7,s.timestamp*1000),el(0xa3,join(Uint8Array.of(0x80|s.trackNumber,0,0,s.key?128:0),s.data)))));
  return {blob:new Blob([header,el(0x18538067,join(info,tracks,...clusters))],{type:'video/webm'}),videoTrack:video.number,audioTrack:audio.number};
}
