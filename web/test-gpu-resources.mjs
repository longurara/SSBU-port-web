export async function testGpuResources(options={}) {
  const worker=new Worker('./gpu-probe-worker.mjs',{type:'module'});
  try {
    return await new Promise((resolve,reject)=> {
      const timeout=setTimeout(()=>reject(new Error('NVN/WebGPU resource fixture timed out')),30000);
      worker.onmessage=({data})=> {clearTimeout(timeout);if(data.error)reject(new Error(data.error));else resolve(data.result);};
      worker.onerror=error=>{clearTimeout(timeout);reject(new Error(error.message));};
      worker.postMessage({withSdk:options.withSdk===true,graphicsFixture:options.graphicsFixture===true});
    });
  }finally{worker.terminate();}
}
