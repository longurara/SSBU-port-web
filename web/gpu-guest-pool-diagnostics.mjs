/** Opt-in diagnostics for the pinned Wasm32 Resource ABI in host_gpu_resources.c.
 * Call only while that registry is stable. Production upload operation2 originates
 * in transfer() while resource_lock is held until the owner replies. No heap writes. */
const BUCKETS=4096,RESOURCE_BYTES=80,MAX_ROWS=16384,MAX_SEARCH=32*1024*1024;
const guest=a=>a===0n||(a>=0x7100000000n&&a<0x10000000000n);
const bucket=key=>Number(((key>>4n)*2654435761n)&4095n);

function reader(heap,buffers){
  if(!(heap instanceof Uint8Array)||heap.byteOffset!==0)throw new Error('Expected a complete Wasm heap view');
  const v=new DataView(heap.buffer,0,heap.byteLength),pointer=p=>p!==0&&p%8===0&&p+RESOURCE_BYTES<=heap.length;
  const row=p=>{
    if(!pointer(p))return null;
    const key=v.getBigUint64(p,true),memory=v.getBigUint64(p+8,true),size=v.getBigUint64(p+16,true);
    const kind=v.getUint32(p+48,true),gpu=v.getUint32(p+56,true),next=v.getUint32(p+72,true);
    if(!key||!guest(key)||!guest(memory)||size>0xffffffffn||kind<1||kind>4||(next&&!pointer(next)))return null;
    if(kind===1&&gpu&&(!buffers.has(gpu)||BigInt(buffers.get(gpu).size)!==size))return null;
    return {pointer:p,key,memory,size,kind,gpu,next,flags:v.getUint32(p+52,true),address:v.getBigUint64(p+40,true)};
  };
  const table=base=>{
    if(!Number.isInteger(base)||base<0||base%4||base+BUCKETS*4>heap.length)return null;
    const rows=[],seen=new Set(),keys=new Set();
    for(let i=0;i<BUCKETS;i++){
      let p=v.getUint32(base+i*4,true);
      while(p){
        if(seen.has(p)||rows.length>=MAX_ROWS)return null;
        const r=row(p);if(!r||keys.has(r.key)||bucket(r.key)!==i)return null;
        seen.add(p);keys.add(r.key);rows.push(r);p=r.next;
      }
    }
    return rows;
  };
  return {v,row,table};
}

export function findGuestPoolTable(heap,buffers,{searchBytes=MAX_SEARCH}={}){
  if(!Number.isInteger(searchBytes)||searchBytes<BUCKETS*4||searchBytes>MAX_SEARCH)throw new Error('Invalid guest-pool search budget');
  const r=reader(heap,buffers),end=Math.min(heap.length,searchBytes),checked=new Set(),found=[];
  for(let slot=0;slot+4<=end;slot+=4){
    const p=r.v.getUint32(slot,true);
    if(!p||p%8||p+RESOURCE_BYTES>heap.length||r.v.getUint32(p+48,true)!==1)continue;
    const gpu=r.v.getUint32(p+56,true);if(!gpu||!buffers.has(gpu))continue;
    const row=r.row(p);if(!row)continue;
    const base=slot-bucket(row.key)*4;
    if(base<0||base+BUCKETS*4>end||checked.has(base))continue;
    checked.add(base);if(checked.size>256)throw new Error('Guest-pool candidate budget exceeded');
    const rows=r.table(base);
    if(rows?.some(x=>x.kind===1&&x.gpu===gpu))found.push(base);
  }
  if(found.length>1)throw new Error('Ambiguous guest-pool registry');
  return found.length?found[0]:null;
}

export function readGuestGpuPools(heap,buffers,base){
  const rows=reader(heap,buffers).table(base);
  if(!rows)throw new Error('Guest-pool registry validation failed');
  return rows.filter(r=>r.kind===1&&r.gpu).map(r=>({gpuHandle:r.gpu,key:'0x'+r.key.toString(16),
    memory:'0x'+r.memory.toString(16),gpuAddress:'0x'+r.address.toString(16),size:Number(r.size),flags:r.flags}));
}

export function describeGuestBufferRange(pools,range){
  if(!Array.isArray(range)||range.length!==3||!range.every(Number.isSafeInteger))return null;
  const [handle,offset,size]=range,pool=pools.get(handle);
  if(!pool||offset<0||size<0||offset>pool.size||size>pool.size-offset)return null;
  return {gpuHandle:handle,poolKey:pool.key,offset,size,
    guestAddress:pool.memory==='0x0'?null:'0x'+(BigInt(pool.memory)+BigInt(offset)).toString(16),
    scope:pool.memory==='0x0'?'Pool has no guest CPU backing':'Guest upload source derived from the observed pool memory and GPU buffer offset'};
}

export function installGuestPoolDiagnostics(backend,getHeap){
  let arrayAddress=null,attempts=0,closed=false;
  const pools=new Map(),errors=[],original=backend.request;
  const wrapped=function(operation,args={}){
    if(!closed&&operation===2&&!pools.has(args.handle)&&pools.size<4096&&attempts<256){
      attempts++;
      try{
        const heap=getHeap();
        arrayAddress??=findGuestPoolTable(heap,backend.buffers);
        if(arrayAddress!==null)for(const pool of readGuestGpuPools(heap,backend.buffers,arrayAddress))pools.set(pool.gpuHandle,pool);
      }catch(error){if(errors.length<8)errors.push(String(error));}
    }
    return original.call(this,operation,args);
  };
  backend.request=wrapped;
  return {pools,describe:range=>describeGuestBufferRange(pools,range),report:()=>({arrayAddress,attempts,pools:[...pools.values()],errors,
    scope:'Read-only Wasm32 resource metadata during upload; GPU handles/sizes cross-checked with the actual backend, not game rendering proof'}),
    close(){closed=true;if(backend.request===wrapped)backend.request=original;}};
}
