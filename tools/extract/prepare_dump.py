"""Prepare 13.0.4 ExeFS/RomFS from a local base XCI/NSP and update package.

    python prepare_dump.py <base> <update> <output> [--exefs-only]

Uses existing upstream decryption/BKTR code and bounded container readers from
inspect_xci.py. Input files and local keys are never modified or logged.
"""
import argparse
import json
from pathlib import Path
import shutil
import struct
import time

import inspect_xci as inspector
import romfs_extract as rx


def program(report, patch):
    matches = [item for item in report['contents'] if item['contentType'] == 0 and
               any(s['type'] == 0 and (s['crypto'] == 4) == patch for s in item['sections'])]
    if len(matches) != 1: raise ValueError('Expected exactly one matching Program NCA')
    return matches[0]


def select(package, item):
    package.selected = item['name']
    header = rx.xts_decrypt(rx.KEYS['header_key'], package.read(item['name'], 0, 0xc00), 0)
    package.selected_rights = header[0x230:0x240]
    return rx.Nca(package)


def target_for(root, name):
    # Treat archive paths as data, never let them select a path outside output.
    parts = name.replace('\\', '/').lstrip('/').split('/')
    if not all(part and part not in ('.', '..') and ':' not in part for part in parts):
        raise ValueError('Unsafe archive path')
    target = root.joinpath(*parts).resolve()
    if not target.is_relative_to(root.resolve()): raise ValueError('Archive path escapes output')
    return target


def write_entry(root, name, size, read):
    target = target_for(root, name)
    target.parent.mkdir(parents=True, exist_ok=True)
    # Partial files cannot be mistaken for completed output after interruption.
    partial = target.with_name(target.name + '.partial')
    with partial.open('wb') as out:
        for offset in range(0, size, 16 << 20):
            count = min(16 << 20, size - offset)
            data = read(offset, count)
            if len(data) != count: raise ValueError('Short decrypted read')
            out.write(data)
    partial.replace(target)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('base'); parser.add_argument('update'); parser.add_argument('output', type=Path)
    parser.add_argument('--exefs-only', action='store_true')
    args = parser.parse_args()
    out = args.output.resolve()
    if out.exists() and any(out.iterdir()): parser.error('Output folder must be new or empty')
    base_info, update_info = inspector.inspect(args.base), inspector.inspect(args.update)
    if not any(i.get('displayVersion') == '13.0.4' for i in update_info['contents']):
        parser.error('Update control metadata must say 13.0.4')
    bitem, uitem = program(base_info, False), program(update_info, True)
    if bitem['titleId'] != uitem['titleId']: parser.error('Base/update title mismatch')
    for name in ('main', 'sdk'):
        if not uitem.get('executables', {}).get(name, {}).get('matchesRequiredTextSize'):
            parser.error(f'Update {name} does not match upstream 13.0.4')
    base, update = inspector.open_package(args.base), inspector.open_package(args.update)
    try:
        bnca, unca = select(base, bitem), select(update, uitem)
        sec = unca.section(1)
        entries = inspector.pfs_files(unca, sec)
        for name in ('main', 'sdk', 'main.npdm'):
            if name not in entries: raise ValueError('Required ExeFS file missing: ' + name)
        romfs = rx.PatchedRomfs(bnca, unca)
        files = [] if args.exefs_only else sorted(rx.walk(romfs, romfs.level6))
        required = sum(s for _, s in entries.values()) + sum(s for _, _, s in files)
        out.parent.mkdir(parents=True, exist_ok=True)
        if shutil.disk_usage(out.parent).free < required + (2 << 30):
            raise ValueError('Insufficient disk space for dump plus 2 GiB margin')
        out.mkdir(exist_ok=True)
        for name, (offset, size) in entries.items():
            write_entry(out / 'exefs', name, size, lambda off, count, start=offset: unca.read(sec, start + off, count))
            print(f'ExeFS: {name} ({size} bytes)', flush=True)
        print(f'RomFS: {len(files)} files, {sum(s for _, _, s in files) / (1 << 30):.2f} GiB', flush=True)
        started = time.monotonic()
        for i, (name, offset, size) in enumerate(files):
            write_entry(out / 'romfs', name, size, lambda off, count, start=offset: romfs.read(start + off, count))
            print(f'RomFS {i + 1}/{len(files)}: {name} ({size} bytes), elapsed {time.monotonic() - started:.1f}s', flush=True)
        (out / 'preparation.json').write_text(json.dumps({
            'version': '13.0.4', 'base': str(Path(args.base).resolve()), 'update': str(Path(args.update).resolve()),
            'exefsOnly': args.exefs_only, 'romfsFiles': len(files),
            'status': 'extracted; run nso2elf to verify executable segment hashes',
        }, indent=2) + '\n')
        print(f'Prepared: {out}', flush=True)
    finally:
        base.f.close(); update.f.close()


if __name__ == '__main__': main()
