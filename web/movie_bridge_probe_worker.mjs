import create from './probe.mjs';
import {MovieBackend} from '../movie-backend.mjs';
self.onmessage=async({data})=>{
  let module,message;const notifications=[];
  const backend=new MovieBackend({notify:(handle,mask)=>notifications.push({handle,mask})});
  try{
    if(!(data.file instanceof Blob))throw Error('Missing local movie blob');
    let finish;const report=new Promise(resolve=>finish=resolve);
    module=await create({webmovieDispatch:request=>backend.dispatch(module,request),onMovieProbeReport:text=>finish(JSON.parse(text))});
    module.FS.mkdir('/movie');module.FS.mount(module.WORKERFS,{blobs:[{name:'input.webm',data:data.file}]},'/movie');
    if(module._movie_bridge_start(data.limit??12,+!!data.expectEof))throw Error('Cannot start movie bridge pthread');
    const result=await report;
    module._movie_bridge_join();result.notifications=notifications.length;result.liveHandles=backend.entries.size;result.pendingCompletions=backend.pending.size;
    message={result};
  }catch(error){message={error:String(error),bridgeError:module?.webmovieError};}
  finally{await backend.close();module?.PThread?.terminateAllThreads();}
  self.postMessage(message);
};
