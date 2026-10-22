"""Ensure the browser harness cannot expose source/dump directories."""
import http.client
from http.server import ThreadingHTTPServer
import json
from pathlib import Path
import tempfile
import threading
import unittest
from serve import handler_for


class ServerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.base = Path(cls.temp.name)
        cls.build = cls.base / 'public'
        cls.build.mkdir()
        (cls.build / 'index.html').write_text('<h1>probe</h1>')
        (cls.build / 'core.wasm').write_bytes(b'\0asm')
        (cls.base / 'dump.arc').write_text('private fixture')
        cls.server = ThreadingHTTPServer(('127.0.0.1', 0), handler_for(cls.build))
        cls.thread = threading.Thread(target=cls.server.serve_forever, daemon=True)
        cls.thread.start()

    @classmethod
    def tearDownClass(cls):
        cls.server.shutdown(); cls.server.server_close(); cls.thread.join()
        cls.temp.cleanup()

    def request(self, method, path, body=None, headers=None):
        connection = http.client.HTTPConnection('127.0.0.1', self.server.server_port, timeout=5)
        try:
            connection.request(method, path, body=body, headers=headers or {})
            response = connection.getresponse()
            return response.status, dict(response.getheaders()), response.read()
        finally: connection.close()

    def test_isolation_and_wasm_mime(self):
        status, headers, data = self.request('GET', '/core.wasm')
        self.assertEqual(status, 200)
        self.assertEqual(headers['Content-Type'], 'application/wasm')
        self.assertEqual(headers['Cross-Origin-Opener-Policy'], 'same-origin')
        self.assertEqual(headers['Cross-Origin-Embedder-Policy'], 'require-corp')
        self.assertEqual(data, b'\0asm')

    def test_head(self):
        status, headers, data = self.request('HEAD', '/')
        self.assertEqual(status, 200)
        self.assertGreater(int(headers['Content-Length']), 0)
        self.assertEqual(data, b'')

    def test_traversal_and_dump_not_served(self):
        for path in ['/../dump.arc', '/%2e%2e/dump.arc', '/..%5cdump.arc', '/dump.arc', '/C:/dump.arc']:
            with self.subTest(path=path): self.assertEqual(self.request('GET', path)[0], 403)
        self.assertEqual(self.request('GET', '/missing.mjs')[0], 404)
        self.assertEqual(self.request('GET', '/nested/')[0], 403)

    def test_report_and_origin(self):
        payload = json.dumps({'results': {}, 'scope': 'synthetic'})
        self.assertEqual(self.request('POST', '/__report', payload, {'Content-Type': 'application/json'})[0], 200)
        self.assertEqual(json.loads((self.build / 'browser-report.json').read_text())['scope'], 'synthetic')
        self.assertEqual(self.request('POST', '/__report', payload, {'Origin': 'https://example.com'})[0], 403)
        self.assertEqual(self.request('POST', '/__report', '[]')[0], 400)
        self.assertEqual(self.request('POST', '/__report', '{')[0], 400)
        self.assertEqual(self.request('POST', '/elsewhere', payload)[0], 404)

    def test_separate_asset_and_loader_reports(self):
        for endpoint, filename in (('/__asset_report', 'asset-browser-report.json'), ('/__elf_report', 'elf-browser-report.json'), ('/__runtime_report', 'runtime-browser-report.json')):
            payload = json.dumps({'results': {'localFile': {'ok': True}}})
            self.assertEqual(self.request('POST', endpoint, payload)[0], 200)
            self.assertTrue(json.loads((self.build / filename).read_text())['results']['localFile']['ok'])
            self.assertEqual(self.request('POST', endpoint, payload, {'Origin': 'https://example.com'})[0], 403)


if __name__ == '__main__': unittest.main()
