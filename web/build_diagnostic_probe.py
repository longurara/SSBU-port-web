"""Verify production fault handling and automatic Asyncify exclusion of reads."""
from pathlib import Path
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[1]


def main():
    out=ROOT/'build/web-diagnostics';out.mkdir(parents=True,exist_ok=True)
    command=[sys.executable,str(ROOT/'deps/emsdk/upstream/emscripten/emcc.py'),'-std=gnu11','-O1','-Wall','-Wextra',
      '-DRECOMP_WEB_MEMORY','-DWEB_FULL_RUNTIME','-pthread','-I',str(ROOT/'runtime'),'-I',str(ROOT/'web'),
      *[str(ROOT/'web'/name) for name in ('diagnostic_probe.c','runtime_memory.c','guest_memory.c','runtime_frame.c','runtime_trap.c')],
      '-sASYNCIFY=1','-sBINARYEN_EXTRA_PASSES=--optimize-level=1','-sASYNCIFY_ADVISE=1','-sSUPPORT_LONGJMP=emscripten','-sMODULARIZE=1','-sEXPORT_ES6=1',
      '-sENVIRONMENT=node','-sPTHREAD_POOL_SIZE=0','--no-entry',
      '-sEXPORTED_FUNCTIONS=["_diagnostic_check","_diagnostic_count","_diagnostic_memory","_diagnostic_read","_diagnostic_write","_diagnostic_switch"]',
      '-sEXPORTED_RUNTIME_METHODS=["PThread"]','-o',str(out/'probe.mjs')]
    with(out/'build.log').open('w',encoding='utf-8')as log:subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
    advice=(out/'build.log').read_text(encoding='utf-8')
    if '[asyncify] diagnostic_switch can change the state' not in advice:raise RuntimeError('Missing positive Asyncify analysis control')
    for name in ('diagnostic_read','diagnostic_write','web_memory_fault','guest_trap'):
        if f'[asyncify] {name} can change the state' in advice:raise RuntimeError(f'Synchronous diagnostic/memory function still marked as suspending: {name}')
    subprocess.run([str(ROOT/'deps/emsdk/node/24.19.0_64bit/node.exe'),str(ROOT/'web/test_diagnostics.mjs')],check=True,timeout=30)
    print('Asyncify analyzer: memory/trap functions do not suspend; genuine fiber switch still does',flush=True)


if __name__=='__main__':main()
