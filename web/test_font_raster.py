"""Run the portable Wasm raster fixture against local user-supplied TTF files."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--font-dir', type=Path, default=ROOT/'build/native/sysdata/fonts')
    args = parser.parse_args()
    fonts = sorted(args.font_dir.resolve().glob('*.ttf'))
    if not fonts:
        parser.error('No local .ttf inputs found; pass --font-dir')
    out = ROOT/'build/font-raster'
    out.mkdir(parents=True, exist_ok=True)
    emcc = ROOT/'deps/emsdk/upstream/emscripten/emcc.py'
    node = ROOT/'deps/emsdk/node/24.19.0_64bit/node.exe'
    sources = [ROOT/'web/font_raster.c', ROOT/'web/font_otf.c', ROOT/'web/font_raster_probe.c']
    command = [sys.executable, str(emcc), '-std=c11', '-O1', '-Wall', '-Wextra', '-Werror',
               '-pthread', '-sUSE_FREETYPE=1', '-sNODERAWFS=1', '-sINITIAL_MEMORY=67108864',
               '-sASSERTIONS=1', *map(str, sources), '-o', str(out/'probe.cjs')]
    built = subprocess.run(command, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (out/'build.log').write_text(built.stdout, encoding='utf-8')
    if built.returncode:
        print(built.stdout); raise SystemExit(built.returncode)
    run = subprocess.run([str(node), str(out/'probe.cjs'), *map(str, fonts)],
                         cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (out/'run.log').write_text(run.stdout, encoding='utf-8')
    report = {'ok': run.returncode == 0 and f'PASS: {len(fonts)} real local fonts;' in run.stdout,
              'scope': 'Standalone Wasm raster/metrics fixture, not guest ABI integration or game completion',
              'inputs': [{'name': p.name, 'bytes': p.stat().st_size} for p in fonts],
              'sources': {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                          for p in [*sources, ROOT/'web/font_raster.h', ROOT/'web/font_otf.h']},
              'wasmSha256': hashlib.sha256((out/'probe.wasm').read_bytes()).hexdigest(),
              'output': run.stdout}
    (out/'report.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print(run.stdout, end='')
    if not report['ok']:
        raise SystemExit(run.returncode or 1)


if __name__ == '__main__':
    main()
