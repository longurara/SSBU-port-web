import {WebGpuBackend} from './gpu-backend.mjs';

/** Read actual presented pixels, including crop orientation and sRGB decode /
 * encode. This is a presentation fixture, not proof of game draw execution. */
export async function testGpuPresent(device) {
  const canvas=new OffscreenCanvas(4,3),backend=new WebGpuBackend(device,{canvas,diagnosticFrames:true});
  let checks=0;
  const check=(value,message)=>{checks++;if(!value)throw new Error(message);};
  const crop=(x=0,y=0,w=0,h=0)=>new Uint8Array(new Int32Array([x,y,w,h]).buffer);
  const pixels=new Uint8Array(4*3*4);
  for(let y=0;y<3;y++)for(let x=0;x<4;x++)pixels.set([x*60,y*80,128,255],(y*4+x)*4);
  // rgba8 sRGB NVN format, one 4x3 subresource.
  const bytes=new Uint8Array(36+24+pixels.length),view=new DataView(bytes.buffer);
  [0x5854564e,0x38,1,4,3,1,1,0,1].forEach((n,i)=>view.setUint32(i*4,n,true));
  [0,0,4,3,1,pixels.length].forEach((n,i)=>view.setUint32(36+i*4,n,true));
  bytes.set(pixels,60);
  try {
    const {value:handle}=await backend.request(14,{bytes});
    for(const [x,y,w,h] of [[0,0,4,3],[1,1,2,2],[0,0,4,3]]) {
      await backend.request(23,{handle,bytes:crop(x,y,w,h)});
      check(canvas.width===w&&canvas.height===h,'Presented size differs from crop');
      const image=canvas.transferToImageBitmap(),copy=new OffscreenCanvas(w,h),context=copy.getContext('2d');
      context.drawImage(image,0,0);image.close();
      const result=context.getImageData(0,0,w,h).data;
      for(let row=0;row<h;row++)for(let col=0;col<w;col++)for(let c=0;c<4;c++)
        check(Math.abs(result[(row*w+col)*4+c]-pixels[((row+y)*4+col+x)*4+c])<=1,'Presentation pixel/crop/sRGB mismatch');
    }
    const before=backend.statistics.framesPresented;
    for(const bad of [crop(-1,0,1,1),crop(3,2,2,2),new Uint8Array(12)]) {
      let rejected=false;try{await backend.request(23,{handle,bytes:bad});}catch{rejected=true;}
      check(rejected,'Invalid presentation was accepted');
    }
    check(backend.statistics.framesPresented===before,'Invalid present advanced frame count');
    await backend.request(23,{handle,bytes:crop()});
    check(backend.statistics.framesPresented===4,'Default full crop or frame count failed');
    const samples=backend.render.frameDiagnostics.samples;
    check(samples.length===4&&!backend.render.frameDiagnostics.lastError,'Presentation diagnostics missing or failed');
    check(samples[0].nonBlackPixels===12&&samples[1].nonBlackPixels===4,'Presentation diagnostic ignored crop');
    check(JSON.stringify(samples[1].minimum)==='[60,80,128]'&&JSON.stringify(samples[1].maximum)==='[120,160,128]','Diagnostic encoded extrema differ from actual source pixels');
    check(samples[0].hash===samples[2].hash&&samples[0].hash===samples[3].hash,'Identical source crops changed diagnostic hash');
    await backend.request(15,{handle});
    let stale=false;try{await backend.request(23,{handle,bytes:crop()});}catch{stale=true;}
    check(stale,'Destroyed window texture was presented');
    return {checks,failures:0,framesPresented:backend.statistics.framesPresented};
  } finally {backend.close();}
}

/** Actual GPU source readbacks distinguish alpha-only black and cropped-out
 * color, including BGRA ordering and 10-bit precision. */
export async function testGpuPresentDiagnostics(device) {
  let checks=0;const check=(value,message)=>{checks++;if(!value)throw new Error(message);};
  for(const [format,rgba] of [[0x25,[1,2,3]],[0x99,[3,2,1]],[0x3d,[1,2,1023]]]) {
    const backend=new WebGpuBackend(device,{canvas:new OffscreenCanvas(4,3),diagnosticFrames:true});
    try {
      const bytes=new Uint8Array(108),v=new DataView(bytes.buffer);
      [0x5854564e,format,1,4,3,1,1,0,1,0,0,4,3,1,48].forEach((n,i)=>v.setUint32(i*4,n,true));
      for(let i=60;i<108;i+=4)v.setUint32(i,format===0x3d?0xc0000000:0xff000000,true);
      if(format===0x3d)v.setUint32(60,0xc0000000|(1023<<20)|(2<<10)|1,true);
      else bytes.set([1,2,3,255],60);
      const {value:handle}=await backend.request(14,{bytes});
      await backend.request(23,{handle,bytes:new Uint8Array(new Int32Array([1,1,2,2]).buffer)});
      const black=backend.render.frameDiagnostics.samples[0];
      check(black.nonBlackPixels===0&&black.bounds===null,'Alpha or cropped-out color counted as visible RGB');
      check(black.minimum.every(n=>n===0)&&black.maximum.every(n=>n===0),'Black source extrema differ');
      await backend.request(23,{handle,bytes:new Uint8Array(16)});
      const color=backend.render.frameDiagnostics.samples[1];
      check(color.nonBlackPixels===1&&JSON.stringify(color.bounds)==='[0,0,1,1]','Nonblack source bounds differ');
      check(JSON.stringify(color.maximum)===JSON.stringify(rgba),'Diagnostic lost channel order or 10-bit precision');
      check(color.encodedChannelMaximum===(format===0x3d?1023:255),'Diagnostic channel scale differs');
      check(!backend.render.frameDiagnostics.lastError,'GPU source diagnostic failed');
      const read=backend.textures.read;
      backend.textures.read=async()=>{throw new Error('injected diagnostic read failure');};
      await backend.request(23,{handle,bytes:new Uint8Array(16)});
      check(backend.statistics.framesPresented===3&&backend.render.frameDiagnostics.lastError.includes('injected'),'Diagnostic failure interrupted presentation');
      backend.textures.read=read;
      await backend.request(15,{handle});
    }finally{backend.close();}
  }
  const ordinary=new WebGpuBackend(device);
  check(ordinary.render.frameDiagnostics===undefined,'Frame diagnostic enabled by default');ordinary.close();
  return {ok:true,checks,scope:'GPU presentation-source diagnostics only; not game frame or input proof'};
}
