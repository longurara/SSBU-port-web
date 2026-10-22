"""Execute the SDK InitializeFiber prefix, stopping before platform context setup.

Flag 0 models refusal by the optional alias allocator. This does not claim to
execute SDK context assembly, which the web adapter replaces with Asyncify.
"""
import hashlib
import json
from pathlib import Path
import struct
from unicorn import Uc,UC_ARCH_ARM64,UC_MODE_ARM,UC_HOOK_CODE
from unicorn.arm64_const import UC_ARM64_REG_X0,UC_ARM64_REG_X1,UC_ARM64_REG_X30,UC_ARM64_REG_SP,UC_ARM64_REG_PC

ROOT=Path(__file__).resolve().parents[1]
BASE=0x7500000000


def main():
    sdk=ROOT/'build/native/binaries/13.0.4/sdk.elf'
    if hashlib.sha256(sdk.read_bytes()).hexdigest()!='d42578f463806dbd0023752dcecc43fed230c9f35d3a787696c3e92dd08eccdd':raise RuntimeError('Unexpected SDK')
    image=(ROOT/'build/web-nvn-layout/sdk-layout-image.bin').read_bytes()
    uc=Uc(UC_ARCH_ARM64,UC_MODE_ARM);uc.mem_map(BASE,(len(image)+4095)&~4095);uc.mem_write(BASE,image)
    storage,host_stack=0x7901000000,0x7300100000
    uc.mem_map(storage,4096);uc.mem_map(host_stack,65536)
    attempts=0;tail=[]
    def hook(machine,address,size,unused):
        nonlocal attempts
        if address==BASE+0x536a20:
            attempts+=1;machine.reg_write(UC_ARM64_REG_X0,0);machine.reg_write(UC_ARM64_REG_PC,machine.reg_read(UC_ARM64_REG_X30))
        elif address==BASE+0x536a60:
            tail.append([machine.reg_read(UC_ARM64_REG_X0),machine.reg_read(UC_ARM64_REG_X1)]);machine.emu_stop()
    uc.hook_add(UC_HOOK_CODE,hook)
    cases=[]
    for flags in (0,1):
        attempts=0;tail.clear();uc.mem_write(storage,bytes(256));uc.reg_write(UC_ARM64_REG_SP,host_stack+65520)
        values=[storage,0x71037cc4c0,storage,0x7100100000,0x100000,flags]
        for index,value in enumerate(values):uc.reg_write(UC_ARM64_REG_X0+index,value)
        uc.reg_write(UC_ARM64_REG_X30,host_stack)
        uc.emu_start(BASE+0x32eff0,host_stack,count=10000)
        data=bytes(uc.mem_read(storage,0x50))
        state=struct.unpack_from('<H',data,0x20)[0]
        function,argument,stack,active_stack,size=struct.unpack_from('<5Q',data,0x28)
        assert state==1 and [function,argument,stack,active_stack,size]==[values[1],values[2],values[3],values[3],values[4]]
        assert tail==[[storage,BASE+0x32f0d0]] and attempts==(1 if flags==0 else 0),(flags,tail,attempts)
        cases.append({'flags':flags,'aliasAllocationAttempts':attempts,'suppliedStackPreserved':True,'state':state})
    report={'ok':True,'cases':cases,'scope':'Actual SDK InitializeFiber prefix; alias allocator deliberately refuses requests for flags 0; stops before platform context setup'}
    path=ROOT/'build/sdk-fiber-flags.json';path.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))


if __name__=='__main__':main()
