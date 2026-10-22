/** Diagnostic compilation of actual registered programs at a boot boundary.
 * Explicit options are a probe variant, not inferred native draw state. */
export async function inspectGpuPrograms(backend,options) {
  const entries=Array.from(backend.programs.records);
  if(entries.length>64||entries.reduce((sum,[,record])=>sum+record.stages.length,0)>256)
    throw new Error('Program inspection exceeds its bounded diagnostic set');
  const results=[];
  for(const [handle,record] of entries) {
    const result={handle,sourceTypes:record.stages.map(stage=>stage.type)};
    try {
      const stages=await backend.compileProgram(handle,options);
      result.ok=true;
      result.stages=stages.map(stage=>({metadata:stage.metadata,layout:stage.layout}));
      if(record.stages.length===1&&record.stages[0].type===6){
        try{result.computePipeline={ok:true,...await backend.checkComputeProgram(handle,options)};}
        catch(error){result.computePipeline={ok:false,error:String(error.message||error).slice(0,4096)};}
      }
    }catch(error){result.ok=false;result.error=String(error.message||error).slice(0,4096);}
    results.push(result);
  }
  const vertices=results.filter(result=>result.ok&&result.sourceTypes.length===1&&result.sourceTypes[0]===1);
  const fragments=results.filter(result=>result.ok&&result.sourceTypes.length===1&&result.sourceTypes[0]===5);
  const pairs=vertices.flatMap(vertex=>fragments.filter(fragment=>{
    const output=vertex.stages[0].metadata.outputAttributes>>>0,input=fragment.stages[0].metadata.inputAttributes>>>0;
    return ((output&input)>>>0)===input;
  }).map(fragment=>({vertex,fragment,cost:vertex.stages[0].metadata.instructions+fragment.stages[0].metadata.instructions})));
  pairs.sort((a,b)=>a.cost-b.cost);let graphicsPipeline;
  if(pairs.length){
    const {vertex,fragment}=pairs[0];
    try{graphicsPipeline={ok:true,...await backend.checkGraphicsPrograms(vertex.handle,fragment.handle,options)};}
    catch(error){graphicsPipeline={ok:false,vertexHandle:vertex.handle,fragmentHandle:fragment.handle,error:String(error.message||error).slice(0,4096)};}
  }
  return {options,programsExamined:results.length,successfulPrograms:results.filter(result=>result.ok).length,
    compiledStages:results.reduce((sum,result)=>sum+(result.stages?.length||0),0),
    computePipelines:results.filter(result=>result.computePipeline?.ok).length,graphicsPipeline,results,
    scope:'Diagnostic variants of actual programs registered by this boot; explicit triangle/float interpretation; graphics pair selected by interface masks with synthetic vertex layout, not observed native binding; no dispatch/draw/native pixel comparison'};
}
