"""Build the dump-free fixture for actual NVN/WebGPU resource transfers."""
from pathlib import Path
import argparse
import shutil
import subprocess
import sys
from prepare_gpu_states import prepare as prepare_gpu_states
from prepare_nvn_layout import prepare as prepare_nvn_layout

ROOT=Path(__file__).resolve().parents[1]
WEB=ROOT/'web'


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--with-sdk',action='store_true');args=parser.parse_args()
    output=ROOT/'build/web-poc';output.mkdir(exist_ok=True,parents=True)
    sources=['gpu_probe.c','host_gpu.c','host_gpu_bridge.c','host_gpu_resources.c','host_gpu_commands.c','host_gpu_sampler.c','guest_memory.c','runtime_memory.c']
    sources += ['host_gpu_programs.c']
    sources += ['host_gpu_texture.c']
    sources += ['host_gpu_texture_objects.c','host_gpu_texture_views.c']
    sources += ['host_gpu_copy.c','host_gpu_render.c','host_gpu_draw.c']
    extra=[]
    if args.with_sdk:
        extra=['-DWEB_GPU_WITH_SDK',str(prepare_nvn_layout()),str(WEB/'host_nvn_layout.c'),
               '--embed-file',str(ROOT/'build/web-nvn-layout/sdk-layout-image.bin')+'@/sdk-layout-image.bin']
    subprocess.run([sys.executable,str(ROOT/'deps/emsdk/upstream/emscripten/emcc.py'),'-std=gnu11','-O1',
      '-Wall','-Wextra','-Wno-unused-label','-fno-strict-aliasing','-DRECOMP_WEB_MEMORY','-DWEB_FULL_RUNTIME',
      '-I',str(ROOT/'runtime'),'-I',str(WEB),*[str(WEB/name) for name in sources],str(prepare_gpu_states()),str(ROOT/'runtime/tex_decode.c'),
      *extra,'--no-entry','-pthread','-sPTHREAD_POOL_SIZE=1',f'-sINITIAL_MEMORY={33554432 if args.with_sdk else 16777216}','-sALLOW_MEMORY_GROWTH=1',
      '-sMAXIMUM_MEMORY=268435456','-sSTACK_SIZE=2097152','-sMODULARIZE=1','-sEXPORT_ES6=1',
      '-sENVIRONMENT=worker,node','-sASSERTIONS=1',
      '-sEXPORTED_FUNCTIONS=["_gpu_probe_start","_gpu_probe_join","_gpu_complete"]',
      '-sEXPORTED_RUNTIME_METHODS=["PThread","HEAPU8"]','-o',str(output/('gpu-texture-probe.mjs' if args.with_sdk else 'gpu-probe.mjs'))],check=True)
    for name in ['gpu-probe-worker.mjs','gpu-guest-pool-diagnostics.mjs','gpu-backend.mjs','gpu-programs.mjs','gpu-textures.mjs','gpu-samplers.mjs','gpu-render.mjs','gpu-draw.mjs','gpu-compute.mjs','gpu-depth-sampling.mjs','gpu-logical-sampling.mjs','spirv-uniform-branches.mjs','gpu-depth.mjs','wgsl-interface.mjs','test-gpu-resources.mjs','test-gpu-samplers.mjs','test-gpu-render.mjs','test-gpu-depth.mjs','test-gpu-present.mjs','test-gpu-depth-clear.mjs','test-gpu-draw.mjs','test-gpu-compute.mjs']:
        shutil.copy2(WEB/name,output/name)


if __name__=='__main__':main()
