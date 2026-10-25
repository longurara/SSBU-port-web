"""Build and run SDK value ABI/guest allocator integration checks (no game link)."""
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]

def main():
    out=ROOT/'build/web-movie-values'
    out.mkdir(parents=True,exist_ok=True)
    subprocess.run([sys.executable,str(ROOT/'deps/emsdk/upstream/emscripten/emcc.py'),
        '-std=c11','-O1','-DRECOMP_WEB_MEMORY','-DWEB_FULL_RUNTIME',
        '-I',str(ROOT/'runtime'),'-I',str(ROOT/'web'),
        *[str(ROOT/'web'/n) for n in ['movie_values_probe.c','host_movie.c','guest_memory.c','runtime_memory.c']],
        '--no-entry','-pthread','-sPTHREAD_POOL_SIZE=2','-sMODULARIZE=1','-sEXPORT_ES6=1','-sENVIRONMENT=node',
        '-sINITIAL_MEMORY=16777216','-sMAXIMUM_MEMORY=67108864','-sALLOW_MEMORY_GROWTH=1',
        '-sEXPORTED_FUNCTIONS=["_movie_values_probe"]','-sEXPORTED_RUNTIME_METHODS=["ccall"]',
        '-o',str(out/'probe.mjs')],check=True)
    subprocess.run([str(ROOT/'deps/emsdk/node/24.19.0_64bit/node.exe'),str(ROOT/'web/test_movie_values.mjs')],check=True)

if __name__=='__main__':main()
