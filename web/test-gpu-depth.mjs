import {WebGpuBackend} from './gpu-backend.mjs';

/** Synthetic source encodings follow runtime/tex_decode.c's native GL upload
 * types. D24 comparisons allow its specified opaque physical precision. */
export async function testGpuDepth(device) {
  const backend=new WebGpuBackend(device);let checks=0;
  function check(value,message){checks++;if(!value)throw new Error(message);}
  const bpp=nvn=>[1,2,4,4,4,8][nvn-0x31];
  function source(nvn,w,h,seed) {
    const bytes=new Uint8Array(w*h*bpp(nvn)),v=new DataView(bytes.buffer);
    for(let i=0;i<w*h;i++){
      const at=i*bpp(nvn),n=(i+seed)%7;
      if(nvn===0x31)v.setUint8(at,(i*31+seed)&255);
      if(nvn===0x32)v.setUint16(at,[0,1,65535,32768,8192,54321,12345][n],true);
      if(nvn===0x33)v.setUint32(at,[0,0xffffffff,0x80000000,0x12345678,0x76543210,0xeeeeeeee,0x01010101][n],true);
      if(nvn===0x34||nvn===0x36)v.setFloat32(at,[0,1,0.5,0.125,0.75,0.9,0.3333333][n],true);
      if(nvn===0x35)v.setUint32(at,(([0,0xffffff,0x800000,0x123456,0x765432,0xeeeeee,0x010101][n]<<8)|((i*31+seed)&255))>>>0,true);
      if(nvn===0x36)v.setUint32(at+4,(i*31+seed)&255,true);
    }return bytes;
  }
  function packet(nvn) {
    const items=[];let length=36;
    for(let layer=0;layer<2;layer++)for(let mip=0;mip<2;mip++){
      const w=Math.max(1,5>>mip),h=Math.max(1,3>>mip),bytes=source(nvn,w,h,layer*11+mip*29);items.push({mip,layer,w,h,bytes});length+=24+bytes.length;
    }
    const bytes=new Uint8Array(length),v=new DataView(bytes.buffer);[0x5854564e,nvn,4,5,3,2,2,0,4].forEach((value,i)=>v.setUint32(i*4,value,true));let at=36;
    for(const item of items){[item.mip,item.layer,item.w,item.h,1,item.bytes.length].forEach((value,i)=>v.setUint32(at+i*4,value,true));at+=24;bytes.set(item.bytes,at);at+=item.bytes.length;}
    return {bytes,items};
  }
  function equal(nvn,actual,expected) {
    check(actual.length===expected.length,'Depth readback length differs');
    const a=new DataView(actual.buffer,actual.byteOffset,actual.byteLength),e=new DataView(expected.buffer,expected.byteOffset,expected.byteLength);
    for(let i=0;i<actual.length;i+=bpp(nvn)) {
      if(nvn===0x31)check(a.getUint8(i)===e.getUint8(i),'Stencil upload/readback differs');
      if(nvn===0x32)check(a.getUint16(i,true)===e.getUint16(i,true),'Depth16 upload/readback differs');
      if(nvn===0x33)check(Math.abs(a.getUint32(i,true)-e.getUint32(i,true))<=512,'Depth24 exceeds D24 precision tolerance');
      if(nvn===0x34||nvn===0x36)check(a.getFloat32(i,true)===e.getFloat32(i,true),'Depth32float source changed');
      if(nvn===0x35){check(Math.abs((a.getUint32(i,true)>>>8)-(e.getUint32(i,true)>>>8))<=1,'Packed D24 depth changed');check(a.getUint8(i)===e.getUint8(i),'Packed D24 stencil changed');}
      if(nvn===0x36)check(a.getUint32(i+4,true)===e.getUint32(i+4,true),'Packed D32 stencil changed');
    }
  }
  const formats=[],unavailable=[];
  try {
    for(let nvn=0x31;nvn<=0x36;nvn++) {
      if(nvn===0x36&&!device.features.has('depth32float-stencil8')){unavailable.push(nvn);continue;}
      const {bytes,items}=packet(nvn),handle=(await backend.request(14,{bytes})).value;
      for(const item of items)equal(nvn,(await backend.request(16,{handle,offset:item.mip,size:item.layer})).bytes,item.bytes);
      const view=new Uint8Array(18);view[0]=0x80;
      const depthView=(await backend.request(18,{handle,bytes:view})).value;
      check(backend.textures.getView(depthView).descriptor.aspect===(nvn>=0x35?'depth-only':'all'),'Default depth view aspect differs');
      await backend.request(19,{handle:depthView});
      if(nvn>=0x35){view[0]=0x90;view[2]=1;const stencilView=(await backend.request(18,{handle,bytes:view})).value;
        check(backend.textures.getView(stencilView).descriptor.format==='stencil8','Stencil view format differs');await backend.request(19,{handle:stencilView});}
      if(nvn===0x34||nvn===0x36){
        const step=bpp(nvn),bad=new Uint8Array(4+(36+step)*2),bv=new DataView(bad.buffer),valid=source(nvn,1,1,77);
        bv.setUint32(0,2,true);
        [handle,0,0,0,0,1,1,1,step].forEach((value,i)=>bv.setUint32(4+i*4,value,true));bad.set(valid,40);
        [handle,0,1,0,0,1,1,1,step].forEach((value,i)=>bv.setUint32(40+step+i*4,value,true));bv.setUint32(76+step,0x7fc00000,true);
        const beforeCopies=backend.statistics.bufferTextureCopies;
        let rejected=false;
        try{await backend.request(17,{bytes:bad});}catch(error){rejected=/finite \[0,1\]/.test(String(error));}
        check(rejected,'Invalid NaN depth copy batch did not fail fast');
        check(backend.statistics.bufferTextureCopies===beforeCopies,'Rejected depth copy batch changed statistics');
        equal(nvn,(await backend.request(16,{handle,offset:0,size:0})).bytes,items[0].bytes);
      }
      const replacements=[source(nvn,3,2,99),source(nvn,3,2,100)],copy=new Uint8Array(40+replacements[0].length*2),v=new DataView(copy.buffer);
      [1,handle,0,1,1,0,3,2,2,replacements[0].length*2].forEach((value,i)=>v.setUint32(i*4,value,true));copy.set(replacements[0],40);copy.set(replacements[1],40+replacements[0].length);
      await backend.request(17,{bytes:copy});
      for(const item of items){
        const expected=item.bytes.slice();if(item.mip===0)for(let row=0;row<2;row++)expected.set(replacements[item.layer].subarray(row*3*bpp(nvn),(row+1)*3*bpp(nvn)),((row+1)*5+1)*bpp(nvn));
        equal(nvn,(await backend.request(16,{handle,offset:item.mip,size:item.layer})).bytes,expected);
      }
      await backend.request(15,{handle});formats.push(nvn);
    }
    check(backend.statistics.liveTextures===0&&backend.statistics.liveTextureViews===0,'Depth fixture leaked resources');
    return {ok:true,checks,formats,unavailable,statistics:{...backend.statistics},
      scope:'Synthetic native-backend depth/stencil source encodings through actual WebGPU upload, mip/layer reads, aspect views and partial copy preservation. D24 is compared within opaque-format precision; no game draw or native GPU parity.'};
  }finally{backend.close();}
}
