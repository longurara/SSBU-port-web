import create from './fiber-migration/probe.mjs';
let module;
try {
  let complete;
  const completed=new Promise(resolve=>{complete=resolve;});
  module=await create({onMigrationReport:text=>complete(JSON.parse(text))});
  if(module._migration_start())throw new Error('pthread creation failed');
  const result=await completed;
  if(module._migration_join())throw new Error('pthread join failed');
  postMessage({type:'complete',result});
}catch(error){postMessage({type:'error',error:String(error.stack||error)});}
finally{module?.PThread.terminateAllThreads();}
