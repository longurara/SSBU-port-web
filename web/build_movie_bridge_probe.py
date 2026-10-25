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
    parser.add_argument('--sdk-decoder',action='store_true')
    parser.add_argument('--sdk-av',action='store_true')
    args=parser.parse_args()
    if args.sdk_av:args.sdk_decoder=True
    if args.sdk_decoder:args.sdk_extractor=True
    out=ROOT/('build/web-movie-sdk-av' if args.sdk_av else 'build/web-movie-sdk-decoder' if args.sdk_decoder else 'build/web-movie-sdk-extractor' if args.sdk_extractor else 'build/web-movie-bridge')
    out.mkdir(parents=True,exist_ok=True)
    subprocess.run([sys.executable,str(ROOT/'deps/emsdk/upstream/emscripten/emcc.py'),'-std=gnu11','-O1',
        '-I',str(ROOT/'runtime'),'-I',str(ROOT/'web'),
        *[str(ROOT/'web'/n) for n in ['movie_av_probe.c' if args.sdk_av else 'movie_decoder_probe.c' if args.sdk_decoder else 'movie_bridge_probe.c','host_movie_bridge.c']],
        *(['-DMOVIE_SDK_EXTRACTOR','-DRECOMP_WEB_MEMORY','-DWEB_FULL_RUNTIME','-sUSE_FREETYPE=1',*[str(ROOT/'web'/n) for n in ['movie_sdk_probe_support.c','host_movie.c','host_movie_codec.c','host_movie_decoder.c','host_os.c','host_font.c','font_raster.c','font_otf.c','host_fs.c','host_mutex.c','guest_sort.c','guest_memory.c','runtime_memory.c']]] if args.sdk_extractor else []),
        '--no-entry','-pthread',f'-sPTHREAD_POOL_SIZE={3 if args.sdk_decoder else 1}','-sMODULARIZE=1','-sEXPORT_ES6=1','-sENVIRONMENT=worker',
        '-sINITIAL_MEMORY=16777216','-sMAXIMUM_MEMORY=134217728','-sALLOW_MEMORY_GROWTH=1',
        '-sFORCE_FILESYSTEM=1','-lworkerfs.js',
        '-sEXPORTED_RUNTIME_METHODS=["FS","WORKERFS","PThread","HEAPU8"]',
        '-o',str(out/'probe.mjs')],check=True)
    publish=ROOT/('build/web-poc/movie-sdk-av-probe' if args.sdk_av else 'build/web-poc/movie-sdk-decoder-probe' if args.sdk_decoder else 'build/web-poc/movie-sdk-extractor-probe' if args.sdk_extractor else 'build/web-poc/movie-bridge-probe')
    publish.mkdir(parents=True,exist_ok=True)
    for path in out.iterdir():
        if path.suffix in ['.mjs','.wasm','.js']:shutil.copy2(path,publish/path.name)
    shutil.copy2(ROOT/'web/movie_bridge_probe_worker.mjs',publish/'worker.mjs')
    for name in ['movie-backend.mjs','movie-webm.mjs','movie-video.mjs','movie-audio.mjs']:
        shutil.copy2(ROOT/'web'/name,ROOT/'build/web-poc'/name)
    print('Movie bridge fixture published without changing game Wasm or runtime-worker')

if __name__=='__main__':main()
