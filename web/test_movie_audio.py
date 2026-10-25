"""Exercise production Vorbis decoding with real local media, preserving the game."""
from pathlib import Path
import argparse
import json
import re
import shutil
import subprocess
import urllib.request

ROOT=Path(__file__).resolve().parents[1]

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--movie',type=Path,required=True)
    p.add_argument('--offset',type=int,default=0)
    p.add_argument('--length',type=int)
    p.add_argument('--packets',type=int,default=256)
    p.add_argument('--track',type=int,default=2)
    p.add_argument('--session',default='ssbu-web')
    args=p.parse_args();movie=args.movie.resolve(strict=True)
    length=args.length if args.length is not None else movie.stat().st_size-args.offset
    if args.offset<0 or length<1 or args.offset+length>movie.stat().st_size or not 2<=args.packets<=10000:p.error('Invalid media range or packet count')
    cli=shutil.which('npx.cmd') or shutil.which('npx')
    if not cli:raise SystemExit('npx is required')
    for name in ['movie-webm.mjs','movie-audio.mjs']:
        source=ROOT/'web'/name;shutil.copy2(source,ROOT/'build/web-poc'/name)
        assert urllib.request.urlopen('http://127.0.0.1:8787/'+name,timeout=10).read()==source.read_bytes(),name
    out=ROOT/'build/movie-audio-probe';out.mkdir(exist_ok=True)
    template=(ROOT/'web/movie_audio_probe.cjs.in').read_text(encoding='utf-8')
    for key,value in {'MOVIE_PATH':json.dumps(str(movie)),'OFFSET':args.offset,'LENGTH':length,'TRACK':args.track,'PACKETS':args.packets}.items():template=template.replace('__'+key+'__',str(value))
    helper=out/'probe.cjs';helper.write_text(template,encoding='utf-8')
    result=subprocess.run([cli,'--yes','--package','@playwright/cli','playwright-cli',f'-s={args.session}','run-code','--filename',str(helper)],cwd=ROOT,capture_output=True,text=True,encoding='utf-8',timeout=90)
    output=result.stdout+result.stderr;(out/'probe.log').write_text(output,encoding='utf-8')
    m=re.search(r'^### Result\s*\n',output,re.M)
    if result.returncode or '### Error' in output or not m:raise SystemExit(output[:2000])
    report=json.JSONDecoder().raw_decode(output[m.end():])[0]
    report['sourceRange']={'offset':args.offset,'length':length,'track':args.track}
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    if len(report['packets'])!=args.packets or len(report['frames'])!=args.packets-1 or report['occupied'] or report['awaitingOutput']:raise SystemExit('Vorbis output count/reservations failed; inspect report')
    if any(f['bytes']!=f['samples']*f['channels']*2 for f in report['frames']):raise SystemExit('Invalid PCM size')
    if args.packets>=100 and (report['maxOccupied']!=64 or not report['backpressure']):raise SystemExit('Audio queue backpressure was not exercised')
    print(f"PASS: {len(report['frames'])} real Vorbis PCM blocks; report: {out/'report.json'}")

if __name__=='__main__':main()
