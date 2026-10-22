/** Read the bounded entry-point interface emitted by our Naga compiler.
 * This is deliberately not a general WGSL parser or a native vertex layout. */
function argumentList(code,open) {
  let depth=1;
  for(let end=open+1;end<code.length;end++){
    if(code[end]==='(')depth++;
    if(code[end]===')'&&!--depth)return code.slice(open+1,end);
  }
  throw new Error('Unterminated WGSL entry-point arguments');
}
function splitFields(text){
  const fields=[];let start=0,depth=0;
  for(let i=0;i<text.length;i++){
    if(text[i]==='('||text[i]==='<')depth++;
    if(text[i]===')'||text[i]==='>')depth--;
    if(depth<0)throw new Error('Invalid WGSL interface delimiters');
    if(text[i]===','&&!depth){fields.push(text.slice(start,i).trim());start=i+1;}
  }
  if(depth)throw new Error('Invalid WGSL interface delimiters');
  fields.push(text.slice(start).trim());return fields.filter(Boolean);
}
export function stageInterface(code,stage){
  if(typeof code!=='string'||code.length>16*1024*1024||!['vertex','fragment','compute'].includes(stage))throw new Error('Invalid WGSL interface input');
  const tags=Array.from(code.matchAll(new RegExp(`@${stage}\\b`,'g')));
  if(tags.length!==1)throw new Error('WGSL stage entry point is ambiguous');
  const rest=code.slice(tags[0].index),signature=rest.match(/^@\w+\b[\s\S]*?\bfn\s+([A-Za-z_]\w*)\s*\(/);
  if(!signature||signature[0].includes('{'))throw new Error('Unsupported WGSL entry-point signature');
  const args=argumentList(rest,signature[0].length-1),locations=[],outputs=[];
  function fields(text,allowStruct,destination=locations){
    for(const field of splitFields(text)){
      const location=field.match(/@location\(\s*(\d+)\s*\)/);
      if(/@builtin\(/.test(field))continue;
      const typed=field.match(/\b([A-Za-z_]\w*)\s*:\s*([\s\S]+)$/);
      if(!typed)throw new Error('Unsupported WGSL interface field');
      const type=typed[2].replace(/@\w+\s*\([^)]*\)\s*/g,'').replace(/\s/g,'');
      if(!location){
        if(!allowStruct||!/^[A-Za-z_]\w*$/.test(type))throw new Error('WGSL input lacks a location');
        const struct=code.match(new RegExp(`\\bstruct\\s+${type}\\s*\\{([^}]*)\\}`));
        if(!struct)throw new Error('Missing WGSL input struct');fields(struct[1],false,destination);continue;
      }
      const value=Number(location[1]),vector=type.match(/^vec([234])<(f32|i32|u32)>$/),scalar=type.match(/^(f32|i32|u32)$/);
      if(!Number.isSafeInteger(value)||value>31||(!vector&&!scalar))throw new Error('Unsupported WGSL vertex input type/location');
      const components=vector?Number(vector[1]):1,kind=vector?vector[2]:scalar[1];
      destination.push({shaderLocation:value,components,kind});
    }
  }
  if(stage==='vertex')fields(args,true);
  if(stage==='fragment'){
    const tail=rest.slice(signature[0].length+args.length+1).match(/^\s*(?:->\s*([^{}]+))?\s*\{/);
    if(!tail)throw new Error('Unsupported WGSL fragment return signature');
    if(tail[1])fields(`result:${tail[1].trim()}`,true,outputs);
  }
  locations.sort((a,b)=>a.shaderLocation-b.shaderLocation);
  if(new Set(locations.map(item=>item.shaderLocation)).size!==locations.length)throw new Error('Duplicate WGSL vertex input location');
  outputs.sort((a,b)=>a.shaderLocation-b.shaderLocation);
  if(new Set(outputs.map(item=>item.shaderLocation)).size!==outputs.length)throw new Error('Duplicate WGSL fragment output location');
  return {entryPoint:signature[1],locations,outputs};
}
export function diagnosticVertexLayout(code){
  const {entryPoint,locations}=stageInterface(code,'vertex');let stride=0;
  const attributes=locations.map(({shaderLocation,components,kind})=>{
    const format=({f32:'float32',i32:'sint32',u32:'uint32'})[kind]+(components===1?'':`x${components}`);
    const attribute={shaderLocation,offset:stride,format};stride+=components*4;return attribute;
  });
  return {entryPoint,buffers:attributes.length?[{arrayStride:stride,stepMode:'vertex',attributes}]:[]};
}
