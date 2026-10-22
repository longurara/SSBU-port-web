"""Prepare the real constructor dependency graph from the user's native lift.

Remove only instruction labels proven unreachable by AArch64 control flow.
This avoids pulling nnMain in through the lifter's padding fallthrough edges.
Exception/resume functions and reachable indirect branches stay conservative.
"""
import argparse
import json
from pathlib import Path
import re
import struct

ROOT = Path(__file__).resolve().parents[1]
BASE = 0x7100000000
DEFINITION = re.compile(r'^void ([fu]_[0-9a-f]+)\(Ctx \*c(?:, uint64_t rp)?\) \{(?:[^\n]*\}|\n.*?^\})', re.M | re.S)
LABEL = re.compile(r'^L_([0-9a-f]+):', re.M)
DEPENDENCY = re.compile(r'\b([fu]_[0-9a-f]+)\b')
STATEMENT = re.compile(
    r'uint64_t a = c->x\[(\d+)\] \+ (\d+)ULL;\s*c->x\[(\d+)\] = \(uint64_t\)\(rd64\(a\)\);|'
    r'c->x\[(\d+)\] = ([^;]+);'
)
RD32_JT = re.compile(
    r'uint64_t a = c->x\[(\d+)\] \+ \(\(\(uint64_t\)c->x\[(\d+)\]\)\s*<<\s*2\);\s*'
    r'c->x\[(\d+)\] = \(uint64_t\)\(\(int64_t\)\(int32_t\)rd32\(a\)\);'
)


def load_bucket(generated, bucket, cache, required=True):
    """Range/pointer discovery can cross gaps with no native functions.

    An actual call dependency is still required to have a lifted definition;
    a previous optional lookup must never hide a missing required source.
    """
    if bucket not in cache:
        path=generated/f'c_{bucket:04x}.c'
        cache[bucket]={m.group(1):m.group(0) for m in DEFINITION.finditer(path.read_text(encoding='utf-8'))} if path.is_file() else None
    if cache[bucket] is None:
        if required:raise FileNotFoundError(f'Required native code bucket missing: c_{bucket:04x}.c')
        return {}
    return cache[bucket]


def signed(value, bits):
    return value - (1 << bits) if value >> (bits - 1) else value


def prune_huge_dispatch(body, image):
    if 'L_dispatch:' not in body:
        return body
    d_idx = body.index('L_dispatch:')
    if body[d_idx:].count('case 0x') < 150:
        return body
    addresses = {int(m.group(1), 16) for m in LABEL.finditer(body)}
    lines = body[:d_idx].splitlines()
    tables = []
    for i, line in enumerate(lines):
        if 'goto L_dispatch;' not in line:
            continue
        window = '\n'.join(lines[max(0, i - 20):i + 1])
        matches = list(RD32_JT.finditer(window))
        jm = matches[-1] if matches else None
        if not jm:
            jtm = re.search(r'jt = c->x\[(\d+)\];', line)
            jt_reg = int(jtm.group(1)) if jtm else -1
            last_assign = ''
            for k in range(i - 1, max(-1, i - 25), -1):
                if f'c->x[{jt_reg}] =' in lines[k]:
                    last_assign = lines[k]
                    break
            if 'rd64(' in last_assign or '(int32_t)rd32(' not in window:
                continue
            return body
        base_reg, idx_reg = int(jm.group(1)), int(jm.group(2))
        bound = 1024
        bm = list(re.finditer(rf'adds(?:32|64)\(c, [^,]*c->x\[{idx_reg}\], ~\(uint(?:32|64)_t\)(0x[0-9a-f]+|\d+)ULL, 1\)', window))
        if bm:
            bound = int(bm[-1].group(1), 0) + 2
        base_addr = None
        for k in range(i, -1, -1):
            am = re.search(rf'c->x\[{base_reg}\] = \(uint64_t\)\(c->x\[(\d+)\] \+ (\d+)ULL\);', lines[k])
            if am:
                src_reg, add_off = int(am.group(1)), int(am.group(2))
                for k2 in range(k - 1, max(-1, k - 40), -1):
                    pm = re.search(rf'c->x\[{src_reg}\] = \(uint64_t\)\(0x([0-9a-f]+)ULL\);', lines[k2])
                    if pm:
                        base_addr = int(pm.group(1), 16) + add_off
                        break
                if base_addr is not None:
                    break
        if base_addr is None:
            return body
        tables.append((base_addr, bound))
    all_bases = sorted({b for b, _ in tables})
    tset = set()
    for base_addr, bound in tables:
        pos = all_bases.index(base_addr)
        if pos + 1 < len(all_bases):
            bound = min(bound, (all_bases[pos + 1] - base_addr) // 4)
        for idx in range(bound):
            file_off = 0x1000 + base_addr - BASE + idx * 4
            if file_off < 0 or file_off + 4 > len(image):
                break
            rel = struct.unpack_from('<i', image, file_off)[0]
            target = base_addr + rel
            if target in addresses:
                tset.add(target)
            else:
                break
    cases = ''.join(f'  case 0x{t:x}ULL: goto L_{t:x};\n' for t in sorted(tset))
    return body[:d_idx] + f'L_dispatch: switch (jt) {{\n{cases}  default: call_indirect(c, jt); return;\n}}\n}}'


def reachable_body(name, body, image):
    body = prune_huge_dispatch(body, image)
    labels = list(LABEL.finditer(body))
    if not labels or name.startswith('u_'):
        return body
    addresses = {int(match.group(1), 16): i for i, match in enumerate(labels)}
    last = max(addresses)
    reachable, todo = set(), [int(name[2:], 16)]
    fallthrough = False
    while todo:
        pc = todo.pop()
        if pc in reachable:
            continue
        if pc not in addresses:
            if pc == last + 4:
                fallthrough = True
            continue
        reachable.add(pc)
        word = struct.unpack_from('<I', image, 0x1000 + pc - BASE)[0]
        if word & 0xfffffc1f == 0xd65f0000:  # RET
            continue
        if word & 0xfffffc1f == 0xd61f0000:  # BR: retain all dispatch targets/pads
            return body
        if word & 0xfc000000 == 0x14000000:  # B
            todo.append(pc + signed(word & 0x3ffffff, 26) * 4)
            continue
        if word & 0xff000010 == 0x54000000 or word & 0x7e000000 == 0x34000000:
            todo.append(pc + signed((word >> 5) & 0x7ffff, 19) * 4)
        elif word & 0x7e000000 == 0x36000000:
            todo.append(pc + signed((word >> 5) & 0x3fff, 14) * 4)
        todo.append(pc + 4)
    prefix = body[:labels[0].start()]
    pieces = []
    for i, match in enumerate(labels):
        if int(match.group(1), 16) in reachable:
            end = labels[i + 1].start() if i + 1 < len(labels) else len(body) - 1
            part = body[match.start():end]
            if i + 1 == len(labels) and not fallthrough:
                part = re.sub(r'^  f_[0-9a-f]+\(c\); return;\n?', '', part, flags=re.M)
            pieces.append(part)
    return prefix + ''.join(pieces).rstrip() + '\n}'


def computed_addresses(body, pointer_relocations=None, data_relocations=None):
    """Conservatively collect ADRP+ADD and GOT ADRP+LDR constants.

    This only expands compilation roots; it never rewrites execution or infers
    that a computed address is callable without finding its actual definition.
    """
    values = {}
    for statement in STATEMENT.finditer(body):
        if statement.group(1) is not None:
            base_reg, offset, dst_reg = int(statement.group(1)), int(statement.group(2)), int(statement.group(3))
            if base_reg in values:
                address = values[base_reg] + offset
                if pointer_relocations and address in pointer_relocations:
                    value = pointer_relocations[address]
                    values[dst_reg] = value
                    yield value
                elif data_relocations and address in data_relocations:
                    value = data_relocations[address]
                    values[dst_reg] = value
                    yield value
                else:
                    values.pop(dst_reg, None)
            else:
                values.pop(dst_reg, None)
            continue
        register, expression = int(statement.group(4)), statement.group(5)
        constant = re.fullmatch(r'\(uint64_t\)\(0x([0-9a-f]+)ULL\)', expression)
        addition = re.fullmatch(r'\(uint64_t\)\(c->x\[(\d+)\] \+ (\d+)ULL\)', expression)
        if constant:
            values[register] = int(constant.group(1), 16)
        elif addition and int(addition.group(1)) in values:
            value = values[int(addition.group(1))] + int(addition.group(2))
            values[register] = value
            yield value
        else:
            values.pop(register, None)


def computed_code_pointers(body, pointer_relocations=None, data_relocations=None):
    return (
        value
        for value in computed_addresses(body, pointer_relocations=pointer_relocations, data_relocations=data_relocations)
        if BASE <= value < BASE + 0x39c7e90
    )


def pointer_table_targets(address,relocations,limit=256,data_relocations=None,symbol_relocations=None):
    """Only follow consecutive actual relative relocations into main code.

    Discovery adds compilation roots, never substitutes execution. Null/data
    entries stop the walk; the limit bounds optional discovery, not required
    direct call dependencies.
    """
    if address%8:return []
    result=[]
    for offset in range(limit):
        slot=address+offset*8
        target=relocations.get(slot)
        if target is not None:
            result.append(target)
        elif symbol_relocations and slot in symbol_relocations:
            continue
        else:
            break
    if not result and data_relocations and address in data_relocations:
        return pointer_table_targets(data_relocations[address],relocations,limit=limit,symbol_relocations=symbol_relocations)
    return result


def callback_pointer_tables(callbacks,relocations,limit=256):
    """Find complete contiguous code-pointer groups containing observed targets.

    No recursive table walk: only the supplied, actually observed callbacks
    select groups. Oversized groups are rejected whole, never truncated into
    apparently small tables. Addresses remain original image relocations.
    """
    if limit<1:raise ValueError('Callback table limit must be positive')
    callbacks=set(callbacks);groups={};group=[];previous=None
    def finish():
        if group and len(group)<=limit and callbacks.intersection(target for _,target in group):
            groups[group[0][0]]=[target for _,target in group]
    for address,target in sorted(relocations.items()):
        if address%8:continue
        if previous is None or address!=previous+8:
            finish();group=[]
        group.append((address,target));previous=address
    finish();return groups


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native', type=Path, default=ROOT / 'build/native')
    parser.add_argument('--out', type=Path, default=ROOT / 'build/web-runtime')
    parser.add_argument('--main', action='store_true', help='also include nnMain direct dependency graph')
    parser.add_argument('--pointer-tables',action='store_true',help='discover tables referenced by observed callback bodies or containing observed callback targets')
    parser.add_argument('--all-pointer-tables',action='store_true',help='experimental broad pointer-table discovery across the whole graph')
    parser.add_argument('--callback-tables',action='store_true',help='include bounded relocation tables containing actually observed callback targets')
    args = parser.parse_args()
    generated = args.native / 'work/gen_eh_o2'
    image = (args.native / 'binaries/13.0.4/main.elf').read_bytes()
    init = [struct.unpack_from('<Q', image, 0x1000 + 0x52a7be0 + i * 8)[0] for i in range(306)]
    pointer_targets = set();pointer_relocations={};data_relocations={};symbol_relocations=set()
    # Apply original relative relocation records to the constructor table.
    for offset in range(0x1000 + 0x39c8070, 0x1000 + 0x39c8070 + 0x7ad0d8, 24):
        target, info, addend = struct.unpack_from('<QQQ', image, offset)
        if info & 0xffffffff == 1027 and 0 < addend < 0x39c7e90 and not addend % 4:
            pointer_targets.add(BASE + addend)
            pointer_relocations[BASE+target]=BASE+addend
        elif info & 0xffffffff == 1027 and addend >= 0x39c7e90 and not addend % 8:
            data_relocations[BASE+target]=BASE+addend
        elif info & 0xffffffff == 257 and not target % 8:
            symbol_relocations.add(BASE+target)
        if 0x52a7be0 <= target < 0x52a7be0 + 0x990:
            if info & 0xffffffff != 1027:
                raise ValueError('Non-relative constructor relocation')
            init[(target - 0x52a7be0) // 8] = BASE + addend
    roots = [f'f_{address:x}' for address in init if address] + ['f_71002c55e0']
    discovered = json.loads((ROOT / 'web/runtime_roots.json').read_text())
    observed_callbacks={prefix+address for address in discovered['indirectTargets'] for prefix in ('f_','u_')}
    roots += ['f_' + address for address in discovered['indirectTargets']]
    if args.main:
        roots.append('f_71002c5620')
    cache, selected = {}, {}
    callback_tables=callback_pointer_tables((int(address,16) for address in discovered['indirectTargets']),pointer_relocations) if args.callback_tables or args.pointer_tables or args.all_pointer_tables else {}
    callback_table_roots=set();callback_table_missing=set()
    for address in {target for targets in callback_tables.values() for target in targets}:
        name=f'f_{address:x}'
        if name in load_bucket(generated,(address-BASE)//0xc000,cache,required=False):
            roots.append(name);callback_table_roots.add(address)
        else:callback_table_missing.add(address)
    # Include tiny, self-contained functions actually referenced by image pointer
    # tables. This covers real virtual accessors without one rebuild per setter.
    leaf_roots = []
    for address in sorted(pointer_targets):
        bucket = (address - BASE) // 0xc000
        definitions=load_bucket(generated,bucket,cache,required=False)
        name = f'f_{address:x}'
        body = definitions.get(name)
        if not body or not 1 <= len(LABEL.findall(body)) <= 8:
            continue
        body = reachable_body(name, body, image)
        if set(DEPENDENCY.findall(body)) - {name} or 'host_import(' in body or 'call_indirect(' in body:
            continue
        leaf_roots.append(name)
    roots += leaf_roots
    leaf_roots_set = set(leaf_roots)
    for entry in discovered.get('ranges', []):
        start, end = int(entry['start'], 16), int(entry['end'], 16)
        for bucket in range((start - BASE) // 0xc000, (end - BASE - 1) // 0xc000 + 1):
            definitions=load_bucket(generated,bucket,cache,required=False)
            roots += [name for name in definitions if name.startswith('f_') and start <= int(name[2:], 16) < end]
    todo = list(roots);table_roots=set(callback_table_roots)
    for address in callback_table_roots:
        observed_callbacks.add(f'f_{address:x}')
        observed_callbacks.add(f'u_{address:x}')
    for _ in range(2):
        for name in list(observed_callbacks):
            if not name.startswith('f_'):
                continue
            address = int(name[2:], 16)
            defs = load_bucket(generated, (address - BASE) // 0xc000, cache, required=False)
            body = defs.get(name)
            if body:
                observed_callbacks.update(DEPENDENCY.findall(reachable_body(name, body, image)))
    data_targets = set(data_relocations.values()) | {target + 16 for target in data_relocations.values()} | symbol_relocations
    while todo:
        name = todo.pop()
        if name in selected:
            continue
        address = int(name[2:], 16)
        bucket = (address - BASE) // 0xc000
        definitions=load_bucket(generated,bucket,cache)
        if name not in definitions:
            raise ValueError(f'Function not found: {name} in bucket {bucket:x}')
        body = reachable_body(name, definitions[name], image)
        selected[name] = body
        todo.extend(set(DEPENDENCY.findall(body)) - selected.keys())
        addrs = list(
            computed_addresses(
                body,
                pointer_relocations=pointer_relocations if name not in leaf_roots_set else None,
                data_relocations=data_relocations if name not in leaf_roots_set else None,
            )
        )
        candidates = {value for value in addrs if BASE <= value < BASE + 0x39c7e90}
        if args.all_pointer_tables or (args.pointer_tables and name in observed_callbacks):
            for pointer in addrs:
                targets=pointer_table_targets(pointer,pointer_relocations,data_relocations=data_relocations,symbol_relocations=symbol_relocations)
                candidates.update(targets);table_roots.update(targets)
        elif args.pointer_tables and data_relocations:
            for pointer in addrs:
                if pointer in data_relocations or (name not in leaf_roots_set and (pointer in data_targets or pointer_relocations.get(pointer) in (0x71003cb1c0, 0x71003a47b0, 0x71003cb810) or pointer_relocations.get(pointer + 16) in (0x71032ca8d0, 0x71003cb830) or pointer_relocations.get(pointer + 40) in (0x7103720900, 0x71037208f0, 0x71024a8ab0, 0x7102404b80))):
                    targets=pointer_table_targets(pointer,pointer_relocations,data_relocations=data_relocations,symbol_relocations=symbol_relocations)
                    candidates.update(targets);table_roots.update(targets)
        for pointer in candidates:
            candidate = f'f_{pointer:x}'
            if candidate in selected:
                continue
            candidate_bucket = (pointer - BASE) // 0xc000
            if candidate in load_bucket(generated,candidate_bucket,cache,required=False):
                todo.append(candidate)
    args.out.mkdir(parents=True, exist_ok=True)
    header = ['#include "runtime.h"', 'void nro_catch_call(Ctx *c, void (*body)(Ctx *, uint64_t));']
    for name in sorted(selected):
        header.append(f'void {name}(Ctx *c' + (', uint64_t rp' if name.startswith('u_') else '') + ');')
    (args.out / 'runtime_funcs.h').write_text('\n'.join(header) + '\n', encoding='utf-8')
    names = sorted(selected)
    units = []
    buckets = {}
    for name in names:
        bucket = (int(name[2:], 16) - BASE) // 0xc000
        buckets.setdefault(bucket, []).append(name)
    for bucket, members in sorted(buckets.items()):
        filename = f'runtime_c{bucket:04x}.c'
        body = '\n\n'.join(selected[name] for name in members)
        dependencies = sorted(set(DEPENDENCY.findall(body)))
        declarations = ['#include "runtime.h"', 'void nro_catch_call(Ctx *c, void (*body)(Ctx *, uint64_t));']
        declarations += [f'void {name}(Ctx *c' + (', uint64_t rp' if name.startswith('u_') else '') + ');' for name in dependencies]
        (args.out / filename).write_text('\n'.join(declarations) + '\n' + body + '\n', encoding='utf-8')
        units.append(filename)
    table = ['#include "runtime_funcs.h"', 'const FnEntry fn_table[] = {']
    table += [f'{{0x{name[2:]}ULL, {name}}},' for name in names if name.startswith('f_')]
    table += ['};', 'const int fn_table_count = sizeof(fn_table)/sizeof(fn_table[0]);']
    table += ['const uint64_t constructor_addresses[] = {' + ','.join(f'0x{address:x}ULL' for address in init) + '};',
              'const unsigned constructor_count = sizeof(constructor_addresses)/sizeof(constructor_addresses[0]);']
    (args.out / 'runtime_table.c').write_text('\n'.join(table) + '\n', encoding='utf-8')
    (args.out / 'runtime_imports.c').write_text((generated / 'imports.c').read_text(encoding='utf-8'), encoding='utf-8')
    (args.out / 'runtime_sdkdata.c').write_text((args.native / 'work/sdkdata.c').read_text(encoding='utf-8'), encoding='utf-8')
    manifest = dict(functions=len(selected), constructors=len(init), units=units, withMain=args.main,
                    pointerTableLeaves=len(leaf_roots),referencedPointerTableTargets=len(table_roots),
                    pointerTableDiscovery='all' if args.all_pointer_tables else 'observed' if args.pointer_tables else 'callbacks' if args.callback_tables else 'none',
                    callbackPointerTables=len(callback_tables),callbackPointerTableTargets=len(callback_table_roots),
                    callbackPointerTableMissingDefinitions=[f'{address:x}' for address in sorted(callback_table_missing)],
                    sourceBytes=sum(len(body) for body in selected.values()),
                    imports=sorted({int(i) for body in selected.values() for i in re.findall(r'host_import\(c, (\d+)\)', body)}))
    (args.out / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({key: value for key, value in manifest.items() if key not in ('units', 'imports')}, indent=2))


if __name__ == '__main__':
    main()
