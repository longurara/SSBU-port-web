import create from './mutex/probe.mjs';
let module;
try{
  let finish,fail;
  const done=new Promise((resolve,reject)=>{finish=resolve;fail=reject;});
  module=await create({onMutexReport:text=>finish(JSON.parse(text)),onAbort:reason=>fail(new Error(String(reason)))});
  if(module._mutex_probe_start())throw new Error('Cannot start mutex fixture');
  const result=await done;
  if(module._mutex_probe_join())throw new Error('Cannot join mutex fixture');
  self.postMessage({type:'complete',result});
}catch(error){self.postMessage({type:'error',error:String(error)});}
finally{module?.PThread.terminateAllThreads();self.close();}
