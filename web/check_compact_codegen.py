"""Compare compact code generation with the prepared AArch64 leaf oracle.

Requires the standard local game32 fixture/manifest. Derived function sources,
vectors, Wasm output and reports remain under ignored build/.
"""
import argparse
import json
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--asyncify',action='store_true',help='also verify the optimized Asyncify transformation')
    args=parser.parse_args()
    manifest=json.loads((ROOT/'build/web-poc/manifest.json').read_text(encoding='utf-8'))
    if 'game32' not in manifest.get('commands',{}):raise RuntimeError('Build the standard game32 fixture first')
    arguments=['-O1' if argument=='-O2' else argument for argument in manifest['commands']['game32']]
    flavor='game32-asyncify-opt1' if args.asyncify else 'game32-compact'
    arguments[arguments.index('-o')+1]=str(ROOT/f'build/web-poc/{flavor}.mjs')
    if args.asyncify:
        # Match the full fiber runtime's JS longjmp ABI, rather than the
        # ordinary leaf fixture's Wasm exception mode (incompatible with Asyncify).
        arguments=[item for item in arguments if item not in ('-fwasm-exceptions','-sSUPPORT_LONGJMP=wasm')]
        arguments+=['-sASYNCIFY=1','-sSUPPORT_LONGJMP=emscripten','-sBINARYEN_EXTRA_PASSES=--optimize-level=1']
    subprocess.run([sys.executable,str(ROOT/'deps/emsdk/upstream/emscripten/emcc.py'),*arguments,'-Xclang','-disable-llvm-passes'],check=True)
    subprocess.run([str(ROOT/'deps/emsdk/node/24.19.0_64bit/node.exe'),str(ROOT/'web/test_compact_game.mjs'),*(['--asyncify'] if args.asyncify else [])],check=True,timeout=30)


if __name__=='__main__':main()
