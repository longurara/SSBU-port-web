"""Build the dump-free pthread/LightEvent import integration fixture."""
from pathlib import Path
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[1]


def main():
    output=ROOT/'build/web-os';output.mkdir(parents=True,exist_ok=True)
    subprocess.run([sys.executable,str(ROOT/'deps/emsdk/upstream/emscripten/emcc.py'),'-std=gnu11','-O1',
       '-DRECOMP_WEB_MEMORY','-DWEB_FULL_RUNTIME','-I',str(ROOT/'runtime'),'-I',str(ROOT/'web'),
       *[str(ROOT/'web'/name) for name in ('os_probe.c','host_os.c','host_font.c','font_raster.c','font_otf.c','host_fs.c','host_mutex.c','guest_sort.c','guest_memory.c','runtime_memory.c')],
       '-sUSE_FREETYPE=1',
       '--no-entry','-pthread','-sPTHREAD_POOL_SIZE=3','-sMODULARIZE=1','-sEXPORT_ES6=1','-sENVIRONMENT=node',
       '-sINITIAL_MEMORY=16777216','-sALLOW_MEMORY_GROWTH=1','-sMAXIMUM_MEMORY=67108864',
       '-sEXPORTED_FUNCTIONS=["_os_probe_start","_os_probe_join"]','-sEXPORTED_RUNTIME_METHODS=["PThread","HEAPU8"]',
       '-o',str(output/'probe.mjs')],check=True)
    subprocess.run([str(ROOT/'deps/emsdk/node/24.19.0_64bit/node.exe'),str(ROOT/'web/test_os_events.mjs')],check=True)


if __name__=='__main__':main()
