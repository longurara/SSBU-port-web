"""Build a stock negative control and a project-local migration adapter probe."""
from pathlib import Path
import subprocess
import sys
import shutil
from fiber_toolchain import verify
ROOT=Path(__file__).resolve().parents[1]


def main():
    verify(ROOT)
    out=ROOT/'build/web-fiber-migration';out.mkdir(parents=True,exist_ok=True)
    for baseline in (True,False):
        name='baseline' if baseline else 'probe'
        sources=[ROOT/'web'/item for item in ('fiber_migration_probe.c','fiber_migration_tls.c','portable_fiber.c')]
        command=[sys.executable,str(ROOT/'deps/emsdk/upstream/emscripten/emcc.py'),'-std=gnu11','-O1','-g','-pthread',
          '-sASYNCIFY=1','-sBINARYEN_EXTRA_PASSES=--optimize-level=1','-sASYNCIFY_STACK_SIZE=262144','-sSUPPORT_LONGJMP=emscripten','-sASYNCIFY_IGNORE_INDIRECT=0',
          '-sPTHREAD_POOL_SIZE=2','-sMODULARIZE=1','-sEXPORT_ES6=1','-sENVIRONMENT=web,worker,node','-sASSERTIONS=1',
          '-sINITIAL_MEMORY=16777216','-sALLOW_MEMORY_GROWTH=1','-sMAXIMUM_MEMORY=134217728','--no-entry',
          '-sEXPORTED_FUNCTIONS=["_migration_start","_migration_join"]','-sEXPORTED_RUNTIME_METHODS=["PThread"]',
          *map(str,sources),'-o',str(out/f'{name}.mjs')]
        if baseline:command.append('-DWEB_FIBER_BASELINE')
        subprocess.run(command,check=True)
        with (out/f'{name}.log').open('w',encoding='utf-8') as log:
            result=subprocess.run([str(ROOT/'deps/emsdk/node/24.19.0_64bit/node.exe'),str(ROOT/'web/test_fiber_migration.mjs'),*(['--baseline'] if baseline else [])],
                                  stdout=log,stderr=subprocess.STDOUT,timeout=25)
        if baseline:
            if result.returncode==0:raise RuntimeError('Stock control unexpectedly passed; inspect migration coverage before accepting the adapter')
            print('Stock control failed; inspect build/web-fiber-migration/baseline.log',flush=True)
        elif result.returncode:
            raise RuntimeError('Migration adapter failed; inspect build/web-fiber-migration/probe.log')
        else:print((out/'probe.log').read_text(encoding='utf-8'),flush=True)
    publish=ROOT/'build/web-poc/fiber-migration';publish.mkdir(parents=True,exist_ok=True)
    for suffix in ('mjs','wasm'):shutil.copy2(out/f'probe.{suffix}',publish/f'probe.{suffix}')
    shutil.copy2(ROOT/'web/fiber-migration-worker.mjs',publish.parent/'fiber-migration-worker.mjs')


if __name__=='__main__':main()
