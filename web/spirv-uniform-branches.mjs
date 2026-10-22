/** Remove simple untaken selections only after evaluating their condition from
 * the exact read-only constant-buffer words for this draw. Unknown data,
 * control flow, aliasing and loops never authorize removing a branch. */
export function specializeUniformBranches(input, uniformWords={}) {
  const bytes=input instanceof Uint8Array?input:new Uint8Array(input);
  if(bytes.byteLength%4)throw new Error('Unaligned SPIR-V');
  const words=new Uint32Array(bytes.slice().buffer),ops=[];
  if(words[0]!==0x07230203)throw new Error('Invalid SPIR-V');
  for(let at=5;at<words.length;){const length=words[at]>>>16;if(!length||at+length>words.length)throw new Error('Truncated SPIR-V');ops.push({op:words[at]&65535,a:Array.from(words.subarray(at+1,at+length)),at,length});at+=length;}
  const types=new Map(),constants=new Map(),bindings=new Map(),groups=new Map(),variables=new Map(),strides=new Map(),offsets=new Map(),readOnly=new Set();let entry;
  const known=(bits,reads=[])=>({bits,reads});
  for(const {op,a}of ops){
    if(op>=19&&op<=39)types.set(a[0],{op,a:a.slice(1)});
    if(op===15&&[0,4].includes(a[0])){if(entry!==undefined&&entry!==a[1])return {bytes,branches:0,reads:[]};entry=a[1];}
    if(op===41||op===42)constants.set(a[1],known(op===41));
    if(op===43&&types.get(a[0])?.a[0]===32)constants.set(a[1],known(a[2]));
    if(op===71&&a[1]===33)bindings.set(a[0],a[2]);
    if(op===71&&a[1]===34)groups.set(a[0],a[2]);
    if(op===71&&a[1]===6)strides.set(a[0],a[2]);
    if(op===71&&a[1]===24)readOnly.add(a[0]);
    if(op===72&&a[1]===0&&a[2]===35)offsets.set(a[0],a[3]);
    if(op===59)variables.set(a[1],{type:a[0],storage:a[2],initializer:a[3]});
  }
  let inside=false;const body=[];
  for(const x of ops){if(x.op===54)inside=x.a[1]===entry;if(inside)body.push(x);if(x.op===56&&inside)break;}
  // Phi/loops need a fixed-point interpreter; refuse this optimization until
  // that proof exists. Shader execution itself remains unchanged.
  if(!body.length||body.some(x=>[245,246].includes(x.op)))return {bytes,branches:0,reads:[]};
  const blocks=new Map();let block;
  for(const x of body){if(x.op===248){block={id:x.a[0],ops:[],incoming:new Set()};blocks.set(block.id,block);}else if(block&&x.op!==56)block.ops.push(x);}
  for(const b of blocks.values()){const last=b.ops.at(-1);for(const id of last?.op===249?[last.a[0]]:last?.op===250?last.a.slice(1,3):[])blocks.get(id)?.incoming.add(b.id);}
  const values=new Map(constants),memory=new Map(),pointers=new Map();
  for(const [id,v]of variables){pointers.set(id,{root:id,path:[]});if(v.initializer!==undefined&&constants.has(v.initializer))memory.set(id,constants.get(v.initializer));}
  const float=bits=>{const view=new DataView(new ArrayBuffer(4));view.setUint32(0,bits,true);return view.getFloat32(0,true);};
  const combine=(bits,...vs)=>known(bits,[...new Set(vs.flatMap(v=>v.reads))]);
  let uniformValid=true;
  const readPointer=p=>{
    if(!p)return;
    const v=variables.get(p.root);
    if(!v)return;
    if([6,7].includes(v.storage)&&!p.path.length)return memory.get(p.root);
    // The Maxwell CB layout is one storage-buffer member containing uint[].
    // Accept exactly that layout, rather than guessing arbitrary byte offsets.
    const key=`${groups.get(p.root)??0}/${bindings.get(p.root)}`;
    const ptr=types.get(v.type),struct=types.get(ptr?.a[1]),member=struct?.a[0],arr=types.get(member),element=types.get(arr?.a[0]);
    if(!uniformValid||v.storage!==2||!readOnly.has(p.root)||ptr?.op!==32||struct?.op!==30||struct.a.length!==1||offsets.get(ptr.a[1])!==0||arr?.op!==29||strides.get(member)!==4||element?.op!==21||element.a[0]!==32||element.a[1]!==0||p.path.length!==2||p.path[0]!==0)return;
    const index=p.path[1],data=uniformWords[key];
    if(!Number.isSafeInteger(index)||index<0||!data||index>=data.length)return;
    const bits=data[index];if(!Number.isInteger(bits)||bits<0||bits>0xffffffff)throw new Error('Invalid uniform word');
    return known(bits,[...new Set([...(p.reads??[]),`${key}/${index}`])]);
  };
  const removed=new Set(),rewritten=new Map(),seen=new Set(),reads=new Set();let branches=0,current=blocks.keys().next().value;
  while(current!==undefined&&!seen.has(current)){
    seen.add(current);const b=blocks.get(current);if(!b)break;let next;
    for(let i=0;i<b.ops.length;i++){
      const x=b.ops[i],{op,a}=x;
      if(op===59){if(a[3]!==undefined&&values.has(a[3]))memory.set(a[1],values.get(a[3]));continue;}
      if(op===65){const p=pointers.get(a[2]),indexValues=a.slice(3).map(id=>values.get(id)),indices=indexValues.map(v=>v?.bits);if(p&&indices.every(n=>Number.isInteger(n)))pointers.set(a[1],{root:p.root,path:[...p.path,...indices],reads:[...new Set([...(p.reads??[]),...indexValues.flatMap(v=>v.reads)])]});continue;}
      if(op===61){const value=readPointer(pointers.get(a[2]));if(value)values.set(a[1],value);continue;}
      if(op===62){const p=pointers.get(a[0]),storage=p?variables.get(p.root)?.storage:undefined;if(p&&[6,7].includes(storage)&&!p.path.length){if(values.has(a[1]))memory.set(p.root,values.get(a[1]));else memory.delete(p.root);}else{memory.clear();if(storage===undefined||![3,6,7].includes(storage))uniformValid=false;}continue;}
      if(op===83||op===124){const value=values.get(a[2]);if(value)values.set(a[1],value);if(op===83&&pointers.has(a[2]))pointers.set(a[1],pointers.get(a[2]));continue;}
      if([164,165,166,167,168,170,171,180,181,182,183].includes(op)){
        const l=values.get(a[2]),r=values.get(a[3]);if(!l||(op!==168&&!r))continue;
        let value;
        if(op===168)value=!l.bits;
        else if(op===164||op===170)value=l.bits===r.bits;
        else if(op===165||op===171)value=l.bits!==r.bits;
        else if(op===166)value=l.bits||r.bits;
        else if(op===167)value=l.bits&&r.bits;
        else{const u=float(l.bits),v=float(r.bits),nan=Number.isNaN(u)||Number.isNaN(v);value=op===180?!nan&&u===v:op===181?nan||u===v:op===182?!nan&&u!==v:nan||u!==v;}
        values.set(a[1],combine(value,l,...(r?[r]:[])));continue;
      }
      if(op===249){next=a[0];break;}
      if(op===250){
        const condition=values.get(a[0]),selection=b.ops[i-1];
        // Only a single-entry, single-block then arm, with no else arm. Keep
        // the merge label and all instructions on the selected path intact.
        const skipped=blocks.get(a[1]),merge=selection?.op===247?selection.a[0]:undefined;
        if(condition?.bits===false&&merge===a[2]&&skipped?.incoming.size===1&&skipped.incoming.has(b.id)&&skipped.ops.at(-1)?.op===249&&skipped.ops.at(-1).a[0]===merge&&!skipped.ops.some(x=>[245,246,247].includes(x.op))){
          removed.add(skipped.id);rewritten.set(selection.at,[]);rewritten.set(x.at,[(2<<16)|249,merge]);branches++;for(const key of condition.reads)reads.add(key);next=merge;
        }
        break;
      }
      if([252,253,254,255].includes(op))break;
      // Calls and pointer-writing instructions invalidate mutable state.
      // Pure scalar/vector/image instructions do not alter local predicates.
      if(op===57||op===63||op===64||(op>=227&&op<=242)||(op===12&&a.slice(4).some(id=>pointers.has(id)))){memory.clear();uniformValid=false;}
    }
    current=next;
  }
  if(!branches)return {bytes,branches:0,reads:[]};
  const output=Array.from(words.subarray(0,5));let skip=false;
  for(const x of ops){if(x.op===248)skip=removed.has(x.a[0]);if(x.op===54||x.op===56)skip=false;if(skip)continue;output.push(...(rewritten.get(x.at)??words.subarray(x.at,x.at+x.length)));}
  return {bytes:new Uint8Array(new Uint32Array(output).buffer),branches,reads:[...reads]};
}
