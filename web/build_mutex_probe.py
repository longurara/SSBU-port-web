"""Exercise real pthread mutex leases, ownership and more than 512 guest keys."""
from pathlib import Path
import subprocess
import sys
import shutil
ROOT=Path(__file__).resolve().parents[1]


def main():
    out=ROOT/'build/web-mutex';out.mkdir(parents=True,exist_ok=True)
    subprocess.run([sys.executable,str(ROOT/'deps/emsdk/upstream/emscripten/emcc.py'),'-std=gnu11','-O1','-Wall','-Wextra','-pthread',
      '-DRECOMP_WEB_MEMORY','-DWEB_FULL_RUNTIME','-I',str(ROOT/'web'),'-I',str(ROOT/'runtime'),
      *[str(ROOT/'web'/name)for name in ('mutex_probe.c','host_mutex.c','guest_memory.c','runtime_memory.c')],
      '--no-entry','-sSUPPORT_LONGJMP=emscripten','-sPTHREAD_POOL_SIZE=3','-sINITIAL_MEMORY=16777216','-sALLOW_MEMORY_GROWTH=1',
      '-sMAXIMUM_MEMORY=134217728','-sMODULARIZE=1','-sEXPORT_ES6=1','-sENVIRONMENT=worker,node','-sASSERTIONS=1',
      '-sEXPORTED_FUNCTIONS=["_mutex_probe_start","_mutex_probe_join"]','-sEXPORTED_RUNTIME_METHODS=["PThread"]',
      '-o',str(out/'probe.mjs')],check=True)
    subprocess.run([str(ROOT/'deps/emsdk/node/24.19.0_64bit/node.exe'),str(ROOT/'web/test_mutex.mjs')],check=True,timeout=30)
    publish=ROOT/'build/web-poc/mutex';publish.mkdir(parents=True,exist_ok=True)
    for suffix in ('mjs','wasm'):shutil.copy2(out/f'probe.{suffix}',publish/f'probe.{suffix}')
    shutil.copy2(ROOT/'web/mutex-probe-worker.mjs',publish.parent/'mutex-probe-worker.mjs')


if __name__=='__main__':main()
