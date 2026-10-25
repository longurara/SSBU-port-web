"""Decode real local WebM samples in a temporary Playwright page.

Requires the existing web/serve.py server and a Playwright CLI browser session.
Keeps its original page (including a running game) intact. Media stays local
and is selected through a temporary file input, never added to served assets.
Usage: python web/test_movie_video.py --movie path/to/local.webm
"""
from pathlib import Path
import argparse
import hashlib
import json
import re
import shutil
import subprocess
import urllib.request

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--movie', type=Path, required=True)
    parser.add_argument('--session', default='ssbu-web')
    parser.add_argument('--port', type=int, default=8787)
    parser.add_argument('--offset', type=int, default=0, help='Media entry offset within a local archive')
    parser.add_argument('--length', type=int, help='Media entry length (default: remainder of file)')
    args = parser.parse_args()
    movie = args.movie.resolve(strict=True)
    length = args.length if args.length is not None else movie.stat().st_size - args.offset
    if args.offset < 0 or length < 1 or args.offset + length > movie.stat().st_size:
        raise SystemExit('Media range is outside the selected local file')
    cli = shutil.which('npx.cmd') or shutil.which('npx')
    if not cli:
        raise SystemExit('npx is required for the Playwright CLI probe')
    publication = ROOT / 'build/web-poc'
    if not publication.is_dir():
        raise SystemExit('Build publication directory does not exist')
    base = f'http://127.0.0.1:{args.port}'
    for name in ['movie-webm.mjs', 'movie-video.mjs']:
        source = ROOT / 'web' / name
        shutil.copy2(source, publication / name)
        with urllib.request.urlopen(f'{base}/{name}', timeout=10) as response:
            if response.read() != source.read_bytes():
                raise SystemExit(f'Server is not serving current {name}')
    directory = ROOT / 'build/movie-video-probe'
    directory.mkdir(parents=True, exist_ok=True)
    helper = directory / 'probe.cjs'
    template = (ROOT / 'web/movie_video_probe.cjs.in').read_text(encoding='utf-8')
    helper.write_text(template.replace('__MOVIE_PATH__', json.dumps(str(movie)))
                      .replace('__PROBE_URL__', json.dumps(base + '/index.html?media-probe=1'))
                      .replace('__MEDIA_OFFSET__', str(args.offset)).replace('__MEDIA_LENGTH__', str(length)),
                      encoding='utf-8')
    result = subprocess.run([cli, '--yes', '--package', '@playwright/cli', 'playwright-cli',
                             f'-s={args.session}', 'run-code', '--filename', str(helper)],
                            cwd=ROOT, capture_output=True, text=True, encoding='utf-8', timeout=90)
    output = result.stdout + result.stderr
    (directory / 'probe.log').write_text(output, encoding='utf-8')
    match = re.search(r'^### Result\s*\n', output, re.M)
    if result.returncode or '### Error' in output or not match:
        raise SystemExit(output[:2000])
    report = json.JSONDecoder().raw_decode(output[match.end():])[0]
    with movie.open('rb') as stream:
        stream.seek(args.offset)
        digest = hashlib.sha256()
        remaining = length
        while remaining:
            chunk = stream.read(min(remaining, 1048576))
            if not chunk:
                raise SystemExit('Local media range was truncated during hashing')
            digest.update(chunk)
            remaining -= len(chunk)
        report['sourceSha256'] = digest.hexdigest()
    report['sourceRange'] = {'offset': args.offset, 'length': length}
    (directory / 'report.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    if len(report['samples']) != 12 or len(report['frames']) != 12:
        raise SystemExit('Expected twelve real decoded video frames; inspect report.json')
    if [s['timestamp'] for s in report['samples']] != [f['timestamp'] for f in report['frames']]:
        raise SystemExit('Decoded timestamps differ from actual container sample timestamps')
    for frame in report['frames']:
        expected = frame['width'] * frame['height'] + 2 * ((frame['width'] + 1) // 2) * ((frame['height'] + 1) // 2)
        if frame['bytes'] != expected:
            raise SystemExit('Decoded NV12 size mismatch')
    print(f'PASS: 12 real browser-decoded frames; report: {directory / "report.json"}')


if __name__ == '__main__':
    main()
