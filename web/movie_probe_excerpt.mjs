// Test-only WebM muxer: a finite container around actual unchanged VP9 packets.
// Used to test natural EOF/EOS without waiting for a complete game movie. Never
// imported by runtime-worker or the production movie backend.
import {WebmDemuxer} from './movie-webm.mjs';
function join(...parts){const out=new Uint8Array(parts.reduce((n,p)=>n+p.length,0));let at=0;for(const p of parts){out.set(p,at);at+=p.length;}return out;}
function size(n){for(let width=1;width<=8;width++)if(BigInt(n)<(1n<<BigInt(width*7))-1n){let value=BigInt(n)|(1n<<BigInt(width*7));const out=new Uint8Array(width);for(let i=width-1;i>=0;i--){out[i]=Number(value&255n);value>>=8n;}return out;}throw Error('Excerpt element too large');}
function el(id,data){const hex=id.toString(16).padStart(Math.ceil(id.toString(16).length/2)*2,'0');return join(Uint8Array.from(hex.match(/../g),s=>parseInt(s,16)),size(data.length),data);}
function uint(id,n){let width=1;while(BigInt(n)>=(1n<<BigInt(8*width)))width++;const data=new Uint8Array(width);let value=BigInt(n);for(let i=width-1;i>=0;i--){data[i]=Number(value&255n);value>>=8n;}return el(id,data);}
const str=(id,s)=>el(id,new TextEncoder().encode(s));
export async function movieProbeExcerpt(source,count=12){
  if(!Number.isInteger(count)||count<1||count>3000)throw Error('Invalid movie excerpt size');
  const d=await new WebmDemuxer(source).open(),track=d.tracks.find(t=>t.type===1);
  if(track?.codecId!=='V_VP9'||track.codecPrivate.length)throw Error('Excerpt fixture requires VP9 without CodecPrivate');
  const samples=[];for await(const s of d.samples({trackNumber:track.number})){samples.push(s);if(samples.length===count)break;}
  if(samples.length!==count||!samples[0].key)throw Error('Excerpt needs a complete key-frame-led packet sequence');
  const header=el(0x1a45dfa3,join(uint(0x4286,1),uint(0x42f7,1),uint(0x42f2,4),uint(0x42f3,8),str(0x4282,'webm'),uint(0x4287,2),uint(0x4285,2)));
  const duration=new Uint8Array(8);new DataView(duration.buffer).setFloat64(0,(samples.at(-1).timestamp+(samples.at(-1).duration??0))*1000);
  const info=el(0x1549a966,join(uint(0x2ad7b1,1),el(0x4489,duration),str(0x4d80,'ssbu-local-probe'),str(0x5741,'ssbu-local-probe')));
  const tracks=el(0x1654ae6b,el(0xae,join(uint(0xd7,1),uint(0x73c5,1),uint(0x83,1),str(0x86,'V_VP9'),uint(0x23e383,track.defaultDurationNs),el(0xe0,join(uint(0xb0,track.width),uint(0xba,track.height))))));
  const clusters=samples.map(s=>el(0x1f43b675,join(uint(0xe7,s.timestamp*1000),el(0xa3,join(Uint8Array.of(0x81,0,0,s.key?128:0),s.data)))));
  return new Blob([header,el(0x18538067,join(info,tracks,...clusters))],{type:'video/webm'});
}
