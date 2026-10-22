"""Prepare a real-browser WORKERFS fixture for the production font C adapter.

Run the printed Playwright helper in a separate browser session. The existing
local server serves build/web-poc; no input font bytes are uploaded to it.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import subprocess
import sys
from font_gpos_fixture import make_fixture

ROOT=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--font-dir',type=Path,default=ROOT/'build/native/sysdata/fonts')
    parser.add_argument('--verify',action='store_true',help='archive a successful build/font-browser/run.log against the compiled local fixture')
    args=parser.parse_args()
    out=ROOT/'build/web-poc/font-probe';out.mkdir(parents=True,exist_ok=True)
    local=ROOT/'build/font-browser';local.mkdir(parents=True,exist_ok=True)
    if args.verify:
        raw=(local/'run.log').read_text(encoding='utf-8-sig')
        if '### Error' in raw:raise RuntimeError('Browser helper failed')
        result=json.loads(raw.split('### Result',1)[1].strip().splitlines()[0])
        if not result.get('ok') or result.get('crossOriginIsolated') is not True:raise RuntimeError(result)
        manifest=json.loads((local/'build.json').read_text())
        if hashlib.sha256((out/'probe.wasm').read_bytes()).hexdigest()!=manifest['wasmSha256']:raise RuntimeError('Fixture publication changed')
        for name,digest in manifest['sources'].items():
            if hashlib.sha256((ROOT/name).read_bytes()).hexdigest()!=digest:raise RuntimeError('Fixture sources changed')
        report={'timestamp':datetime.now(timezone.utc).isoformat(),'results':{'fontBrowser':result},**manifest,
                'shaScope':'Local fixture publication hash, not independently hashed in browser'}
        (local/'report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
        print('Verified build/font-browser/report.json');return
    folder=args.font_dir.resolve();fonts=sorted(folder.glob('*.ttf'))
    if len(fonts)!=7:parser.error('Expected the seven local system TTF inputs')
    # Keep the font variant out of the served directory. Browser receives it
    # through a local file input, just as it receives the original fonts.
    make_fixture(folder/'nintendo_udsg-r_std_003.ttf',local/'gpos.ttf')
    sources=[ROOT/'web'/n for n in ('host_font_probe.c','host_font.c','font_raster.c','font_otf.c','guest_memory.c','runtime_memory.c')]
    command=[sys.executable,str(ROOT/'deps/emsdk/upstream/emscripten/emcc.py'),'-std=c11','-O1',
             '-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-DRECOMP_WEB_MEMORY',
             '-I'+str(ROOT/'web'),'-I'+str(ROOT/'runtime'),'-pthread','-sUSE_FREETYPE=1',
             '-sPTHREAD_POOL_SIZE=3','-sPROXY_TO_PTHREAD=1','-sINITIAL_MEMORY=268435456','-sASSERTIONS=1',
             '-sMODULARIZE=1','-sEXPORT_ES6=1','-sENVIRONMENT=worker','-lworkerfs.js',
             '-sINVOKE_RUN=0','-sEXPORTED_RUNTIME_METHODS=["FS","WORKERFS","PThread","callMain"]',
             *map(str,sources),'-o',str(out/'probe.mjs')]
    result=subprocess.run(command,cwd=ROOT,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    (local/'build.log').write_text(result.stdout,encoding='utf-8')
    if result.returncode:print(result.stdout);raise SystemExit(result.returncode)
    inputs=[*sources,*(ROOT/'web'/name for name in ('host_font.h','font_raster.h','font_otf.h','guest_memory.h','runtime_memory.h')),ROOT/'runtime/runtime.h']
    manifest={'wasmSha256':hashlib.sha256((out/'probe.wasm').read_bytes()).hexdigest(),
              'sources':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs}}
    (local/'build.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    (out/'worker.mjs').write_text('''import create from './probe.mjs';
self.onmessage=async({data})=>{
 let module,aborted;const logs=[];
 try{
  module=await create({noInitialRun:true,print:s=>logs.push(s),printErr:s=>logs.push(s),onAbort:s=>{aborted=String(s);}});
  module.FS.mkdir('/fonts');module.FS.mount(module.WORKERFS,{blobs:data.fonts.map(file=>({name:file.name,data:file}))},'/fonts');
  module.FS.writeFile('/fixture-gpos.ttf',new Uint8Array(await data.gpos.arrayBuffer()));
  module.callMain([]);
  const deadline=performance.now()+55000;
  while(!logs.some(s=>s.startsWith('PASS:'))&&!aborted&&performance.now()<deadline)await new Promise(resolve=>setTimeout(resolve,25));
  if(aborted)throw Error(aborted);
  if(!logs.some(s=>s.startsWith('PASS:')))throw Error('Font pthread fixture timed out');
  self.postMessage({ok:logs.some(s=>s.startsWith('PASS:')),logs,crossOriginIsolated:self.crossOriginIsolated,
    scope:'Production C font adapter on a runtime pthread, File/WORKERFS proxied to owner worker, two additional pthread waiters in Chrome; not game boot'});
 }catch(error){self.postMessage({ok:false,error:String(error),logs});}
 finally{module?.PThread.terminateAllThreads();self.close();}
};
''',encoding='utf-8')
    (out/'index.html').write_text('''<!doctype html><meta charset="utf-8"><title>Local font adapter fixture</title>
<label>System fonts<input id="fonts" type="file" multiple></label>
<label>Known GPOS variant<input id="gpos" type="file"></label><button id="start">Run font fixture</button><pre id="result">Ready</pre>
<script type="module">
document.querySelector('#start').onclick=()=>{
 window.fontProbeResult=null;const worker=new Worker('./worker.mjs',{type:'module'});
 worker.onmessage=({data})=>{window.fontProbeResult=data;document.querySelector('#result').textContent=JSON.stringify(data,null,2);};
 worker.onerror=event=>{window.fontProbeResult={ok:false,error:event.message};};
 worker.postMessage({fonts:[...document.querySelector('#fonts').files],gpos:document.querySelector('#gpos').files[0]});
};
</script>''',encoding='utf-8')
    helper='''async(page)=>{
 if(!page.url().includes('/font-probe/'))throw Error('Use a separate font-probe browser session');
 await page.locator('#fonts').setInputFiles(FONT_PATHS);
 await page.locator('#gpos').setInputFiles(GPOS_PATH);
 await page.locator('#start').click();
 await page.waitForFunction(()=>window.fontProbeResult,{},{timeout:60000});
 return await page.evaluate(()=>window.fontProbeResult);
}
'''.replace('FONT_PATHS',json.dumps([str(p) for p in fonts])).replace('GPOS_PATH',json.dumps(str(local/'gpos.ttf')))
    (local/'run.cjs').write_text(helper,encoding='utf-8')
    print('Prepared http://127.0.0.1:8787/font-probe/index.html and build/font-browser/run.cjs')


if __name__=='__main__':main()
