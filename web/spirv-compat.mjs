/** SPIR-V interface lowering for WebGPU. Instruction semantics are preserved;
 * point size is ignored only when the caller explicitly selects triangles. */
function zeroViewportOutputs(ops,types,builtins,instanceBase){
  const outputVariables=new Set(ops.filter(([op,a])=>op===59&&a[2]===3).map(([,a])=>a[1]));
  const outputs=new Set([...builtins].filter(([key,value])=>outputVariables.has(key)&&(value===9||value===10)).map(([key])=>key));
  if(!outputs.size)return outputs;
  const definitions=new Map(),labels=new Map(),functionAt=[],entries=new Map();let fn;
  for(let index=0;index<ops.length;index++){
    const [op,a]=ops[index];if(op===54)fn=a[1];functionAt[index]=fn;
    if(op===248){labels.set(a[0],index);if(!entries.has(fn))entries.set(fn,index);}
    if(op>=41&&op!==71&&op!==72&&a.length>=2&&types.has(a[0]))definitions.set(a[1],{op,a,index});
    if(op===56)fn=undefined;
  }
  const successors=index=>{const [op,a]=ops[index];
    if(op===249)return [labels.get(a[0])];
    if(op===250)return [labels.get(a[1]),labels.get(a[2])];
    if(op===251){const selector=definitions.get(a[0]),type=selector&&types.get(selector.a[0]);if(type?.[0]!==21||type[1][0]!==32)throw new Error('Viewport proof requires 32-bit switch selectors');return [labels.get(a[1]),...a.filter((_,i)=>i>=3&&i%2===1).map(value=>labels.get(value))];}
    if([56,252,253,254,255].includes(op))return [];
    return functionAt[index+1]===functionAt[index]?[index+1]:[];
  };
  let budget=2000000;
  const reaches=(from,to,excluded=new Set())=>{const pending=[from],seen=new Set();while(pending.length){if(--budget<0)throw new Error('Viewport proof budget exceeded');const index=pending.pop();if(index===to)return true;if(index===undefined||excluded.has(index)||seen.has(index))continue;seen.add(index);pending.push(...successors(index));}return false;};
  const stores=new Map();
  for(let index=0;index<ops.length;index++){const [op,a]=ops[index];const pointer=op===62?a[0]:op===59&&a.length===4?a[1]:undefined,value=op===62?a[1]:a[3];
    if(pointer!==undefined){if(!stores.has(pointer))stores.set(pointer,[]);stores.get(pointer).push({index,value});}}
  const cache=new Map(),evaluating=new Set();
  function evaluate(id){
    if(cache.has(id))return cache.get(id);if(evaluating.has(id))return undefined;evaluating.add(id);
    const def=definitions.get(id);let value;
    if(def){const {op,a,index}=def;
      if(op===43&&a.length===3)value=a[2]>>>0;
      else if(op===41||op===42)value=op===41?1:0;
      else if(op===124||op===83)value=evaluate(a[2]);
      else if([128,130,132].includes(op)){const x=evaluate(a[2]),y=evaluate(a[3]);if(x!==undefined&&y!==undefined)value=op===128?(x+y)>>>0:op===130?(x-y)>>>0:Math.imul(x,y)>>>0;}
      else if(op===245){const values=a.filter((_,i)=>i>=2&&i%2===0).map(evaluate);if(values.length&&values[0]!==undefined&&values.every(v=>v===values[0]))value=values[0];}
      else if(op===61){const pointer=a[2],builtin=builtins.get(pointer);
        if((builtin===43||builtin===4425)&&instanceBase!==undefined)value=instanceBase>>>0;
        else {const variable=definitions.get(pointer);
          const escaped=ops.some(([kind,args])=>kind===62&&args[1]===pointer||[63,64].includes(kind)&&args[0]===pointer||[65,66,67,70,83].includes(kind)&&args[2]===pointer||kind===57&&args.slice(3).includes(pointer)||[169,245].includes(kind)&&args.slice(2).includes(pointer));
          if(variable?.op===59&&variable.a[2]===7&&!escaped){
            const candidates=(stores.get(pointer)||[]).filter(store=>functionAt[store.index]===functionAt[index]);
            for(const candidate of candidates){
              if(reaches(entries.get(functionAt[index]),index,new Set([candidate.index])))continue;
              const after=successors(candidate.index)[0];
              const overwritten=candidates.some(other=>other!==candidate&&reaches(after,other.index,new Set([candidate.index,index]))&&reaches(successors(other.index)[0],index,new Set([candidate.index])));
              if(!overwritten){value=evaluate(candidate.value);if(value!==undefined)break;}
            }
          }
        }
      }
    }
    evaluating.delete(id);cache.set(id,value);return value;
  }
  // Pointer aliases and reads of these outputs need a separate lowering.
  for(const variable of outputs){
    const declaration=definitions.get(variable),pointer=declaration&&types.get(declaration.a[0]),type=pointer?.[0]===32&&types.get(pointer[1][1]);
    if(type?.[0]!==21||type[1][0]!==32)throw new Error('Viewport/layer output must be a 32-bit integer');
    if(ops.some(([op,a])=>op===61&&a[2]===variable||[63,64].includes(op)&&a[0]===variable||[65,66,67,70,83].includes(op)&&a[2]===variable||op===62&&a[1]===variable||op===57&&a.slice(3).includes(variable)))throw new Error('Viewport/layer output pointer escapes');
    if(!(stores.get(variable)||[]).length)throw new Error('Uninitialized viewport/layer output needs separate lowering');
    for(const store of stores.get(variable)||[])if(evaluate(store.value)!==0)throw new Error('Nonzero or unproven viewport/layer needs multiple viewport/layer support');
  }
  return outputs;
}

export function lowerSpirv(input, {topology,singleInstanceBase,textureSwizzles} = {}) {
  if(singleInstanceBase!==undefined&&(!Number.isInteger(singleInstanceBase)||singleInstanceBase<0||singleInstanceBase>0xffffffff))throw new Error('Invalid single-instance base');
  const bytes = input instanceof Uint8Array ? input : new Uint8Array(input);
  if (bytes.length < 20 || bytes.length % 4 || bytes.length > 16 * 1024 * 1024) throw new Error('Invalid SPIR-V byte length');
  const words = new Uint32Array(bytes.slice().buffer);
  if (words[0] !== 0x07230203 || !words[3] || words[3] > 0x400000) throw new Error('Invalid SPIR-V header');
  let ops=[];
  for(let i=5;i<words.length;) {
    const size=words[i]>>>16,opcode=words[i]&65535;
    if (!size || i+size>words.length) throw new Error('Malformed SPIR-V instruction');
    ops.push([opcode,Array.from(words.subarray(i+1,i+size))]);i+=size;
  }
  // Never let Naga silently ignore an unknown gather operand. ConstOffset and
  // Offset are lowered below; ConstOffsets and other operands remain explicit.
  for(const [op,a]of ops)if((op===96||op===97)&&a.length>5&&![0,8,16].includes(a[5]))throw new Error(`Gather image operands 0x${a[5].toString(16)} need explicit lowering`);
  const types=new Map(ops.filter(([op])=>op>=19&&op<=39).map(([op,a])=>[a[0],[op,a.slice(1)]]));
  const valueTypes=new Map(ops.filter(([op,a])=>op>=41&&op!==71&&op!==72&&a.length>=2&&types.has(a[0])).map(([,a])=>[a[1],a[0]]));
  const decorations=new Map();
  for(const [op,a] of ops) if(op===71 && a.length===3) {
    if (!decorations.has(a[0])) decorations.set(a[0],new Map());
    decorations.get(a[0]).set(a[1],a[2]);
  }
  if(ops.some(([op,a])=>op===72&&a[2]===11&&(a[3]===9||a[3]===10)))throw new Error('Viewport/layer struct output needs separate lowering');
  const builtins=new Map([...decorations].filter(([,value])=>value.has(11)).map(([key,value])=>[key,value.get(11)]));
  const viewportOutputs=zeroViewportOutputs(ops,types,builtins,singleInstanceBase);
  const declarations=[],annotations=[],layout={samplers:[],pushConstant:null,drawParameters:null,pointSizeLocations:[],classifications:0,integerOperations:0};
  if(viewportOutputs.size)layout.singleViewport={outputs:[...viewportOutputs].map(variable=>builtins.get(variable)),instanceBase:singleInstanceBase};
  function id() {if(words[3]>=0x400000) throw new Error('SPIR-V ID limit exceeded');return words[3]++;}
  function declare(op,a) {declarations.push([op,a]);types.set(a[0],[op,a.slice(1)]);}
  function matchingType(op,tail) {
    for(const [key,[kind,a]] of types) if(kind===op&&a.length===tail.length&&a.every((v,i)=>v===tail[i]))return key;
    const result=id();declare(op,[result,...tail]);return result;
  }
  const occupied=new Map();
  for(const [,a] of ops.filter(([op])=>op===59)) {
    const d=decorations.get(a[1]);
    if(d?.has(33)) {
      const group=d.get(34)??0;if(!occupied.has(group))occupied.set(group,new Set());occupied.get(group).add(d.get(33));
    }
  }
  const sampled=new Map([...types].filter(([,v])=>v[0]===27).map(([key,v])=>[key,v[1][0]]));
  const combined=new Map();
  let samplerType,samplerPointer;
  for(const [op,a] of ops) if(op===59&&a[2]===0) {
    const pointer=types.get(a[0]);
    if(pointer?.[0]!==32 || !sampled.has(pointer[1][1])) continue;
    const combinedType=pointer[1][1],imageType=sampled.get(combinedType),d=decorations.get(a[1]);
    if(!d?.has(33)) throw new Error('Combined sampler lacks explicit binding');
    if(!samplerType) {samplerType=matchingType(26,[]);samplerPointer=matchingType(32,[0,samplerType]);}
    // Programs are compiled one stage at a time, then combined in a pipeline.
    // Choosing the first free binding independently would let a fragment
    // sampler occupy a vertex image's binding. Reserve a stable sampler
    // namespace above the Maxwell stage image bindings (0..255).
    const group=d.get(34)??0,used=occupied.get(group),imageBinding=d.get(33);
    if(imageBinding>=256)throw new Error('Combined sampler image exceeds the reserved binding namespace');
    const binding=256+imageBinding;
    if(used.has(binding))throw new Error('Combined sampler conflicts with the reserved binding namespace');
    used.add(binding);
    const variable=id();declarations.push([59,[samplerPointer,variable,0]]);
    annotations.push([71,[variable,34,group]],[71,[variable,33,binding]]);
    combined.set(a[1],{imageType,variable,combinedType,pointer:a[0]});
    const image=types.get(imageType),dimension=image?.[1][1],arrayed=image?.[1][3],viewDimension=dimension===0&&!arrayed?'1d':dimension===1?(arrayed?'2d-array':'2d'):dimension===2&&!arrayed?'3d':dimension===3?(arrayed?'cube-array':'cube'):null;
    if(!viewDimension)throw new Error('Image dimensionality needs explicit WebGPU lowering');
    (layout.textureDimensions??={})[`${group}/${imageBinding}`]=viewDimension;
    layout.samplers.push({group,imageBinding:d.get(33),samplerBinding:binding,sourceVariable:a[1],...(types.get(imageType)?.[1][2]===1?{depth:true}:{})});
  }
  const newImagePointers=new Map([...combined.values()].map(item=>[item.pointer,item.imageType]));
  const locations=new Set();
  const outputs=new Set(ops.filter(([op,a])=>op===59&&a[2]===3).map(([,a])=>a[1]));
  for(const [op,a] of ops) {if(op===71&&outputs.has(a[0])&&a[1]===30)locations.add(a[2]);if(op===72&&a[2]===30)locations.add(a[3]);}
  function pointLocation() {
    if(topology!=='triangles')throw new Error('PointSize requires explicit triangle topology; points need expansion');
    let location=0;while(locations.has(location))location++;
    if(location>=16)throw new Error('No varying location for triangle PointSize lowering');
    locations.add(location);layout.pointSizeLocations.push(location);return location;
  }
  const push=[];
  for(const [op,a] of ops)if(op===59&&a[2]===9)push.push(a[1]);
  if(push.length>1)throw new Error('Multiple push-constant blocks unsupported');
  if(push.length) {
    if(occupied.get(3)?.has(0))throw new Error('Push-constant uniform conflicts with group 3 binding 0');
    annotations.push([71,[push[0],34,3]],[71,[push[0],33,0]]);layout.pushConstant={group:3,binding:0};
  }
  const drawVariables=new Map();
  for(const [variable,d] of decorations) {
    const builtin=d.get(11);
    if(builtin>=4424&&builtin<=4426)drawVariables.set(variable,builtin-4424);
  }
  let drawUniform,drawPointer,unsigned;
  if(drawVariables.size) {
    if(occupied.get(3)?.has(1))throw new Error('Draw parameters conflict with group 3 binding 1');
    unsigned=matchingType(21,[32,0]);
    const structure=matchingType(30,[unsigned,unsigned,unsigned]);
    const pointer=matchingType(32,[2,structure]);drawPointer=matchingType(32,[2,unsigned]);
    drawUniform=id();declarations.push([59,[pointer,drawUniform,2]]);
    annotations.push([71,[structure,2]],...[0,1,2].map(member=>[72,[structure,member,35,member*4]]),
      [71,[drawUniform,34,3]],[71,[drawUniform,33,1]]);
    layout.drawParameters={group:3,binding:1,bytes:16,fields:['baseVertex','baseInstance','drawIndex']};
  }
  const constants=new Map();
  function constant(value,width) {
    const key=`${value}/${width}`;if(constants.has(key))return constants.get(key);
    const unsigned=matchingType(21,[32,0]);let result;
    if(width===1) {result=id();declarations.push([43,[unsigned,result,value]]);}
    else {const component=constant(value,1),vector=matchingType(23,[unsigned,width]);result=id();declarations.push([44,[vector,result,...Array(width).fill(component)]]);}
    constants.set(key,result);return result;
  }
  const floatConstants=new Map();
  function floatPair(type,bits){
    const key=`${type}/${bits}`;if(floatConstants.has(key))return floatConstants.get(key);
    const scalar=id(),vector=id();declarations.push([43,[type,scalar,bits]],[44,[matchingType(23,[type,2]),vector,scalar,scalar]]);floatConstants.set(key,vector);return vector;
  }
  let glslImport=ops.find(([op,a])=>op===11&&new TextDecoder().decode(new Uint8Array(new Uint32Array(a.slice(1)).buffer)).split('\0')[0]==='GLSL.std.450')?.[1][0];
  const sampledSources=new Map();let gatherQueries=false;
  const rewritten=[];
  for(const [op,original] of ops) {
    const a=[...original];
    if(op===17&&a[0]===5254)continue;
    if(op===10&&new TextDecoder().decode(new Uint8Array(new Uint32Array(a).buffer)).split('\0')[0]==='SPV_EXT_shader_viewport_index_layer')continue;
    if((op===5||op===71)&&viewportOutputs.has(a[0])||op===59&&viewportOutputs.has(a[1])||op===62&&viewportOutputs.has(a[0]))continue;
    if(op===17&&a[0]===25)continue; // ImageGatherExtended, checked above.
    if((op===96||op===97)&&a.length>5&&a[5]){
      if(a.length!==7)throw new Error('Invalid gather offset operand count');
      const item=sampledSources.get(a[2]),imageType=item&&types.get(item.imageType),coordinateType=valueTypes.get(a[3]),coordinate=types.get(coordinateType),offset=types.get(valueTypes.get(a[6]));
      const scalar=coordinate?.[0]===23&&types.get(coordinate[1][0]),integer=offset?.[0]===23&&types.get(offset[1][0]);
      if(imageType?.[0]!==25||imageType[1][1]!==1||imageType[1][4]||coordinate?.[0]!==23||coordinate[1][1]!==2+imageType[1][3]||scalar?.[0]!==22||scalar[1][0]!==32||offset?.[0]!==23||offset[1][1]!==2||integer?.[0]!==21||integer[1][0]!==32)throw new Error('Gather offset requires a 2D/2D-array image and 32-bit coordinates');
      const float=coordinate[1][0],vec2=matchingType(23,[float,2]),unsigned=matchingType(21,[32,0]),uvec2=matchingType(23,[unsigned,2]),arrayed=!!imageType[1][3],queryType=arrayed?matchingType(23,[unsigned,3]):uvec2;
      if(!glslImport){glslImport=id();annotations.push([11,[glslImport,0x4c534c47,0x6474732e,0x3035342e,0]]);}
      const image=id(),size=id(),sizeXY=arrayed?id():size,sizeFloat=id(),xy=arrayed?id():a[3],scaled=id(),edge=id(),base=id(),offsetFloat=id(),shifted=id(),center=id(),normalized=id();
      rewritten.push([100,[item.imageType,image,a[2]]],[103,[queryType,size,image,constant(0,1)]]);
      if(arrayed)rewritten.push([79,[uvec2,sizeXY,size,size,0,1]],[79,[vec2,xy,a[3],a[3],0,1]]);
      rewritten.push([112,[vec2,sizeFloat,sizeXY]],[133,[vec2,scaled,xy,sizeFloat]],[131,[vec2,edge,scaled,floatPair(float,0x3f000000)]],[12,[vec2,base,glslImport,8,edge]],
        [integer[1][1]?111:112,[vec2,offsetFloat,a[6]]],[129,[vec2,shifted,base,offsetFloat]],[129,[vec2,center,shifted,floatPair(float,0x3f800000)]],[136,[vec2,normalized,center,sizeFloat]]);
      if(arrayed){const x=id(),y=id(),layer=id(),coord=id();rewritten.push([81,[float,x,normalized,0]],[81,[float,y,normalized,1]],[81,[float,layer,a[3],2]],[80,[coordinateType,coord,x,y,layer]]);a[3]=coord;}else a[3]=normalized;
      a.splice(5);rewritten.push([op,a]);gatherQueries=true;layout.gatherOffsets=(layout.gatherOffsets||0)+1;continue;
    }
    if(op===17&&a[0]===4427&&drawVariables.size)continue;
    if(op===10&&drawVariables.size) {
      const name=new TextDecoder().decode(new Uint8Array(new Uint32Array(a).buffer)).split('\0')[0];
      if(name==='SPV_KHR_shader_draw_parameters')continue;
    }
    if((op===5||op===71)&&drawVariables.has(a[0]))continue;
    if(op===59&&drawVariables.has(a[1]))continue;
    if(op===32&&newImagePointers.has(a[0]))a[2]=newImagePointers.get(a[0]);
    if(op===32&&a[1]===9)a[1]=2;
    if(op===59&&a[2]===9)a[2]=2;
    if(op===71&&a[1]===11&&a[2]===1) {a[1]=30;a[2]=pointLocation();}
    if(op===72&&a[2]===11&&a[3]===1) {a[2]=30;a[3]=pointLocation();}
    if(op===15 && (words[1]>=0x00010400||drawVariables.size||viewportOutputs.size)) {
      // SPIR-V 1.4+ requires all statically used global resources in the
      // entry-point interface, including each newly split sampler variable.
      let end=2;
      while(end<a.length) {const w=a[end++];if([0,8,16,24].some(shift=>((w>>>shift)&255)===0))break;}
      const interfaces=a.slice(end);
      if(words[1]>=0x00010400) {
        for(const [source,item] of combined)if(interfaces.includes(source))a.push(item.variable);
        if(interfaces.some(source=>drawVariables.has(source)))a.push(drawUniform);
      }
      for(let i=a.length-1;i>=end;i--)if(drawVariables.has(a[i])||viewportOutputs.has(a[i]))a.splice(i,1);
    }
    if(op===61&&drawVariables.has(a[2])) {
      if(a.length!==3)throw new Error('Unsupported draw-parameter memory operands');
      const type=types.get(a[0]);
      if(type?.[0]!==21||type[1][0]!==32)throw new Error('Draw parameter must be a 32-bit integer');
      const member=constant(drawVariables.get(a[2]),1),pointer=id(),value=a[0]===unsigned?a[1]:id();
      rewritten.push([65,[drawPointer,pointer,drawUniform,member]],[61,[unsigned,value,pointer]]);
      if(value!==a[1])rewritten.push([124,[a[0],a[1],value]]);
      continue;
    }
    if(op===61&&sampled.has(a[0])) {
      const item=combined.get(a[2]);
      if(!item || a.length!==3)throw new Error('Unsupported combined-sampler pointer or memory operands');
      const image=id(),sampler=id();
      rewritten.push([61,[item.imageType,image,a[2]]],[61,[samplerType,sampler,item.variable]],[86,[a[0],a[1],image,sampler]]);
      sampledSources.set(a[1],item);
      continue;
    }
    if(op===156||op===157) {
      const [resultType,result,operand]=a,boolean=types.get(resultType);
      const width=boolean?.[0]===23?boolean[1][1]:1;
      const operandType=types.get(valueTypes.get(operand));
      const scalar=operandType?.[0]===23?types.get(operandType[1][0]):operandType;
      if(!scalar || scalar[0]!==22||scalar[1][0]!==32||width<1||width>4)throw new Error('Only scalar/vector f32 classification supported');
      const unsigned=matchingType(21,[32,0]),integer=width===1?unsigned:matchingType(23,[unsigned,width]);
      const bits=id(),absolute=id(),mask=constant(0x7fffffff,width),infinity=constant(0x7f800000,width);
      rewritten.push([124,[integer,bits,operand]],[199,[integer,absolute,bits,mask]],[op===156?172:170,[resultType,result,absolute,infinity]]);
      layout.classifications++;continue;
    }
    if(op===149||op===150) {
      const [resultType,result,left,right]=a,structure=types.get(resultType);
      if(structure?.[0]!==30||structure[1].length!==2||structure[1][0]!==structure[1][1])throw new Error('Unsupported carry-result structure');
      const integer=structure[1][0],kind=types.get(integer),width=kind?.[0]===23?kind[1][1]:1;
      const scalar=kind?.[0]===23?types.get(kind[1][0]):kind;
      if(scalar?.[0]!==21||scalar[1][0]!==32||scalar[1][1]!==0||width<1||width>4)throw new Error('Only unsigned 32-bit carry/borrow supported');
      const boolean=matchingType(20,[]),boolType=width===1?boolean:matchingType(23,[boolean,width]);
      const sum=id(),condition=id(),carry=id(),one=constant(1,width),zero=constant(0,width);
      rewritten.push([op===149?128:130,[integer,sum,left,right]],
        [176,[boolType,condition,op===149?sum:left,op===149?left:right]],
        [169,[integer,carry,condition,one,zero]],[80,[resultType,result,sum,carry]]);
      layout.integerOperations++;continue;
    }
    rewritten.push([op,a]);
  }
  ops=rewritten;
  if(gatherQueries&&!ops.some(([op,a])=>op===17&&a[0]===50))ops.unshift([17,[50]]);
  // Imports precede memory model/entry-point declarations, unlike decorations.
  for(let index=annotations.length-1;index>=0;index--)if(annotations[index][0]===11){const [entry]=annotations.splice(index,1);const at=ops.findIndex(([op])=>op===14);ops.splice(at<0?0:at,0,entry);}
  if(annotations.length) {
    const first=ops.findIndex(([op])=>op>=19&&op<=39);
    if(first<0)throw new Error('SPIR-V has no type declarations');ops.splice(first,0,...annotations);
  }
  if(declarations.length) {
    const first=ops.findIndex(([op])=>op===54);
    if(first<0)throw new Error('SPIR-V has no functions');ops.splice(first,0,...declarations);
  }
  if(textureSwizzles)ops=swizzleImages(ops,textureSwizzles,id,layout);
  const output=Array.from(words.subarray(0,5));
  for(const [op,a] of ops) {output.push(((a.length+1)<<16)|op,...a);}
  return {bytes:new Uint8Array(new Uint32Array(output).buffer),layout};
}


/** Apply descriptor component mapping to shader image results, not uploaded
 * texels: preserves filtering, integer types and out-of-bounds fetch behavior.
 * Gather remaps the selected channel, never the four neighboring texels. */
function swizzleImages(ops,mappings,id,layout){
  if(!mappings||Array.isArray(mappings)||typeof mappings!=='object')throw new Error('Invalid texture swizzle mappings');
  const requested=new Map();
  for(const [key,value]of Object.entries(mappings)){
    if(!/^\d+\/\d+$/.test(key)||!Array.isArray(value)||value.length!==4||value.some(n=>!Number.isInteger(n)||n<0||n>5))throw new Error('Invalid texture swizzle mapping');
    if(value.some((n,i)=>n!==i+2))requested.set(key,value);
  }
  if(!requested.size)return ops;
  const types=new Map(ops.filter(([op])=>op>=19&&op<=39).map(([op,a])=>[a[0],[op,a.slice(1)]]));
  const defs=new Map(ops.filter(([op,a])=>op>=41&&types.has(a[0])&&a.length>=2).map(([op,a])=>[a[1],[op,a]]));
  const decorations=new Map();for(const [op,a]of ops)if(op===71&&(a[1]===33||a[1]===34)){if(!decorations.has(a[0]))decorations.set(a[0],{});decorations.get(a[0])[a[1]]=a[2];}
  const resources=new Map();for(const [op,a]of ops)if(op===59){const d=decorations.get(a[1]),pointer=types.get(a[0]),type=pointer&&types.get(pointer[1][1]);if(d&&[25,27].includes(type?.[0]))resources.set(a[1],`${d[34]??0}/${d[33]}`);}
  const roots=new Map();
  function origin(value,seen=new Set()){
    if(resources.has(value))return resources.get(value);if(roots.has(value))return roots.get(value);
    if(seen.has(value))throw new Error('Cyclic image source needs swizzle lowering');seen.add(value);
    const def=defs.get(value);let result;
    if(def){const [op,a]=def;
      if([61,83,86,100].includes(op))result=origin(a[2],seen);
      else if(op===169||op===245){const values=op===169?a.slice(3,5):a.filter((_,i)=>i>=2&&i%2===0),sources=values.map(v=>origin(v,new Set(seen)));if(sources.some(s=>s!==sources[0]))throw new Error('Mixed image sources need swizzle lowering');result=sources[0];}
    }
    roots.set(value,result);return result;
  }
  const declarations=[],constants=new Map();
  function scalar(type,value){const key=`${type}/${value}`;if(constants.has(key))return constants.get(key);const t=types.get(type);if(!t||![21,22].includes(t[0])||t[1][0]!==32)throw new Error('Texture swizzle requires 32-bit components');const n=id();declarations.push([43,[type,n,t[0]===22&&value===1?0x3f800000:value]]);constants.set(key,n);return n;}
  const rewritten=[];let operations=0;
  for(const [op,args]of ops){const a=[...args];
    if(op>=87&&op<=120&&op!==99&&op!==100){
      const key=origin(a[2]),mapping=key&&requested.get(key);
      if(!key&&op<=98)throw new Error('Unresolved image source needs swizzle lowering');
      if(mapping){
        if([101,102,103,104,105,106,107].includes(op)){rewritten.push([op,a]);continue;}
        if(![87,88,91,92,95,96,98].includes(op))throw new Error('Comparison/sparse image swizzle needs explicit lowering');
        const vector=types.get(a[0]);if(vector?.[0]!==23||vector[1][1]!==4)throw new Error('Texture swizzle requires a four-component image result');const component=vector[1][0];
        if(op===96){const c=defs.get(a[4]);if(c?.[0]!==43||c[1][2]>3)throw new Error('Gather swizzle requires a constant component');const selected=mapping[c[1][2]];
          if(selected<2){rewritten.push([80,[a[0],a[1],...Array(4).fill(scalar(component,selected))]]);}
          else{a[4]=scalar(c[1][0],selected-2);rewritten.push([op,a]);}
        }else{const result=a[1],raw=id();a[1]=raw;rewritten.push([op,a]);const channels=mapping.map(selected=>{if(selected<2)return scalar(component,selected);const n=id();rewritten.push([81,[component,n,raw,selected-2]]);return n;});rewritten.push([80,[a[0],result,...channels]]);}
        operations++;continue;
      }
    }
    rewritten.push([op,a]);
  }
  if(declarations.length){const at=rewritten.findIndex(([op])=>op===54);if(at<0)throw new Error('Texture swizzle module has no functions');rewritten.splice(at,0,...declarations);}
  layout.textureSwizzles=operations;return rewritten;
}
