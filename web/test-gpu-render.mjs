import {WebGpuBackend} from './gpu-backend.mjs';

/** Analytic color-clear readbacks on physical WebGPU targets. These packets
 * do not yet originate in a guest NVN command buffer. */
export async function testGpuRender(device) {
  const backend=new WebGpuBackend(device);let checks=0;
  function check(value,message){checks++;if(!value)throw new Error(message);}
  async function texture(format,bpp,layers=1,levels=1) {
    let length=36;
    for(let layer=0;layer<layers;layer++)for(let mip=0;mip<levels;mip++)length+=24+(4>>mip)*(4>>mip)*bpp;
    const bytes=new Uint8Array(length),v=new DataView(bytes.buffer),put=(at,value)=>v.setUint32(at,value,true);
    [0x5854564e,format,layers===1?1:3,4,4,layers,levels,0,layers*levels].forEach((value,i)=>put(i*4,value));
    let at=36;
    for(let layer=0;layer<layers;layer++)for(let mip=0;mip<levels;mip++) {
      const w=4>>mip,h=4>>mip,size=w*h*bpp;
      [mip,layer,w,h,1,size].forEach((value,i)=>put(at+i*4,value));at+=24+size;
    }
    return (await backend.request(14,{bytes})).value;
  }
  function packet(commands,legacy=false) {
    const stride=legacy?56:60,bytes=new Uint8Array(4+commands.length*stride),v=new DataView(bytes.buffer);v.setUint32(0,commands.length,true);
    commands.forEach(({handle,mip=0,layer=0,kind=0,mask=15,rect,value,viewFormat=0},i)=>{
      const at=4+i*stride;
      [handle,mip,layer,kind,mask,rect?1:0,...(rect||[0,0,0,0])].forEach((item,j)=>v.setUint32(at+j*4,item,true));
      value.forEach((item,j)=>v[kind===0?'setFloat32':kind===1?'setInt32':'setUint32'](at+40+j*4,item,true));
      if(!legacy)v.setUint32(at+56,viewFormat,true);
    });return bytes;
  }
  async function read(handle,kind,mip=0,layer=0) {
    const {bytes}=await backend.request(16,{handle,offset:mip,size:layer});
    return kind===1?new Int32Array(bytes.buffer):kind===2?new Uint32Array(bytes.buffer):bytes;
  }
  async function rejected(bytes,expression) {
    let error;try{await backend.request(22,{bytes});}catch(cause){error=cause;}
    check(error&&expression.test(String(error)),`Expected rejection: ${expression}`);
  }
  try {
    for(const kind of [0,1,2]) {
      const handle=await texture([0x25,0x30,0x2f][kind],kind?16:4);
      const base=kind===1?[-100,-200,-300,-400]:kind===2?[0xffffffff,0xfffffffe,300,400]:[0,0,0,1];
      const color=kind===1?[-7,12345,-8000,42]:kind===2?[0x80000000,70000,0xfffffffd,1]:[1,0,1,0];
      for(let mask=0;mask<16;mask++) {
        await backend.request(22,{bytes:packet([{handle,kind,value:base},{handle,kind,mask,rect:[1,1,2,2],value:color}],kind===0&&mask===0)});
        const result=await read(handle,kind);
        for(let y=0;y<4;y++)for(let x=0;x<4;x++)for(let channel=0;channel<4;channel++) {
          const selected=x>=1&&x<3&&y>=1&&y<3&&(mask&(1<<channel));
          const expected=(selected?color:base)[channel]*(kind?1:255);
          check(result[(y*4+x)*4+channel]===expected,`Clear mismatch kind=${kind} mask=${mask} pixel=${x},${y} channel=${channel}`);
        }
      }
      await backend.request(15,{handle});
    }
    for(const kind of [0,2]) {
      const rgb=await texture(kind?0x23:0x22,12),value=kind?[0xffffffff,123,100,9]:[-4,0.5,8,999];
      await backend.request(22,{bytes:packet([{handle:rgb,kind,value}])});
      const bytes=(await backend.request(16,{handle:rgb})).bytes;
      const result=kind?new Uint32Array(bytes.buffer):new Float32Array(bytes.buffer);
      result.forEach((item,i)=>check(item===(i%4===3?1:value[i%4]),'Expanded RGB clear changed implicit alpha'));
      await backend.request(15,{handle:rgb});
    }
    const floating=await texture(0x2e,16);
    await backend.request(22,{bytes:packet([{handle:floating,value:[-8,0.125,32,1]},{handle:floating,mask:5,rect:[1,1,2,2],value:[-2,0,0.5,0]}])});
    const floatingBytes=(await backend.request(16,{handle:floating})).bytes;
    new Float32Array(floatingBytes.buffer).forEach((value,i)=>{
      const pixel=Math.floor(i/4),x=pixel%4,y=Math.floor(pixel/4),inside=x>=1&&x<3&&y>=1&&y<3;
      check(value===(inside?[-2,0.125,0.5,1]:[-8,0.125,32,1])[i%4],'Float target was clamped or lost channel precision');
    });
    await backend.request(15,{handle:floating});
    const srgb=await texture(0x37,4);
    await backend.request(22,{bytes:packet([{handle:srgb,value:[0.5,0.25,0.75,1]},{handle:srgb,mask:2,value:[0,0.5,0,0]}])});
    (await read(srgb,0)).forEach((value,i)=>check(Math.abs(value-[188,188,225,255][i%4])<=1,'sRGB clear was not encoded from linear values'));
    await backend.request(15,{handle:srgb});
    const handle=await texture(0x25,4,2,2);
    await backend.request(22,{bytes:packet([{handle,mip:1,layer:0,viewFormat:0x38,value:[0.5,0.25,0.75,1]}])});
    (await read(handle,0,1,0)).forEach((item,i)=>check(Math.abs(item-[188,137,225,255][i%4])<=1,'sRGB view clear ignored its view format'));
    await backend.request(22,{bytes:packet([{handle,mip:1,layer:0,viewFormat:0x25,value:[0,0,0,0]}])});
    await rejected(packet([{handle,viewFormat:0x2f,kind:2,value:[0,0,0,0]}]),/reinterpretation/);
    await backend.request(22,{bytes:packet([{handle,mip:1,layer:1,value:[1,1,0,1]}])});
    const selected=await read(handle,0,1,1);
    selected.forEach((value,i)=>check(value===[255,255,0,255][i%4],'Selected mip/layer clear differs'));
    for(const [mip,layer]of [[0,0],[1,0],[0,1]])(await read(handle,0,mip,layer)).forEach(value=>check(value===0,'Clear touched another subresource'));
    const first={handle,value:[0,1,0,1]},invalid={handle,mask:16,value:[1,0,0,1]};
    await rejected(packet([first,invalid]),/mask/);
    (await read(handle,0)).forEach(value=>check(value===0,'Rejected batch changed target pixels'));
    await rejected(packet([{handle,kind:2,value:[0,0,0,0]}]),/type/);
    await rejected(packet([{handle,mip:9,value:[0,0,0,0]}]),/subresource/);
    await rejected(packet([{handle,rect:[3,0,2,1],value:[0,0,0,0]}]),/scissor/);
    await rejected(new Uint8Array(3),/packet/);
    await backend.request(22,{bytes:packet([{handle,value:[1,0,0,1]},{handle,mask:2,value:[0,1,0,0]},{handle,mask:4,value:[0,0,1,0]}])});
    (await read(handle,0)).forEach(value=>check(value===255,'Ordered clears did not preserve other channels'));
    await backend.request(22,{bytes:packet([{handle,rect:[0,0,0,0],value:[0,0,0,0]}])});
    (await read(handle,0)).forEach(value=>check(value===255,'Empty scissor changed pixels'));
    await backend.request(15,{handle});
    check(backend.statistics.liveTextures===0,'Clear fixture leaked textures');
    const stats={...backend.statistics};
    backend.close();
    await rejected(packet([{handle,value:[0,0,0,0]}]),/closed/);
    return {ok:true,checks,statistics:stats,
      scope:'Analytic WebGPU render-target clears: all RGBA masks on normalized/signed/unsigned targets, scissor, mip/layer isolation, ordered batch and rejection readbacks. No guest NVN command encoding, game draw or native pixel equivalence.'};
  }finally{backend.close();}
}
