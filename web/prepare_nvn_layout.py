"""Prepare the user's lifted SDK texture-layout graph and relocated data image.

All generated code/data stays in ignored build/. The copied SDK device model
describes the source texture memory layout, not WebGPU physical capabilities.
"""
import hashlib
import json
from pathlib import Path
import re
import struct

ROOT=Path(__file__).resolve().parents[1]
BASE=0x7500000000
SIZE=0xcfb000
SDK_SHA256='d42578f463806dbd0023752dcecc43fed230c9f35d3a787696c3e92dd08eccdd'


def prepare():
    out=ROOT/'build/web-nvn-layout';out.mkdir(parents=True,exist_ok=True)
    elf=(ROOT/'build/native/binaries/13.0.4/sdk.elf').read_bytes()
    if hashlib.sha256(elf).hexdigest()!=SDK_SHA256:
        raise ValueError('SDK image differs from the verified 13.0.4 source-layout module; regenerate and audit its roots/model first')
    image=bytearray(SIZE)
    phoff=struct.unpack_from('<Q',elf,32)[0];tags={}
    programs=[struct.unpack_from('<IIQQQQQQ',elf,phoff+i*56) for i in range(struct.unpack_from('<H',elf,56)[0])]
    for kind,_,offset,address,_,size,memory,_ in programs:
        if kind==1:
            if address+memory>SIZE or size>memory or offset+size>len(elf):raise ValueError('Invalid SDK LOAD bounds')
            image[address:address+size]=elf[offset:offset+size]
        elif kind==2:
            for position in range(offset,offset+size,16):
                tag,value=struct.unpack_from('<QQ',elf,position)
                if not tag:break
                tags[tag]=value
    for start,length in ((tags[7],tags[8]),(tags[23],tags[2])):
        for offset in range(start,start+length,24):
            target,info,addend=struct.unpack_from('<QQq',image,offset)
            kind,symbol=info&0xffffffff,info>>32
            if target+8>SIZE:raise ValueError('SDK relocation outside image')
            if kind==1027:value=BASE+addend
            elif kind in (257,1025,1026):
                _,_,_,section,value,_=struct.unpack_from('<IBBHQQ',image,tags[6]+symbol*24)
                value=BASE+value+(addend if kind==257 else 0) if section else 0
            else:raise ValueError(f'Unsupported SDK relocation {kind}')
            struct.pack_into('<Q',image,target,value)
    (out/'sdk-layout-image.bin').write_bytes(image)
    source=(ROOT/'build/native/work/sdklift_gen.c').read_text(encoding='utf-8')
    pattern=re.compile(r'^static void (f_[0-9a-f]+)\(Ctx \*c\) \{(?:[^\n]*\}|\n.*?^\})',re.M|re.S)
    functions={match[1]:match[0] for match in pattern.finditer(source)}
    roots={BASE+offset for offset in (0x4278b0,0x428760,0x427a10,0x427b30,0x4290b0,0x427ed0,0x401ae0,0x408e10)}
    # NvRmGpuDevice/chip methods used by the native source-layout model. Keep
    # the actual relocated vtable targets, including their direct dependencies.
    for table,slots in ((0xc114e8,(0x80,0x90,*range(0x108,0x160,8))),(0xc11978,(0x30,))):
        for slot in slots:
            address=struct.unpack_from('<Q',image,table+slot)[0]
            if not BASE+0x400000<=address<BASE+0x410000:raise ValueError('Unexpected SDK source-layout vtable method')
            roots.add(address)
    selected={};pending=[f'f_{address:x}' for address in roots]
    while pending:
        name=pending.pop()
        if name in selected:continue
        body=functions[name]
        if re.search(r'\b(?:sdk_host_call|sdk_trap_call|SDK_SETJMP|sdk_longjmp)\s*\(',body):
            raise ValueError(f'SDK layout dependency needs an unported host service: {name}')
        selected[name]=body
        pending.extend(set(re.findall(r'\bf_[0-9a-f]+(?=\()',body))-selected.keys())
    declarations='\n'.join(f'static void {name}(Ctx *c);' for name in sorted(selected))
    output='#include "runtime.h"\nstatic void layout_dispatch(Ctx *,uint64_t);\n#define call_indirect layout_dispatch\n'+declarations+'\n'
    output+='\n\n'.join(selected[name] for name in sorted(selected))
    output+='\n#undef call_indirect\nstatic void layout_dispatch(Ctx *c,uint64_t address) {\n switch(address) {\n'
    output+='\n'.join(f'case 0x{name[2:]}ULL: {name}(c);return;' for name in sorted(selected))
    output+='\ndefault:guest_trap("uncompiled SDK texture layout target",address);\n}}\n'
    output+='void web_nvn_layout_dispatch(Ctx *c,uint64_t address){layout_dispatch(c,address);}\n'
    path=out/'layout_sdk.c';path.write_text(output,encoding='utf-8')
    manifest=dict(functions=len(selected),sourceBytes=len(output),sdkSha256=hashlib.sha256(elf).hexdigest(),imageSha256=hashlib.sha256(image).hexdigest(),roots=[hex(address) for address in sorted(roots)])
    (out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print(json.dumps(manifest),flush=True)
    return path


if __name__=='__main__':prepare()
