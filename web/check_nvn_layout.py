"""Cross-check the Wasm SDK layout graph against AArch64 execution."""
import json
from pathlib import Path
import struct
import subprocess
import sys
from unicorn import Uc, UC_ARCH_ARM64, UC_MODE_ARM
from unicorn.arm64_const import UC_ARM64_REG_X0, UC_ARM64_REG_X1, UC_ARM64_REG_X30, UC_ARM64_REG_SP, UC_ARM64_REG_PC
from prepare_nvn_layout import prepare, BASE, SIZE

ROOT=Path(__file__).resolve().parents[1]


def main():
    source=prepare();out=source.parent;image=(out/'sdk-layout-image.bin').read_bytes()
    uc=Uc(UC_ARCH_ARM64,UC_MODE_ARM);uc.mem_map(BASE,SIZE);uc.mem_write(BASE,image)
    model,stack,stop,storage=0x7901000000,0x7300100000,0x7300000000,0x7300200000
    for address,size in ((model,0x4000),(stack,65536),(stop,4096),(storage,4096)):uc.mem_map(address,size)
    def put(address,value,width=8):uc.mem_write(address,value.to_bytes(width,'little'))
    put(model+0x2200,BASE+0xc114e8);put(model+0x2208,BASE+0xc11e88)
    put(model+0x2000,BASE+0xc11978);put(model+0x2140,model+0x2200)
    put(model+0x90,0x10000,4);put(model+0x928,model+0x2000)
    uc.mem_write(model+0x19a8,image[0x9f19a0:0x9f19b0]);put(model+0x1acc,1,1)
    known=[format for format in range(1,0xa1) if image[0xc26568+format*0x30]&1]
    inputs=[[0,1,128,64,1,1,format,0,0,0,0,0] for format in known]
    geometries=((1,1,1,1,1,0),(1,127,63,1,7,0),(4,128,64,6,7,0),(2,32,16,8,6,0),(5,128,64,1,1,4),(8,128,128,1,8,0),(10,100,1,1,1,0))
    for format in (1,5,0x25,0x31,0x32,0x33,0x35,0x36,0x3c,0x40,0x60,0x6f,0x7f,0x90,0xa0):
        if format not in known:continue
        for target,width,height,depth,levels,samples in geometries:
            for flags in (0,8,0x20):inputs.append([flags,target,width,height,depth,levels,format,samples,0,0,0,0])
        inputs.extend([[0x10,1,64,64,1,1,format,0,1024,0,0,0],[0x40,1,64,64,1,1,format,0,0,0,0,0],
                       [0,1,127,63,1,7,format,0,0,0,1,0x12]])
    cases=[]
    for fields in inputs:
        flags,target,width,height,depth,levels,format,samples,stride,glname,has_layout,layout=fields
        builder=bytearray(128);struct.pack_into('<Q',builder,8,model)
        struct.pack_into('<8I',builder,0x20,flags,target,width,height,depth,levels,format,samples)
        if has_layout:struct.pack_into('<H',builder,0x60,1|((layout&0xfff)<<4))
        struct.pack_into('<II',builder,0x64,stride,glname);uc.mem_write(storage,bytes(builder))
        results=[]
        for function in (0x4278b0,0x428760,0x427a10,0x427b30,0x4290b0):
            uc.mem_write(storage+128,bytes([0xaa])*16)
            for register,value in ((UC_ARM64_REG_X0,storage),(UC_ARM64_REG_X1,storage+128),
                                  (UC_ARM64_REG_X30,stop),(UC_ARM64_REG_SP,stack+65520)):uc.reg_write(register,value)
            try:uc.emu_start(BASE+function,stop,count=1000000)
            except Exception as error:raise RuntimeError(f'Layout oracle fields={fields}, function={function:x}, pc={uc.reg_read(UC_ARM64_REG_PC):x}') from error
            if uc.reg_read(UC_ARM64_REG_PC)!=stop:raise RuntimeError('Layout oracle did not return')
            if uc.reg_read(UC_ARM64_REG_SP)!=stack+65520:raise RuntimeError('Layout oracle did not restore SP')
            if function!=0x4290b0:results.append(uc.reg_read(UC_ARM64_REG_X0))
            else:results.extend(struct.unpack('<4I',bytes(uc.mem_read(storage+128,16))))
        if target==10:
            bpe=image[0xc26568+format*0x30+0x10];assert results[0]==(4 if bpe==12 else bpe) and results[1]==width*bpe
        if flags==0x40:assert results[0]==0x80
        cases.append(dict(fields=fields,expected=[str(value) for value in results]))
    (out/'reference.json').write_text(json.dumps(cases))
    sources=[ROOT/'web'/name for name in ('layout_probe.c','host_nvn_layout.c','guest_memory.c','runtime_memory.c')]
    subprocess.run([sys.executable,str(ROOT/'deps/emsdk/upstream/emscripten/emcc.py'),'-O1','-std=c11','-pthread',
       '-DRECOMP_WEB_MEMORY','-DWEB_FULL_RUNTIME','-I',str(ROOT/'runtime'),'-I',str(ROOT/'web'),*map(str,sources),str(source),
       '--embed-file',str(out/'sdk-layout-image.bin')+'@/sdk-layout-image.bin','--no-entry','-sMODULARIZE=1','-sEXPORT_ES6=1',
       '-sENVIRONMENT=node','-sINITIAL_MEMORY=33554432','-sALLOW_MEMORY_GROWTH=1','-sMAXIMUM_MEMORY=268435456',
       '-sEXPORTED_FUNCTIONS=["_layout_probe","_layout_probe_error","_malloc","_free"]',
       '-sEXPORTED_RUNTIME_METHODS=["HEAPU8","UTF8ToString","PThread"]','-o',str(out/'probe.mjs')],check=True)
    subprocess.run([str(ROOT/'deps/emsdk/node/24.19.0_64bit/node.exe'),str(ROOT/'web/test_nvn_layout.mjs')],check=True)


if __name__=='__main__':main()
