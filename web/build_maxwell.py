"""Build the actual Maxwell translator + GLSL compiler for browser Workers."""
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
PIN='e1b562a8bed273a02f30b59b66a5d499793cede5'


def main():
    source=ROOT/'deps/glslang-source'
    if not source.exists():
        subprocess.run(['git','clone','--depth','1','--branch','16.6.0','https://github.com/KhronosGroup/glslang.git',str(source)],check=True)
    if subprocess.check_output(['git','-C',str(source),'rev-parse','HEAD'],text=True).strip()!=PIN:
        raise ValueError('Expected pinned glslang 16.6.0 checkout')
    cmake=shutil.which('cmake');ninja=shutil.which('ninja')
    if not cmake or not ninja:
        vswhere=Path(os.environ.get('ProgramFiles(x86)','C:/Program Files (x86)'))/'Microsoft Visual Studio/Installer/vswhere.exe'
        installation=subprocess.check_output([str(vswhere),'-latest','-prerelease','-property','installationPath'],text=True).strip()
        bundled=Path(installation)/'Common7/IDE/CommonExtensions/Microsoft/CMake'
        cmake=cmake or str(bundled/'CMake/bin/cmake.exe')
        ninja=ninja or str(bundled/'Ninja/ninja.exe')
    environment=dict(os.environ)
    environment['PATH']=str(Path(cmake).parent)+os.pathsep+str(Path(ninja).parent)+os.pathsep+environment.get('PATH','')
    emsdk=ROOT/'deps/emsdk/upstream/emscripten'
    build=ROOT/'deps/maxwell-wasm-build'
    subprocess.run([sys.executable,str(emsdk/'emcmake.py'),cmake,'-S',str(ROOT/'web/maxwell'),'-B',str(build),'-G','Ninja',
                    '-DCMAKE_BUILD_TYPE=Release','-DCMAKE_MAKE_PROGRAM='+ninja],env=environment,check=True)
    subprocess.run([cmake,'--build',str(build),'--target','maxwell','--parallel','4'],env=environment,check=True)
    shutil.copy2(ROOT/'web/maxwell-pipeline.mjs',ROOT/'build/web-poc/maxwell-pipeline.mjs')
    shutil.copy2(ROOT/'web/spirv-compat.mjs',ROOT/'build/web-poc/spirv-compat.mjs')
    shutil.copy2(ROOT/'web/game-shaders.mjs',ROOT/'build/web-poc/game-shaders.mjs')
    shutil.copy2(ROOT/'web/test-game-shaders.mjs',ROOT/'build/web-poc/test-game-shaders.mjs')
    shutil.copy2(ROOT/'web/test-gpu-program-snapshots.mjs',ROOT/'build/web-poc/test-gpu-program-snapshots.mjs')
    for name in ('gpu-backend.mjs','gpu-programs.mjs','gpu-textures.mjs','gpu-samplers.mjs','gpu-render.mjs','gpu-draw.mjs','gpu-compute.mjs','gpu-depth-sampling.mjs','gpu-logical-sampling.mjs','spirv-uniform-branches.mjs','gpu-depth.mjs','wgsl-interface.mjs'):
        shutil.copy2(ROOT/'web'/name,ROOT/'build/web-poc'/name)


if __name__=='__main__':main()
