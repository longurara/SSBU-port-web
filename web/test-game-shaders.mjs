import {WebGpuBackend} from './gpu-backend.mjs';

export async function testGameShaders(device) {
  const response=await fetch('./maxwell-fixtures.json');
  if(!response.ok)throw new Error(`Maxwell fixture HTTP ${response.status}`);
  const {fixtures}=await response.json();
  const backend=new WebGpuBackend(device);
  const results=[];
  for(const fixture of fixtures) {
    const bytes=Uint8Array.from(atob(fixture.bytes),character=>character.charCodeAt(0));
    const digest=Array.from(new Uint8Array(await crypto.subtle.digest('SHA-256',bytes)),b=>b.toString(16).padStart(2,'0')).join('');
    if(digest!==fixture.sha256)throw new Error(`Maxwell fixture hash mismatch: ${fixture.name}`);
    const started=performance.now();
    let handle;
    try {
      const view=new DataView(bytes.buffer),controlSize=view.getUint32(12,true),dataSize=view.getUint32(16,true);
      if(view.getUint32(0,true)!==0x3144584d||20+controlSize+dataSize!==bytes.length)throw new Error('Invalid MXD capture');
      const packet=new Uint8Array(12+controlSize+dataSize),header=new DataView(packet.buffer);
      header.setUint32(0,1,true);header.setUint32(4,controlSize,true);header.setUint32(8,dataSize,true);packet.set(bytes.subarray(20),12);
      handle=(await backend.request(12,{bytes:packet})).value;
      const [{code,metadata,layout}]=await backend.compileProgram(handle,{topology:'triangles'});
      results.push({name:fixture.name,sha256:digest,ok:true,metadata,layout,wgslBytes:code.length,milliseconds:performance.now()-started,errors:[]});
    } catch(error) {
      results.push({name:fixture.name,ok:false,error:String(error)});
    } finally {if(handle)await backend.request(13,{handle});}
  }
  const statistics={...backend.statistics};backend.close();
  return {ok:results.every(item=>item.ok),passed:results.filter(item=>item.ok).length,total:results.length,results,
    statistics,scope:'Actual captured Maxwell -> owned program sources -> Wasm translator/glslang -> SPIR-V interface lowering -> Wasm Naga -> WebGPU shader compilation; not a game pipeline or draw'};
}
