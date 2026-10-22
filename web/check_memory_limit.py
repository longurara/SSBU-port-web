"""Verify the actual wasm32 shared-memory limit before a guarded runtime boot."""
import argparse
import hashlib
import json
from pathlib import Path
from wasm_stats import leb


def memory_limits(data):
    if data[:8] != b'\0asm\1\0\0\0':
        raise ValueError('Not a Wasm module')
    def name(at):
        size, at = leb(data, at)
        return at + size
    def value_type(at):
        kind = data[at]
        at += 1
        if kind in (0x63, 0x64):
            _, at = leb(data, at)
        return at
    def limits(at):
        flags, at = leb(data, at)
        if flags & ~3:
            raise ValueError('Expected bounded wasm32 limits')
        minimum, at = leb(data, at)
        maximum = None
        if flags & 1:
            maximum, at = leb(data, at)
        return {'minimumPages': minimum, 'maximumPages': maximum, 'shared': bool(flags & 2)}, at
    at, result = 8, []
    while at < len(data):
        section = data[at]
        size, at = leb(data, at + 1)
        end = at + size
        if section == 2:
            count, at = leb(data, at)
            for _ in range(count):
                at = name(name(at))
                kind = data[at]
                at += 1
                if kind == 0:
                    _, at = leb(data, at)
                elif kind == 1:
                    _, at = limits(value_type(at))
                elif kind == 2:
                    memory, at = limits(at)
                    result.append(memory)
                elif kind == 3:
                    at = value_type(at) + 1
                elif kind == 4:
                    _, at = leb(data, at + 1)
                else:
                    raise ValueError('Unsupported Wasm import kind')
        elif section == 5:
            count, at = leb(data, at)
            for _ in range(count):
                memory, at = limits(at)
                result.append(memory)
        at = end
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('path', type=Path)
    parser.add_argument('--pages', type=int, default=65536)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    data = args.path.read_bytes()
    memories = memory_limits(data)
    if len(memories) != 1 or not memories[0]['shared'] or memories[0]['maximumPages'] != args.pages:
        raise SystemExit(f'Wrong actual Wasm shared-memory limit: {memories}')
    result = {'ok': True, 'wasmSha256': hashlib.sha256(data).hexdigest(), 'memories': memories}
    if args.report:
        args.report.write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps(result))
