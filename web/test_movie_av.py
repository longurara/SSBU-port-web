"""Check production SDK video/audio routing with real local interleaved packets."""
from pathlib import Path
import argparse
import json
import re
import shutil
import subprocess
import urllib.request

ROOT=Path(__file__).resolve().parents[1]
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--movie',type=Path,required=True)
    parser.add_argument('--offset',type=int,default=0)
    parser.add_argument('--length',type=int)
    parser.add_argument('--video-packets',type=int,default=120)
    parser.add_argument('--session',default='ssbu-web')
    args=parser.parse_args();movie=args.movie.resolve(strict=True);length=args.length if args.length is not None else movie.stat().st_size-args.offset
    if args.offset<0 or length<1 or args.offset+length>movie.stat().st_size or not 64<=args.video_packets<=128:parser.error('Invalid media range or fixture count (64..128)')
    cli=shutil.which('npx.cmd') or shutil.which('npx')
    if not cli:raise SystemExit('npx is required')
    for name in ['movie-webm.mjs','movie-video.mjs','movie-audio.mjs','movie-backend.mjs','movie_av_probe_excerpt.mjs']:
        source=ROOT/'web'/name;shutil.copy2(source,ROOT/'build/web-poc'/name)
        assert urllib.request.urlopen('http://127.0.0.1:8787/'+name,timeout=10).read()==source.read_bytes(),name
    out=ROOT/'build/movie-sdk-av-probe';out.mkdir(exist_ok=True)
    template=(ROOT/'web/movie_av_probe.cjs.in').read_text(encoding='utf-8')
    for key,value in {'MOVIE_PATH':json.dumps(str(movie)),'OFFSET':args.offset,'LENGTH':length,'PACKETS':args.video_packets}.items():template=template.replace('__'+key+'__',str(value))
    helper=out/'probe.cjs';helper.write_text(template,encoding='utf-8')
    result=subprocess.run([cli,'--yes','--package','@playwright/cli','playwright-cli',f'-s={args.session}','run-code','--filename',str(helper)],cwd=ROOT,capture_output=True,text=True,encoding='utf-8',timeout=90)
    output=result.stdout+result.stderr;(out/'probe.log').write_text(output,encoding='utf-8');m=re.search(r'^### Result\s*\n',output,re.M)
    if result.returncode or '### Error' in output or not m:raise SystemExit(output[:2000])
    report=json.JSONDecoder().raw_decode(output[m.end():])[0];report['sourceRange']={'offset':args.offset,'length':length}
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    if not report['ok'] or not report['hashesMatch'] or report['liveHandles'] or report['pendingCompletions'] or report['eosVideo']!=1 or report['eosAudio']!=1 or report['maxHeldVideo']!=48 or report['maxHeldAudio']!=64:raise SystemExit('SDK AV fixture failed; inspect report.json')
    print(f"PASS: {len(report['videoFrames'])} SDK video + {len(report['audioFrames'])} SDK PCM outputs, direct hashes/timestamps match; {out/'report.json'}")
if __name__=='__main__':main()
