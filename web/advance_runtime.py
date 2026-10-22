"""Boot locally and add only observed, already-lifted main callbacks.

This is a bounded development command, not a scheduler. It stops at any host
service/GPU failure, stale report or unsupported target; it never supplies stubs.
ROM/SDK inputs and all browser evidence stay under ignored build/.
"""
import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time
from prepare_runtime import BASE, load_bucket

ROOT=Path(__file__).resolve().parents[1]


def advance(args):
    executable=shutil.which('npx.cmd')
    if not executable:raise RuntimeError('Playwright CLI requires npx.cmd')
    roots_path=ROOT/'web/runtime_roots.json'
    history=ROOT/'build/runtime-progress'/str(time.time_ns());history.mkdir(parents=True)
    main=ROOT/'build/native/binaries/13.0.4/main.elf';romfs=ROOT/'build/dump-13.0.4/romfs'
    paths=[romfs/'data.arc',*sorted((romfs/'.nrr').glob('*.nrr'))]
    font_paths=sorted((ROOT/'build/native/sysdata/fonts').glob('*.ttf'))
    if len(font_paths)!=7:raise RuntimeError('Expected seven local system fonts')
    if not main.is_file() or len(paths)!=14 or any(not path.is_file() for path in paths):raise RuntimeError('Expected the verified local 13.0.4 dump')
    helper=history/'boot.cjs'
    helper.write_text('''async (page) => {
  await page.goto(RUNTIME_URL);
  await page.getByLabel('Chọn main.elf').setInputFiles(MAIN_PATH);
  await page.waitForFunction(() => window.__ssbuElfDone);
  await page.locator('#romfs').evaluate(el => el.removeAttribute('webkitdirectory'));
  await page.locator('#romfs').setInputFiles(ROMFS_PATHS);
  await page.locator('#fonts').evaluate(el => el.removeAttribute('webkitdirectory'));
  await page.locator('#fonts').setInputFiles(FONT_PATHS);
  await page.getByRole('button',{name:'Thử boot game',exact:true}).click();
  await page.waitForFunction(() => window.__ssbuRuntimeDone,{},{timeout:300000});
  const error=await page.evaluate(() => window.__ssbuRuntimeError);
  if(error)throw new Error(error);
}
'''.replace('RUNTIME_URL',json.dumps('http://127.0.0.1:8787/'+('?runtime=fibers-compact' if args.fibers and args.compact_codegen else '?runtime=fibers' if args.fibers else '?runtime=compact' if args.compact_codegen else '?runtime=debug'))).replace('MAIN_PATH',json.dumps(str(main))).replace('ROMFS_PATHS',json.dumps([str(path) for path in paths])).replace('FONT_PATHS',json.dumps([str(path) for path in font_paths])),encoding='utf-8')
    report_path=ROOT/'build/web-poc/runtime-browser-report.json'
    events=[]
    for index in range(args.steps):
        # Every report must come from this completed browser invocation, not the
        # previous successful diagnostic artifact left on disk.
        started=datetime.now(timezone.utc)
        with (history/f'boot-{index:02d}.log').open('w',encoding='utf-8') as log:
            subprocess.run([executable,'--yes','--package','@playwright/cli','playwright-cli',f'-s={args.session}',
                            'run-code','--filename',str(helper)],cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,check=True,timeout=320)
        report=json.loads(report_path.read_text(encoding='utf-8'))
        observed=datetime.fromisoformat(report['timestamp'].replace('Z','+00:00'))
        if observed<started:raise RuntimeError('Browser report is stale; no roots changed')
        shutil.copy2(report_path,history/f'report-{index:02d}.json')
        result=report['results']['runtime']
        event={key:result.get(key) for key in ('phase','error','address','import','importCalls','guestFunction','lr','sp','x0','x8','x19','x20','x21','x24','x28','threadFailed')};events.append(event)
        (history/'events.json').write_text(json.dumps(events,indent=2)+'\n')
        print(f'Step {index+1}: '+json.dumps(event),flush=True)
        if result.get('error')!='uncompiled indirect target':
            print(f'Stopped at a non-callback boundary; evidence: {history}',flush=True);return
        address_text=result.get('address','')
        if not re.fullmatch(r'[0-9a-f]{10}',address_text):raise RuntimeError('Unexpected target spelling; no roots changed')
        address=int(address_text,16)
        if not BASE<=address<BASE+0x39c7e90 or address%4:raise RuntimeError('Target outside the supported main image; no roots changed')
        definitions=load_bucket(ROOT/'build/native/work/gen_eh_o2',(address-BASE)//0xc000,{})
        if f'f_{address:x}' not in definitions:raise RuntimeError('Observed callback has no native lifted definition; no roots changed')
        roots=json.loads(roots_path.read_text(encoding='utf-8'))
        if roots.get('version')!='13.0.4':raise RuntimeError('Unexpected runtime root version')
        if address_text in roots['indirectTargets']:raise RuntimeError('Callback already included; investigate the build/browser instead of retrying')
        if index+1==args.steps:
            print(f'Step limit reached before editing the next root; evidence: {history}',flush=True);return
        roots['indirectTargets'].append(address_text)
        staged=history/'roots-next.json';staged.write_text(json.dumps(roots,indent=2)+'\n');staged.replace(roots_path)
        with (history/f'prepare-{index:02d}.log').open('w',encoding='utf-8') as log:
            subprocess.run([sys.executable,str(ROOT/'web/prepare_runtime.py'),'--main',*(['--pointer-tables'] if args.pointer_tables else [])],cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,check=True)
        with (history/f'build-{index:02d}.log').open('w',encoding='utf-8') as log:
            subprocess.run([sys.executable,str(ROOT/'web/build_runtime.py'),'--debug-graph','--jobs',str(args.jobs),*(['--fibers'] if args.fibers else []),*(['--compact-codegen'] if args.compact_codegen else [])],cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,check=True)
    print(f'Evidence: {history}',flush=True)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--steps',type=int,default=10);parser.add_argument('--session',default='ssbu-web')
    parser.add_argument('--fibers',action='store_true',help='boot/build the experimental Asyncify module instead of the default debug module')
    parser.add_argument('--compact-codegen',action='store_true',help='boot/build the compact code generation variant')
    parser.add_argument('--pointer-tables',action='store_true',help='include referenced pointer tables of explicitly observed callbacks when preparing the next graph')
    parser.add_argument('--jobs',type=int,default=2,help='parallel C compiler processes for each rebuild (1..16)')
    args=parser.parse_args()
    if not 1<=args.steps<=50:parser.error('--steps must be between 1 and 50')
    if not 1<=args.jobs<=16:parser.error('--jobs must be between 1 and 16')
    advance(args)
