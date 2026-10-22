"""Compare unsigned-sort Wasm adapters with actual AArch64 SDK execution."""
import base64
import json
from pathlib import Path
import random
import struct
import subprocess
import sys
from unicorn import Uc, UC_ARCH_ARM64, UC_MODE_ARM
from unicorn.arm64_const import UC_ARM64_REG_X0, UC_ARM64_REG_X1, UC_ARM64_REG_X2, UC_ARM64_REG_X30, UC_ARM64_REG_SP, UC_ARM64_REG_PC

ROOT = Path(__file__).resolve().parents[1]


def main():
    out = ROOT / 'build/web-sort'
    out.mkdir(parents=True, exist_ok=True)
    image = (ROOT / 'build/native/binaries/13.0.4/sdk.elf').read_bytes()
    uc = Uc(UC_ARCH_ARM64, UC_MODE_ARM)
    sdk, stack, stop, region = 0x7500000000, 0x7300100000, 0x7300000000, 0x7300200000
    uc.mem_map(sdk, 0xcfb000)
    phoff = struct.unpack_from('<Q', image, 32)[0]
    dynamic = None
    for i in range(struct.unpack_from('<H', image, 56)[0]):
        kind, _, offset, address, _, size, _, _ = struct.unpack_from('<IIQQQQQQ', image, phoff+i*56)
        if kind == 1:
            uc.mem_write(sdk+address, image[offset:offset+size])
        elif kind == 2:
            dynamic = (address,size)
    tags = {}
    if dynamic:
        for offset in range(0,dynamic[1],16):
            tag,value=struct.unpack('<QQ',bytes(uc.mem_read(sdk+dynamic[0]+offset,16)))
            if not tag:
                break
            tags[tag]=value
    # Function pointers in the SDK GOT must use the same relocated base as code.
    for start,length in ((tags.get(7,0),tags.get(8,0)),(tags.get(23,0),tags.get(2,0))):
        for offset in range(0,length,24):
            target,info,addend=struct.unpack('<QQq',bytes(uc.mem_read(sdk+start+offset,24)))
            if (info & 0xffffffff)==1027:
                uc.mem_write(sdk+target,struct.pack('<Q',sdk+addend))
            elif (info & 0xffffffff) in (257,1025,1026):
                symbol=bytes(uc.mem_read(sdk+tags[6]+(info>>32)*24,24))
                _,_,_,section,value,_=struct.unpack('<IBBHQQ',symbol)
                if section:
                    uc.mem_write(sdk+target,struct.pack('<Q',sdk+value+addend))
    for address in (stack, region):
        uc.mem_map(address, 65536)
    uc.mem_map(stop, 4096)
    rng = random.Random(0x53534255)
    cases = []
    for width, function in ((4, 0x7500459ec0), (8, 0x750045caf8)):
        mask = (1 << (width*8))-1
        arrays = [[], [mask], [mask, 0], [0, mask, mask//2, 1, 1 << (width*8-1)]]
        for count in (3, 4, 5, 16, 31, 32, 33, 128, 1025, 2049):
            arrays += [list(range(count)), list(reversed(range(count))), [mask]*count,
                       [rng.getrandbits(width*8) for _ in range(count)]]
        for values in arrays:
            raw = b''.join(value.to_bytes(width, 'little') for value in values)
            address = region+4092  # exercise unaligned 64-bit loads and page crossings
            uc.mem_write(region, bytes(65536))
            uc.mem_write(address-4, b'PRE!'+raw+b'POST')
            for reg, value in ((UC_ARM64_REG_X0,address), (UC_ARM64_REG_X1,address+len(raw)),
                               (UC_ARM64_REG_X2,stack), (UC_ARM64_REG_X30,stop), (UC_ARM64_REG_SP,stack+65520)):
                uc.reg_write(reg,value)
            try:
                uc.emu_start(function,stop,count=10000000)
            except Exception as error:
                raise RuntimeError(f'SDK sort width={width} count={len(values)} pc={uc.reg_read(UC_ARM64_REG_PC):x} lr={uc.reg_read(UC_ARM64_REG_X30):x}') from error
            if uc.reg_read(UC_ARM64_REG_PC)!=stop:
                raise RuntimeError('SDK sort did not return')
            result=bytes(uc.mem_read(address,len(raw)))
            assert result==b''.join(value.to_bytes(width,'little') for value in sorted(values))
            assert bytes(uc.mem_read(address-4,4))==b'PRE!'
            assert bytes(uc.mem_read(address+len(raw),4))==b'POST'
            cases.append(dict(width=width,input=base64.b64encode(raw).decode(),output=base64.b64encode(result).decode()))
    (out/'reference.json').write_text(json.dumps(cases))
    sources = [ROOT/'web'/name for name in ('sort_probe.c','guest_sort.c','guest_memory.c','runtime_memory.c')]
    subprocess.run([sys.executable,str(ROOT/'deps/emsdk/upstream/emscripten/emcc.py'),'-O1','-std=c11',
                    '-DRECOMP_WEB_MEMORY','-DWEB_FULL_RUNTIME','-I',str(ROOT/'runtime'),'-I',str(ROOT/'web'),
                    *map(str,sources),'--no-entry','-sMODULARIZE=1','-sEXPORT_ES6=1','-sENVIRONMENT=node',
                    '-sEXPORTED_FUNCTIONS=["_sort_probe","_malloc","_free"]','-sEXPORTED_RUNTIME_METHODS=["HEAPU8"]',
                    '-o',str(out/'probe.mjs')],check=True)
    subprocess.run([str(ROOT/'deps/emsdk/node/24.19.0_64bit/node.exe'),str(ROOT/'web/test_sdk_sort.mjs')],check=True)


if __name__ == '__main__':
    main()
