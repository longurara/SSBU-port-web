import create from './probe.mjs';
import {MovieBackend,MovieOp} from '../movie-backend.mjs';
self.onmessage=async({data})=>{
  let module,message;const notifications=[];
  const backend=new MovieBackend({notify:(handle,mask)=>{notifications.push({handle,mask});module?._web_movie_notify?.(handle,mask);}});
  try{
    if(!(data.file instanceof Blob))throw Error('Missing local movie blob');
    let finish;const report=new Promise(resolve=>finish=resolve);
    module=await create({webmovieDispatch:async request=>{
      if(request.operation===MovieOp.STATUS&&module._movie_decoder_race_pending?.()===1){
        module._movie_decoder_race_entered();const deadline=Date.now()+5000;
        // Deterministically complete the in-flight SDK call only after its
        // destructor has removed the object and is waiting for that call.
        while(module._movie_decoder_race_live()){
          if(Date.now()>deadline)throw Error('Concurrent Decoder destructor did not unlist its object');
          await new Promise(resolve=>setTimeout(resolve,1));
        }
      }
      return backend.dispatch(module,request);
    },onMovieProbeReport:text=>finish(JSON.parse(text))});
    module.FS.mkdir('/movie');module.FS.mount(module.WORKERFS,{blobs:[{name:'input.webm',data:data.file}]},'/movie');
    if(module._movie_bridge_start(data.limit??12,+!!data.expectEof))throw Error('Cannot start movie bridge pthread');
    const result=await report;
    module._movie_bridge_join();result.notifications=notifications.length;result.liveHandles=backend.entries.size;result.pendingCompletions=backend.pending.size;
    message={result};
  }catch(error){message={error:String(error),bridgeError:module?.webmovieError};}
  finally{await backend.close();module?.PThread?.terminateAllThreads();}
  self.postMessage(message);
};
