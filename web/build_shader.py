"""Build the local browser shader compiler; source archive is checksum-pinned."""
import hashlib
import io
import json
import os
from pathlib import Path
import shutil
import subprocess
import struct
import tarfile
import urllib.request
from patch_naga import patch

ROOT=Path(__file__).resolve().parents[1]
NAGA=ROOT/'deps/naga-web-29.0.4'
SHA='b2bf919621e7975acb27d881bae2fb993e0d45c8e0446e85e6272971e00dc8df'


def main():
    if not NAGA.exists():
        archives=list((Path.home()/'.cargo/registry/cache').glob('*/naga-29.0.4.crate'))
        data=archives[0].read_bytes() if archives else urllib.request.urlopen('https://static.crates.io/crates/naga/naga-29.0.4.crate').read()
        if hashlib.sha256(data).hexdigest()!=SHA:raise ValueError('Naga crate checksum mismatch')
        with tarfile.open(fileobj=io.BytesIO(data),mode='r:gz') as archive:
            # Rust crates have one package prefix; validate before extracting.
            members=archive.getmembers()
            if any(not m.name.startswith('naga-29.0.4/') or '..' in Path(m.name).parts for m in members):
                raise ValueError('Unexpected archive path')
            staging=ROOT/'deps/web-shader-source';staging.mkdir(exist_ok=True)
            archive.extractall(staging,filter='data')
            source=(staging/'naga-29.0.4').resolve()
            if not source.is_relative_to((ROOT/'deps').resolve()) or not NAGA.resolve().is_relative_to((ROOT/'deps').resolve()):
                raise ValueError('Shader source move must stay inside project deps')
            shutil.move(str(source),str(NAGA))
    patch(NAGA)
    cargo=shutil.which('cargo') or str(Path.home()/'.cargo/bin/cargo.exe')
    environment=dict(os.environ,CARGO_TARGET_DIR=str(ROOT/'deps/web-shader-build'))
    subprocess.run([cargo,'build','--manifest-path',str(ROOT/'web/shader-compiler/Cargo.toml'),
                    '--target','wasm32-unknown-unknown','--release','--lib','--locked','-j','4'],env=environment,check=True)
    output=ROOT/'build/web-poc';output.mkdir(exist_ok=True,parents=True)
    shutil.copy2(ROOT/'deps/web-shader-build/wasm32-unknown-unknown/release/ssbu_web_shader.wasm',output/'shader-compiler.wasm')
    shutil.copy2(ROOT/'web/shader-compiler.mjs',output/'shader-compiler.mjs')
    shutil.copy2(ROOT/'web/shader-test.mjs',output/'shader-test.mjs')
    shutil.copy2(ROOT/'web/test-gpu-swizzle.mjs',output/'test-gpu-swizzle.mjs')
    fixture=ROOT/'build/web-shader-test.spv'
    subprocess.run([str(ROOT/'deps/glslang/bin/glslang.exe'),'-V','-S','comp',str(ROOT/'web/shader-test.comp'),'-o',str(fixture)],check=True)
    data=fixture.read_bytes()
    (output/'shader-test.json').write_text(json.dumps({'words':list(struct.unpack('<%dI'%(len(data)//4),data)),'expected':[7,6,4,99]})+'\n')
    depth_fixture=ROOT/'build/web-depth-sampling-test.spv'
    subprocess.run([str(ROOT/'deps/glslang/bin/glslang.exe'),'-V','-S','frag',str(ROOT/'web/depth-sampling-test.frag'),'-o',str(depth_fixture)],check=True)
    depth_data=depth_fixture.read_bytes()
    (output/'depth-sampling-test.json').write_text(json.dumps({'words':list(struct.unpack('<%dI'%(len(depth_data)//4),depth_data))})+'\n')
    gather_fixture=ROOT/'build/web-gather-test.spv'
    subprocess.run([str(ROOT/'deps/glslang/bin/glslang.exe'),'-V','-S','frag',str(ROOT/'web/gather-test.frag'),'-o',str(gather_fixture)],check=True)
    gather_data=gather_fixture.read_bytes()
    (output/'gather-test.json').write_text(json.dumps({'words':list(struct.unpack('<%dI'%(len(gather_data)//4),gather_data))})+'\n')
    subprocess.run([str(ROOT/'deps/glslang/bin/glslang.exe'),'-V','-DFIXTURE_CONSTANT=1','-S','frag',str(ROOT/'web/gather-test.frag'),'-o',str(gather_fixture)],check=True)
    gather_data=gather_fixture.read_bytes()
    (output/'gather-constant-test.json').write_text(json.dumps({'words':list(struct.unpack('<%dI'%(len(gather_data)//4),gather_data))})+'\n')
    viewport_fixture=ROOT/'build/web-viewport-test.spv'
    for name,defines in [('viewport-test',[]),('viewport-nonzero-test',['-DFIXTURE_NONZERO=1'])]:
        subprocess.run([str(ROOT/'deps/glslang/bin/glslang.exe'),'-V',*defines,'-S','vert',str(ROOT/'web/viewport-test.vert'),'-o',str(viewport_fixture)],check=True)
        viewport_data=viewport_fixture.read_bytes()
        (output/(name+'.json')).write_text(json.dumps({'words':list(struct.unpack('<%dI'%(len(viewport_data)//4),viewport_data))})+'\n')
    for name,defines in [('sample',[]),('fetch',['FETCH_IMAGE']),('gather',['GATHER_IMAGE']),('uint',['UINT_IMAGE']),('sint',['SINT_IMAGE']),('array',['ARRAY_IMAGE'])]:
        swizzle_fixture=ROOT/'build'/('swizzle-'+name+'.spv')
        subprocess.run([str(ROOT/'deps/glslang/bin/glslang.exe'),'-V','-S','frag',*['-D'+d+'=1' for d in defines],str(ROOT/'web/swizzle-test.frag'),'-o',str(swizzle_fixture)],check=True)
        swizzle_data=swizzle_fixture.read_bytes()
        (output/('swizzle-'+name+'.json')).write_text(json.dumps({'words':list(struct.unpack('<%dI'%(len(swizzle_data)//4),swizzle_data))})+'\n')
    print('Built shader-compiler.wasm',flush=True)


if __name__=='__main__':main()
