"""Verify guest FiberType imports through actual Wasm pthreads, without a dump."""
from pathlib import Path
import argparse
import subprocess
import sys
import shutil
from fiber_toolchain import verify
ROOT=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--unaligned-control',action='store_true',help='expect the deliberate 8-byte C-stack control to fail before publishing')
    args=parser.parse_args()
    verify(ROOT)
    out=ROOT/('build/web-host-fiber-unaligned' if args.unaligned_control else 'build/web-host-fiber');out.mkdir(parents=True,exist_ok=True)
    command=[sys.executable,str(ROOT/'deps/emsdk/upstream/emscripten/emcc.py'),'-std=gnu11','-O1','-g','-pthread',
      '-DRECOMP_WEB_MEMORY','-DWEB_FULL_RUNTIME','-I',str(ROOT/'runtime'),'-I',str(ROOT/'web'),
      '-sASYNCIFY=1','-sBINARYEN_EXTRA_PASSES=--optimize-level=1','-sASYNCIFY_STACK_SIZE=1048576','-sSUPPORT_LONGJMP=emscripten',
      '-sPTHREAD_POOL_SIZE=2','-sMODULARIZE=1','-sEXPORT_ES6=1','-sENVIRONMENT=web,worker,node','-sASSERTIONS=1',
      '-sINITIAL_MEMORY=16777216','-sALLOW_MEMORY_GROWTH=1','-sMAXIMUM_MEMORY=134217728','--no-entry',
      '-sEXPORTED_FUNCTIONS=["_host_fiber_start","_host_fiber_join"]','-sEXPORTED_RUNTIME_METHODS=["PThread"]',
      *[str(ROOT/'web'/name) for name in ('host_fiber_probe.c','host_fiber.c','host_fiber_tls.c','runtime_frame.c','portable_fiber.c','fiber_migration_tls.c')],
      '-o',str(out/'probe.mjs')]
    if args.unaligned_control:command += [str(ROOT/'web/fiber_unaligned_control.c'),'-Wl,--wrap=aligned_alloc']
    subprocess.run(command,check=True)
    if args.unaligned_control:
        with(out/'control.log').open('w',encoding='utf-8')as log:
            result=subprocess.run([str(ROOT/'deps/emsdk/node/24.19.0_64bit/node.exe'),str(ROOT/'web/test_host_fiber.mjs'),'--unaligned-control'],stdout=log,stderr=subprocess.STDOUT,timeout=25)
        if result.returncode==0 or 'readEmAsmArgs' not in(out/'control.log').read_text(encoding='utf-8'):
            raise RuntimeError('Unaligned control did not fail at the expected varargs ABI boundary')
        print('Unaligned C-stack control failed at readEmAsmArgs as expected; not published');return
    subprocess.run([str(ROOT/'deps/emsdk/node/24.19.0_64bit/node.exe'),str(ROOT/'web/test_host_fiber.mjs')],check=True,timeout=25)
    publish=ROOT/'build/web-poc/host-fiber';publish.mkdir(parents=True,exist_ok=True)
    for suffix in ('mjs','wasm'):shutil.copy2(out/f'probe.{suffix}',publish/f'probe.{suffix}')
    shutil.copy2(ROOT/'web/host-fiber-worker.mjs',publish.parent/'host-fiber-worker.mjs')


if __name__=='__main__':main()
