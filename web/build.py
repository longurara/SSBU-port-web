"""Build browser runtime checks and optional locally prepared game integration."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
WEB = ROOT / 'web'
OUT = ROOT / 'build' / 'web-poc'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--target', choices=['all', 'core32', 'core64', 'fiber32', 'game32', 'game64'], default='all')
    parser.add_argument('--with-game', action='store_true', help='include real game leaf functions prepared by web/prepare_game.py')
    parser.add_argument('--test', action='store_true', help='run core32/core64 under SDK Node after build')
    args = parser.parse_args()
    lock = json.loads((WEB / 'toolchain.json').read_text())
    sdk = Path(os.environ.get('SSBU_EMSDK', ROOT / 'deps' / 'emsdk')).resolve()
    emcc = sdk / 'upstream' / 'emscripten' / 'emcc.py'
    if not emcc.is_file():
        parser.error(f'Emscripten missing. See web/README.md. Expected: {emcc}')
    version = subprocess.check_output([sys.executable, str(emcc), '--version'], text=True)
    first_line = version.splitlines()[0]
    if f' {lock["emscripten"]} ' not in first_line:
        parser.error(f'Expected pinned Emscripten {lock["emscripten"]}; got {first_line}')
    OUT.mkdir(parents=True, exist_ok=True)
    subprocess.run([sys.executable, str(emcc), '-std=c11', '-Wall', '-Wextra', '-Werror',
                    '-I', str(ROOT / 'runtime'), '-fsyntax-only', str(WEB / 'native_header_check.c')], check=True)
    targets = ['core32', 'core64', 'fiber32'] if args.target == 'all' else [args.target]
    if args.with_game and args.target == 'all': targets += ['game32', 'game64']
    commands = {}
    for target in targets:
        common = ['-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off',
                  '-fno-strict-aliasing', '-I', str(WEB), '-I', str(ROOT / 'runtime'),
                  '-sMODULARIZE=1', '-sEXPORT_ES6=1', '-sENVIRONMENT=web,worker,node',
                  '-sASSERTIONS=1', '-sEXIT_RUNTIME=1', '-sINITIAL_MEMORY=67108864',
                  '-sSTACK_SIZE=2097152']
        if target.startswith(('core', 'game')):
            if target.startswith('game'):
                generated = ROOT / 'build' / 'web-game'
                if not (generated / 'game_vectors.h').is_file():
                    parser.error('Run web/prepare_game.py with the dump-tools Python first')
                sources = [str(WEB / 'game_probe.c'), str(generated / 'game_functions.c'), '-I', str(generated), '-Wno-unused-label']
            else:
                sources = [str(WEB / 'probe.c')]
            specific = [*sources, str(WEB / 'guest_memory.c'), str(WEB / 'runtime_memory.c'),
                        '-DRECOMP_WEB_MEMORY', '-pthread',
                        '-sPTHREAD_POOL_SIZE=2', '-sPROXY_TO_PTHREAD=1',
                        '-sMAXIMUM_MEMORY=268435456',
                        '-fwasm-exceptions', '-sSUPPORT_LONGJMP=wasm']
            if target.endswith('64'): specific += ['-m64']
        else:
            specific = [str(WEB / 'fiber_probe.c'), '-sASYNCIFY=1', '-sSUPPORT_LONGJMP=emscripten']
        command = [sys.executable, str(emcc), *common, *specific, '-o', str(OUT / f'{target}.mjs')]
        print(f'Building {target}', flush=True)
        subprocess.run(command, check=True, cwd=ROOT)
        commands[target] = command[2:]
    for name in ['index.html', 'app.mjs', 'node-test.mjs', 'test-elf.mjs']:
        shutil.copy2(WEB / name, OUT / name)
    game = None
    if any(t.startswith('game') for t in targets):
        game = json.loads((ROOT / 'build/web-game/selection.json').read_text())
        command = [sys.executable, str(emcc), '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
                   str(WEB / 'asset_io.c'), str(WEB / 'elf_loader.c'), str(WEB / 'guest_memory.c'),
                   str(WEB / 'runtime_memory.c'), str(WEB / 'startup.c'), str(ROOT / 'build/web-game/game_startup.c'),
                   '-I', str(WEB), '-I', str(ROOT / 'runtime'), '-DRECOMP_WEB_MEMORY', '-Wno-unused-label',
                   '--no-entry', '-lworkerfs.js',
                   '-sMODULARIZE=1', '-sEXPORT_ES6=1', '-sENVIRONMENT=worker,node',
                   '-sINITIAL_MEMORY=16777216', '-sALLOW_MEMORY_GROWTH=1', '-sMAXIMUM_MEMORY=536870912',
                   '-sEXPORTED_FUNCTIONS=["_asset_size","_asset_hash","_asset_status","_elf_load","_elf_info","_elf_hash","_elf_unload","_elf_relocate","_elf_relocation_info","_elf_word","_startup_run","_startup_info"]',
                   '-sEXPORTED_RUNTIME_METHODS=["FS","WORKERFS","ccall","HEAPU8"]',
                   '-sASSERTIONS=1', '-o', str(OUT / 'assets.mjs')]
        subprocess.run(command, check=True, cwd=ROOT)
        shutil.copy2(WEB / 'assets-worker.mjs', OUT / 'assets-worker.mjs')
        shutil.copy2(ROOT / 'build/web-game/asset-manifest.json', OUT / 'asset-manifest.json')
        shutil.copy2(ROOT / 'build/web-game/elf-manifest.json', OUT / 'elf-manifest.json')
    manifest = {'toolchain': first_line, 'upstream': lock['upstream'],
                'targetsBuiltThisInvocation': targets, 'commands': commands,
                'game': game,
                'scope': 'Runtime probes plus local game leaf functions; no game boot.' if game else 'Synthetic probes only. Game dump not used.'}
    (OUT / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(f'Output: {OUT}', flush=True)
    if args.test:
        candidates = sorted((sdk / 'node').glob('*/node.exe')) + sorted((sdk / 'node').glob('*/bin/node'))
        node = str(candidates[-1]) if candidates else shutil.which('node')
        if not node: parser.error('Node not found')
        subprocess.run([node, str(OUT / 'node-test.mjs')], check=True, cwd=OUT, timeout=90)
        if game:
            subprocess.run([node, str(OUT / 'test-elf.mjs'), str(ROOT / 'build/native/binaries/13.0.4/main.elf')], check=True, cwd=OUT, timeout=90)


if __name__ == '__main__':
    main()
