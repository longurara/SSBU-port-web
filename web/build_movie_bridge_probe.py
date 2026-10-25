"""Build only the C/JS movie bridge browser fixture, preserving the game build."""
from pathlib import Path
import shutil
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]

def main():
    out=ROOT/'build/web-movie-bridge'
    out.mkdir(parents=True,exist_ok=True)
    subprocess.run([sys.executable,str(ROOT/'deps/emsdk/upstream/emscripten/emcc.py'),'-std=gnu11','-O1',
        '-I',str(ROOT/'runtime'),'-I',str(ROOT/'web'),
        *[str(ROOT/'web'/n) for n in ['movie_bridge_probe.c','host_movie_bridge.c']],
        '--no-entry','-pthread','-sPTHREAD_POOL_SIZE=1','-sMODULARIZE=1','-sEXPORT_ES6=1','-sENVIRONMENT=worker',
        '-sINITIAL_MEMORY=16777216','-sMAXIMUM_MEMORY=134217728','-sALLOW_MEMORY_GROWTH=1',
        '-sFORCE_FILESYSTEM=1','-lworkerfs.js',
        '-sEXPORTED_RUNTIME_METHODS=["FS","WORKERFS","PThread","HEAPU8"]',
        '-o',str(out/'probe.mjs')],check=True)
    publish=ROOT/'build/web-poc/movie-bridge-probe'
    publish.mkdir(parents=True,exist_ok=True)
    for path in out.iterdir():
        if path.suffix in ['.mjs','.wasm','.js']:shutil.copy2(path,publish/path.name)
    shutil.copy2(ROOT/'web/movie_bridge_probe_worker.mjs',publish/'worker.mjs')
    for name in ['movie-backend.mjs','movie-webm.mjs','movie-video.mjs']:
        shutil.copy2(ROOT/'web'/name,ROOT/'build/web-poc'/name)
    print('Movie bridge fixture published without changing game Wasm or runtime-worker')

if __name__=='__main__':main()
