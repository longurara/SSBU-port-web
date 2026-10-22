import create from './host-fiber/probe.mjs';
let module;
try{
  let complete;
  module=await create({onHostFiberReport:text=>complete(JSON.parse(text))});
  const rounds=[];
  for(let round=0;round<3;round++){
  const completed=new Promise(resolve=>{complete=resolve;});
  if(module._host_fiber_start())throw new Error('Host fiber pthread creation failed');
  const result=await completed;
  if(module._host_fiber_join())throw new Error('Host fiber pthread join failed');
  rounds.push(result);await new Promise(resolve=>setTimeout(resolve,10));
  }
  const ids=rounds[0].workerIds.toSorted().join(',');
  if(rounds.some(result=>!result.ok||result.stage!==6||result.workerIds.toSorted().join(',')!==ids))throw new Error('Fiber worker reuse mismatch');
  postMessage({type:'complete',result:{ok:true,checks:rounds.reduce((sum,result)=>sum+result.checks,0),stage:6,rounds}});
}catch(error){postMessage({type:'error',error:String(error.stack||error)});}
finally{module?.PThread.terminateAllThreads();}
