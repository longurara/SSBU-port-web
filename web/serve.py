"""Serve only the web build directory on loopback, with COOP/COEP."""
import argparse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
import mimetypes
from pathlib import Path
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / 'build' / 'web-poc'
MAX_REPORT = 1024 * 1024
ALLOWED = {'.html', '.mjs', '.js', '.wasm', '.json', '.css'}


def handler_for(build):
    build = Path(build).resolve()

    class Handler(BaseHTTPRequestHandler):
        def end_headers(self):
            self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
            self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
            self.send_header('Cross-Origin-Resource-Policy', 'same-origin')
            self.send_header('Cache-Control', 'no-store')
            self.send_header('X-Content-Type-Options', 'nosniff')
            super().end_headers()

        def do_GET(self):
            self.serve_file(False)

        def do_HEAD(self):
            self.serve_file(True)

        def serve_file(self, head):
            try:
                name = unquote(urlsplit(self.path).path)
                if '\\' in name or '\x00' in name or ':' in name:
                    self.send_error(403); return
                path = (build / (name.lstrip('/') or 'index.html')).resolve()
                if not path.is_relative_to(build) or path.suffix not in ALLOWED:
                    self.send_error(403); return
                if not path.is_file():
                    self.send_error(404); return
                data = path.read_bytes()
            except (OSError, ValueError):
                self.send_error(400); return
            mime = {'.mjs': 'text/javascript', '.wasm': 'application/wasm'}.get(path.suffix)
            mime = mime or mimetypes.guess_type(path.name)[0] or 'application/octet-stream'
            self.send_response(200)
            self.send_header('Content-Type', mime)
            self.send_header('Content-Length', str(len(data)))
            self.end_headers()
            if not head: self.wfile.write(data)

        def do_POST(self):
            reports = {'/__report': 'browser-report.json', '/__asset_report': 'asset-browser-report.json', '/__elf_report': 'elf-browser-report.json', '/__runtime_report': 'runtime-browser-report.json'}
            if self.path not in reports:
                self.send_error(404); return
            # Browsers may submit reports only from this exact loopback origin.
            origin = self.headers.get('Origin')
            expected = f'http://127.0.0.1:{self.server.server_port}'
            if origin not in (None, expected):
                self.send_error(403); return
            try:
                size = int(self.headers.get('Content-Length', '0'))
                if not 0 < size <= MAX_REPORT:
                    self.send_error(413); return
                report = json.loads(self.rfile.read(size))
                if not isinstance(report, dict) or not isinstance(report.get('results'), dict):
                    self.send_error(400); return
                build.mkdir(parents=True, exist_ok=True)
                output = build / reports[self.path]
                if output.is_symlink():
                    self.send_error(403); return
                output.write_text(json.dumps(report, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
            except (OSError, ValueError, json.JSONDecodeError):
                self.send_error(400); return
            data = b'{"saved":true}'
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.send_header('Content-Length', str(len(data)))
            self.end_headers()
            self.wfile.write(data)

    return Handler


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', type=int, default=8787)
    args = parser.parse_args()
    if not (BUILD / 'index.html').is_file(): parser.error('Run python web/build.py first')
    server = ThreadingHTTPServer(('127.0.0.1', args.port), handler_for(BUILD))
    print(f'Web port integration: http://127.0.0.1:{server.server_port}', flush=True)
    print(f'Serving only: {BUILD}', flush=True)
    try: server.serve_forever()
    except KeyboardInterrupt: pass
    finally: server.server_close()


if __name__ == '__main__':
    main()
