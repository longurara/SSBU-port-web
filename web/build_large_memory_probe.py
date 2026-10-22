"""Build the small dump-free shared wasm32 >2GiB pointer fixture."""
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
if __name__ == '__main__':
    subprocess.run([sys.executable, str(ROOT/'deps/emsdk/upstream/emscripten/emcc.py'),
        str(ROOT/'web/large_memory_probe.c'),str(ROOT/'web/host_allocation_stats.c'), '--no-entry', '-O1', '-pthread',
        '-sPTHREAD_POOL_SIZE=0', '-sMODULARIZE=1', '-sEXPORT_ES6=1',
        '-sENVIRONMENT=web,worker,node', '-sINITIAL_MEMORY=16777216',
        '-sALLOW_MEMORY_GROWTH=1', '-sMAXIMUM_MEMORY=4294967296',
        '-sEXPORTED_RUNTIME_METHODS=HEAPU8', '-o',
        str(ROOT/'build/web-poc/large-memory-probe.mjs')], check=True)
