"""Build only the C/JS movie bridge browser fixture, preserving the game build."""
from pathlib import Path
import shutil
import subprocess
import sys
import argparse

ROOT=Path(__file__).resolve().parents[1]

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk-extractor',action='store_true')
    args=parser.parse_args()
    out=ROOT/('build/web-movie-sdk-extractor' if args.sdk_extractor else 'build/web-movie-bridge')
    out.mkdir(parents=True,exist_ok=True)
    subprocess.run([sys.executable,str(ROOT/'deps/emsdk/upstream/emscripten/emcc.py'),'-std=gnu11','-O1',
        '-I',str(ROOT/'runtime'),'-I',str(ROOT/'web'),
        *[str(ROOT/'web'/n) for n in ['movie_bridge_probe.c','host_movie_bridge.c']],
        *(['-DMOVIE_SDK_EXTRACTOR','-DRECOMP_WEB_MEMORY','-DWEB_FULL_RUNTIME',*[str(ROOT/'web'/n) for n in ['movie_sdk_probe_support.c','host_movie.c','host_movie_codec.c','guest_memory.c','runtime_memory.c']]] if args.sdk_extractor else []),
        '--no-entry','-pthread','-sPTHREAD_POOL_SIZE=1','-sMODULARIZE=1','-sEXPORT_ES6=1','-sENVIRONMENT=worker',
        '-sINITIAL_MEMORY=16777216','-sMAXIMUM_MEMORY=134217728','-sALLOW_MEMORY_GROWTH=1',
        '-sFORCE_FILESYSTEM=1','-lworkerfs.js',
        '-sEXPORTED_RUNTIME_METHODS=["FS","WORKERFS","PThread","HEAPU8"]',
        '-o',str(out/'probe.mjs')],check=True)
    publish=ROOT/('build/web-poc/movie-sdk-extractor-probe' if args.sdk_extractor else 'build/web-poc/movie-bridge-probe')
    publish.mkdir(parents=True,exist_ok=True)
    for path in out.iterdir():
        if path.suffix in ['.mjs','.wasm','.js']:shutil.copy2(path,publish/path.name)
    shutil.copy2(ROOT/'web/movie_bridge_probe_worker.mjs',publish/'worker.mjs')
    for name in ['movie-backend.mjs','movie-webm.mjs','movie-video.mjs']:
        shutil.copy2(ROOT/'web'/name,ROOT/'build/web-poc'/name)
    print('Movie bridge fixture published without changing game Wasm or runtime-worker')

if __name__=='__main__':main()
