"""Compile and exercise the production guest font adapter with local inputs."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
from font_gpos_fixture import make_fixture
ROOT=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--font-dir',type=Path,default=ROOT/'build/native/sysdata/fonts')
    args=parser.parse_args()
    folder=args.font_dir.resolve()
    if not folder.is_dir(): parser.error('Local font directory does not exist')
    out=ROOT/'build/host-font';out.mkdir(parents=True,exist_ok=True)
    make_fixture(folder/'nintendo_udsg-r_std_003.ttf',out/'gpos.ttf')
    pre=out/'mount.cjs'
    pre.write_text("Module.preRun=()=>{FS.mkdir('/fonts');FS.mount(NODEFS,{root:process.env.SSBU_TEST_FONT_DIR},'/fonts');FS.writeFile('/fixture-gpos.ttf',require('node:fs').readFileSync(process.env.SSBU_TEST_GPOS_FILE));};\n",encoding='utf-8')
    sources=[ROOT/'web'/n for n in ('host_font_probe.c','host_font.c','font_raster.c','font_otf.c','guest_memory.c','runtime_memory.c')]
    command=[sys.executable,str(ROOT/'deps/emsdk/upstream/emscripten/emcc.py'),'-std=c11','-O1',
             '-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-DRECOMP_WEB_MEMORY',
             '-I'+str(ROOT/'web'),'-I'+str(ROOT/'runtime'),'-pthread','-sUSE_FREETYPE=1',
             '-sPTHREAD_POOL_SIZE=2','-sINITIAL_MEMORY=268435456','-sASSERTIONS=1','-lnodefs.js',
             '--pre-js',str(pre),*map(str,sources),'-o',str(out/'probe.cjs')]
    build=subprocess.run(command,cwd=ROOT,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    (out/'build.log').write_text(build.stdout,encoding='utf-8')
    if build.returncode: print(build.stdout);raise SystemExit(build.returncode)
    env=os.environ.copy();env['SSBU_TEST_FONT_DIR']=str(folder)
    env['SSBU_TEST_GPOS_FILE']=str(out/'gpos.ttf')
    result=subprocess.run([str(ROOT/'deps/emsdk/node/24.19.0_64bit/node.exe'),str(out/'probe.cjs')],
                          cwd=ROOT,env=env,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=120)
    (out/'run.log').write_text(result.stdout,encoding='utf-8')
    report={'ok':result.returncode==0 and 'PASS:' in result.stdout,
            'scope':'Production C font adapter with real local fonts and a generated known-value GPOS variant in Wasm/Node; not actual game boot',
            'sources':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in [*sources,
                ROOT/'web/host_font.h',ROOT/'web/font_raster.h',ROOT/'web/font_otf.h',ROOT/'web/font_gpos_fixture.py']},
            'wasmSha256':hashlib.sha256((out/'probe.wasm').read_bytes()).hexdigest(),'output':result.stdout}
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(result.stdout,end='')
    if not report['ok']: raise SystemExit(result.returncode or 1)


if __name__=='__main__':main()
