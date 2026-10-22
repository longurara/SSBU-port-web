"""The static fiber protocol uses private Asyncify internals: fail on drift."""
import hashlib
from pathlib import Path

LIBASYNC_SHA256='a2c2f495ab5f850d61e045b5c4dd4db66a0b4390b9b3e98d33059c38d6f00329'


def verify(root: Path):
    source=root/'deps/emsdk/upstream/emscripten/src/lib/libasync.js'
    if hashlib.sha256(source.read_bytes()).hexdigest()!=LIBASYNC_SHA256:
        raise RuntimeError('Fiber adapter requires the verified Emscripten 6.0.12 libasync.js; revalidate the protocol before using another version')
