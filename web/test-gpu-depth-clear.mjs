import {WebGpuBackend} from './gpu-backend.mjs';
import {textureFormat} from './gpu-textures.mjs';

export async function testGpuDepthClear(device) {
  const backend=new WebGpuBackend(device);let checks=0;
  const check=(value,message)=>{checks++;if(!value)throw new Error(message);};
  const packet=(handle,nvn,depthMask,depth,stencil,mask,rect=[0,0,4,3])=>{
    const bytes=new Uint8Array(52),v=new DataView(bytes.buffer);
    [1,handle,0,0,depthMask,0,stencil,mask,...rect,nvn].forEach((n,i)=>v.setUint32(i*4,n,true));v.setFloat32(20,depth,true);return bytes;
  };
  const decode=(bytes,nvn,i)=>{
    const v=new DataView(bytes.buffer,bytes.byteOffset,bytes.byteLength),at=i*textureFormat(nvn).sourceBpb;
    if(nvn===0x31)return [undefined,v.getUint8(at)];
    if(nvn===0x32)return [v.getUint16(at,true)/65535,undefined];
    if(nvn===0x33)return [v.getUint32(at,true)/4294967295,undefined];
    if(nvn===0x34)return [v.getFloat32(at,true),undefined];
    if(nvn===0x35){const n=v.getUint32(at,true);return [(n>>>8)/16777215,n&255];}
    return [v.getFloat32(at,true),v.getUint32(at+4,true)];
  };
  try {
    for(const nvn of [0x31,0x32,0x33,0x34,0x35,0x36]) {
      if(nvn===0x36&&!device.features.has('depth32float-stencil8'))continue;
      const format=textureFormat(nvn),length=4*3*format.sourceBpb,texture=new Uint8Array(60+length),v=new DataView(texture.buffer);
      [0x5854564e,nvn,1,4,3,1,1,0,1,0,0,4,3,1,length].forEach((n,i)=>v.setUint32(i*4,n,true));
      const {value:handle}=await backend.request(14,{bytes:texture});
      await backend.request(25,{bytes:packet(handle,nvn,1,0.5,0xa5,255)});
      const full=(await backend.request(16,{handle,offset:0,size:0})).bytes;
      for(let i=0;i<12;i++) {
        const [depth,stencil]=decode(full,nvn,i);
        if(format.depth)check(Math.abs(depth-0.5)<0.00002,`Full clear depth ${nvn}`);
        if(format.stencil)check(stencil===0xa5,`Full clear stencil ${nvn}`);
      }
      await backend.request(25,{bytes:packet(handle,nvn,1,0.75,0x3c,0x0f,[1,1,2,1])});
      const partial=(await backend.request(16,{handle,offset:0,size:0})).bytes;
      for(let y=0;y<3;y++)for(let x=0;x<4;x++) {
        const inside=y===1&&(x===1||x===2),[depth,stencil]=decode(partial,nvn,y*4+x);
        if(format.depth)check(Math.abs(depth-(inside?0.75:0.5))<0.00002,`Partial clear depth/scissor ${nvn}`);
        if(format.stencil)check(stencil===(inside?0xac:0xa5),`Partial clear stencil/write mask ${nvn}`);
      }
      // A later invalid record must prevent the entire packet from executing.
      const before=backend.statistics.depthStencilClears,one=packet(handle,nvn,1,0.125,0,255),invalid=packet(handle,nvn,1,0.25,0,255,[4,0,1,1]);
      const batch=new Uint8Array(100);new DataView(batch.buffer).setUint32(0,2,true);batch.set(one.subarray(4),4);batch.set(invalid.subarray(4),52);
      let rejected=false;try{await backend.request(25,{bytes:batch});}catch{rejected=true;}
      check(rejected&&backend.statistics.depthStencilClears===before,'Invalid batch submitted a partial clear');
      const unchanged=(await backend.request(16,{handle,offset:0,size:0})).bytes;
      check(unchanged.every((n,i)=>n===partial[i]),'Invalid clear batch changed GPU texels');
      await backend.request(25,{bytes:packet(handle,nvn,0,0,0,0)});
      check(backend.statistics.depthStencilClears===before,'Disabled masks changed clear count');
      await backend.request(15,{handle});
    }
    return {checks,failures:0,...backend.render.statistics};
  }finally{backend.close();}
}
