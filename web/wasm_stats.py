"""Inspect numeric local counts in generated Wasm modules/objects."""
import json
from pathlib import Path
import sys


def leb(data, cursor):
    value = shift = 0
    while True:
        byte = data[cursor]; cursor += 1
        value |= (byte & 127) << shift
        if not byte & 128:
            return value, cursor
        shift += 7
        if shift > 63:
            raise ValueError('Invalid LEB128')


def local_counts(path):
    data = Path(path).read_bytes()
    if data[:8] != b'\0asm\1\0\0\0':
        raise ValueError('Not a Wasm binary')
    cursor, counts = 8, []
    while cursor < len(data):
        kind = data[cursor]; cursor += 1
        size, cursor = leb(data, cursor)
        end = cursor + size
        if kind == 10:
            functions, cursor = leb(data, cursor)
            for _ in range(functions):
                body_size, cursor = leb(data, cursor)
                body_end = cursor + body_size
                groups, cursor = leb(data, cursor)
                total = 0
                for _ in range(groups):
                    number, cursor = leb(data, cursor)
                    total += number
                    value_type = data[cursor]; cursor += 1
                    if value_type in (0x63, 0x64):
                        _, cursor = leb(data, cursor)
                counts.append(total)
                cursor = body_end
        cursor = end
    return counts


if __name__ == '__main__':
    for path in sys.argv[1:]:
        counts = local_counts(path)
        print(json.dumps(dict(path=path, functions=len(counts), maximumLocals=max(counts, default=0), overLimit=sum(n > 50000 for n in counts))))
