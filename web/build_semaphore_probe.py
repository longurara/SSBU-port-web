"""Verify the semaphore adapter on real Wasm pthreads, without game data."""
from pathlib import Path
import subprocess
import sys
import shutil
ROOT=Path(__file__).resolve().parents[1]


def main():
    out=ROOT/'build/web-semaphore';out.mkdir(parents=True,exist_ok=True)
    subprocess.run([sys.executable,str(ROOT/'deps/emsdk/upstream/emscripten/emcc.py'),'-std=gnu11','-O1','-Wall','-Wextra','-pthread',
      '-I',str(ROOT/'web'),'-I',str(ROOT/'runtime'),'-DRECOMP_WEB_MEMORY','-DWEB_FULL_RUNTIME',
      str(ROOT/'web/semaphore_probe.c'),str(ROOT/'web/host_semaphore.c'),
      '--no-entry','-sSUPPORT_LONGJMP=emscripten','-sPTHREAD_POOL_SIZE=3','-sMODULARIZE=1','-sEXPORT_ES6=1','-sENVIRONMENT=worker,node',
      '-sASSERTIONS=1','-sEXPORTED_FUNCTIONS=["_semaphore_probe_start","_semaphore_probe_join"]','-sEXPORTED_RUNTIME_METHODS=["PThread"]',
      '-o',str(out/'probe.mjs')],check=True)
    subprocess.run([str(ROOT/'deps/emsdk/node/24.19.0_64bit/node.exe'),str(ROOT/'web/test_semaphore.mjs')],check=True,timeout=25)
    publish=ROOT/'build/web-poc/semaphore';publish.mkdir(parents=True,exist_ok=True)
    for suffix in ('mjs','wasm'):shutil.copy2(out/f'probe.{suffix}',publish/f'probe.{suffix}')
    shutil.copy2(ROOT/'web/semaphore-probe-worker.mjs',publish.parent/'semaphore-probe-worker.mjs')


if __name__=='__main__':main()
