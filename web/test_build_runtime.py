"""A rejected large-local link must not replace the previously served module."""
import hashlib
import json
from pathlib import Path
import tempfile
import unittest
from build_runtime import publish_module


def leb(value):
    result=bytearray()
    while True:
        byte=value&127;value>>=7;result.append(byte|(128 if value else 0))
        if not value:return result


def wasm(locals_count):
    # One () -> () function, one i32 local group, and an empty body.
    body=bytes([1])+leb(locals_count)+bytes([0x7f,0x0b])
    code=bytes([1])+leb(len(body))+body
    return b'\0asm\1\0\0\0'+bytes([1,4,1,0x60,0,0,3,2,1,0,10])+leb(len(code))+code


class PublicationTests(unittest.TestCase):
    def test_rejected_link_preserves_previous_pair_and_report(self):
        with tempfile.TemporaryDirectory()as folder:
            root=Path(folder);stage=root/'staged/runtime.mjs';stage.parent.mkdir();target=root/'served/runtime.mjs'
            stage.write_text('/* fixture A */');stage.with_suffix('.wasm').write_bytes(wasm(12))
            report=publish_module(stage,target);self.assertEqual(report['maximumLocals'],12)
            metadata=target.with_name('runtime-build.json');before=[p.read_bytes()for p in (target,target.with_suffix('.wasm'),metadata)]
            stage.write_text('/* rejected fixture B */');stage.with_suffix('.wasm').write_bytes(wasm(50001))
            with self.assertRaisesRegex(RuntimeError,'numeric-local'):publish_module(stage,target)
            self.assertEqual(before,[p.read_bytes()for p in (target,target.with_suffix('.wasm'),metadata)])
            stage.with_suffix('.wasm').write_bytes(wasm(50000));publish_module(stage,target)
            self.assertEqual(target.read_text(),'/* rejected fixture B */')
            report=json.loads(metadata.read_text());self.assertEqual(report['maximumLocals'],50000)
            self.assertEqual(report['wasmSha256'],hashlib.sha256(target.with_suffix('.wasm').read_bytes()).hexdigest())
            self.assertEqual(list(target.parent.glob('*.partial')),[])

    def test_missing_pair_does_not_create_public_output(self):
        with tempfile.TemporaryDirectory()as folder:
            root=Path(folder);stage=root/'staged/runtime.mjs';stage.parent.mkdir();stage.write_text('/* incomplete */')
            target=root/'served/runtime.mjs'
            with self.assertRaises(FileNotFoundError):publish_module(stage,target)
            self.assertFalse(target.parent.exists())
            with self.assertRaises(ValueError):publish_module(stage,root/'served/different.mjs')
            with self.assertRaises(ValueError):publish_module(stage,stage)


if __name__=='__main__':unittest.main()
