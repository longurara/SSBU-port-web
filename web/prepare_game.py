"""Select bounded leaf functions from the local native lift; build Unicorn oracles.

All extracted code and register/memory vectors stay under ignored build/. Nothing
in this script supplies game bytes. Requires the user's completed native build.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import re
import struct

from unicorn import Uc, UcError, UC_ARCH_ARM64, UC_MODE_ARM
from unicorn import arm64_const as arm

ROOT = Path(__file__).resolve().parents[1]
BASE = 0x7100000000
DATA = 0x7200300000
SIZE = 0x10000
RETURN = 0x7300000000
FUNCTION = re.compile(r'^void f_([0-9a-f]+)\(Ctx \*c\) \{\n.*?^\}', re.M | re.S)
LABEL = re.compile(r'^L_([0-9a-f]+):', re.M)
FORBIDDEN = re.compile(r'\b(?:f_[0-9a-f]+|call_indirect|host_import|guest_unimpl|guest_trap|guest_svc|eh_\w+|setjmp|longjmp)\s*\(|uintptr_t|__asm|__builtin_trap')
XREGS = [getattr(arm, f'UC_ARM64_REG_X{i}') for i in range(31)] + [arm.UC_ARM64_REG_SP]
VREGS = [getattr(arm, f'UC_ARM64_REG_Q{i}') for i in range(32)]


def pattern(seed):
    """Simple deterministic pattern also implemented by game_probe.c."""
    return bytes(((i * 17 + (i >> 8) * 13 + seed * 29) & 255) for i in range(SIZE))


def reference(address, code, seed, memory):
    rng = random.Random(address ^ seed)
    x = [DATA + 0x100 + i * 0x100 if memory else rng.getrandbits(64) for i in range(30)]
    x += [RETURN, DATA + SIZE - 0x100]
    vectors = [int.from_bytes(struct.pack('<4f', *(rng.randint(-10000, 10000) / 64 for _ in range(4))), 'little') for _ in range(32)]
    flags = rng.randrange(16)
    initial = pattern(seed)
    uc = Uc(UC_ARCH_ARM64, UC_MODE_ARM)
    lo = address & ~4095
    uc.mem_map(lo, ((address + len(code) + 4095) & ~4095) - lo)
    uc.mem_write(address, code)
    uc.mem_map(DATA, SIZE)
    uc.mem_write(DATA, initial)
    uc.mem_map(RETURN, 4096)
    for reg, value in zip(XREGS, x):
        uc.reg_write(reg, value)
    for reg, value in zip(VREGS, vectors):
        uc.reg_write(reg, value)
    uc.reg_write(arm.UC_ARM64_REG_NZCV, flags << 28)
    try:
        uc.emu_start(address, RETURN, count=2048)
    except UcError:
        return None
    if uc.reg_read(arm.UC_ARM64_REG_PC) != RETURN:
        return None
    after = bytes(uc.mem_read(DATA, SIZE))
    return dict(seed=seed, x=x, vectors=vectors, flags=flags,
                out_x=[uc.reg_read(reg) for reg in XREGS],
                out_vectors=[uc.reg_read(reg) for reg in VREGS],
                out_flags=uc.reg_read(arm.UC_ARM64_REG_NZCV) >> 28,
                changes=[(i, b) for i, b in enumerate(after) if b != initial[i]])


def words(values):
    return ','.join(f'0x{value:x}ULL' for value in values)


def vwords(values):
    return words([word for value in values for word in (value & ((1 << 64) - 1), value >> 64)])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native', type=Path, default=ROOT / 'build/native')
    parser.add_argument('--functions', type=int, default=96)
    parser.add_argument('--out', type=Path, default=ROOT / 'build/web-game')
    args = parser.parse_args()
    if not 1 <= args.functions <= 512:
        parser.error('--functions must be 1..512')
    native = args.native.resolve()
    elf = native / 'binaries/13.0.4/main.elf'
    # The lift's layout and addresses are specific to the validated 13.0.4 ELF.
    image = elf.read_bytes()
    if image[:4] != b'\x7fELF' or len(image) < 0x39c7e90 + 0x1000:
        parser.error('Expected converted 13.0.4 main ELF')
    files = sorted((native / 'work/gen_eh_o2').glob('c_*.c'))
    if not files:
        parser.error('Native generated C files are missing; run native setup first')
    # Spread the selection across the image instead of taking adjacent getters.
    files = files[::max(1, len(files) // args.functions)]
    selected, tests = [], []
    for path in files:
        text = path.read_text(encoding='utf-8')
        per_file = 0
        for match in FUNCTION.finditer(text):
            body = match.group(0)
            labels = [int(s, 16) for s in LABEL.findall(body)]
            if not 4 <= len(labels) <= 96 or FORBIDDEN.search(body.split('\n', 1)[1]):
                continue
            address = int(match.group(1), 16)
            if labels != list(range(address, address + 4 * len(labels), 4)):
                continue
            memory = bool(re.search(r'\b(?:rd|wr)(?:8|16|32|64|v)\(', body))
            code = image[0x1000 + address - BASE:0x1000 + labels[-1] - BASE + 4]
            cases = [reference(address, code, seed, memory) for seed in (1, 7, 29, 101)]
            if any(case is None for case in cases):
                continue
            fn_index = len(selected)
            selected.append(dict(address=f'{address:x}', source=path.name,
                                 instructions=len(labels), memory=memory,
                                 sha256=hashlib.sha256(code).hexdigest(), body=body))
            tests.extend(dict(case, function=fn_index) for case in cases)
            per_file += 1
            if len(selected) >= args.functions or per_file >= 4:
                break
        if len(selected) >= args.functions:
            break
    if len(selected) < args.functions:
        parser.error(f'Only found {len(selected)} bounded functions; requested {args.functions}')
    args.out.mkdir(parents=True, exist_ok=True)
    (args.out / 'game_functions.c').write_text('#include "runtime.h"\n' + '\n\n'.join(fn['body'] for fn in selected) + '\n', encoding='utf-8')
    header = ['/* Locally generated from the user dump. Do not distribute. */']
    for fn in selected:
        header.append(f'void f_{fn["address"]}(Ctx *c);')
    header.append('static void (*const game_functions[])(Ctx *) = {' + ','.join('f_' + fn['address'] for fn in selected) + '};')
    header.append('static const uint64_t game_addresses[] = {' + ','.join('0x' + fn['address'] + 'ULL' for fn in selected) + '};')
    for i, case in enumerate(tests):
        changes = case['changes'] or [(0, 0)]
        header.append(f'static const MemoryChange changes_{i}[] = {{' + ','.join(f'{{{offset},{value}}}' for offset, value in changes) + '};')
    header.append('static const GameCase game_cases[] = {')
    for i, case in enumerate(tests):
        header.append('{%d,%d,%d,%d,{%s},{%s},{%s},{%s},changes_%d,%d},' % (
            case['function'], case['seed'], case['flags'], case['out_flags'], words(case['x']),
            vwords(case['vectors']), words(case['out_x']), vwords(case['out_vectors']), i, len(case['changes'])))
    header.append('};')
    (args.out / 'game_vectors.h').write_text('\n'.join(header) + '\n', encoding='utf-8')
    report = dict(version='13.0.4', elfSHA256=hashlib.sha256(image).hexdigest(),
                  functions=[{k: v for k, v in fn.items() if k != 'body'} for fn in selected],
                  cases=len(tests), oracle='Unicorn AArch64, default FPCR, four deterministic states/function',
                  limitations='Bounded leaf functions only; no boot, imports, modules, scheduler or NVN renderer.')
    (args.out / 'selection.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    state = json.loads((native / 'state.json').read_text(encoding='utf-8'))
    archive = Path(state['dump']['romfs']) / 'data.arc'
    size = archive.stat().st_size
    samples = []
    for offset in (0, (1 << 31) - 32768, (1 << 32) - 32768, 1 << 33, size // 2, size - 65536):
        if offset < 0 or offset + 65536 > size:
            continue
        with archive.open('rb') as stream:
            stream.seek(offset)
            data = stream.read(65536)
        if len(data) != 65536:
            raise ValueError('Short read while preparing archive reference')
        value = 2166136261
        for b in data:
            value = ((value ^ b) * 16777619) & 0xffffffff
        samples.append(dict(offset=str(offset), length=len(data), fnv32=value))
    (args.out / 'asset-manifest.json').write_text(json.dumps(dict(size=size, samples=samples,
        scope='Six sampled file ranges only; no full archive integrity claim.'), indent=2) + '\n', encoding='utf-8')
    elf_samples = []
    segments = 0
    phoff = struct.unpack_from('<Q', image, 32)[0]
    phnum = struct.unpack_from('<H', image, 56)[0]
    for i in range(phnum):
        kind, _, offset, address, _, filesz, memsz, _ = struct.unpack_from('<IIQQQQQQ', image, phoff + i * 56)
        if kind != 1 or not memsz:
            continue
        segments += 1
        for delta in (0, filesz - 64):
            if delta < 0:
                continue
            value = 2166136261
            for b in image[offset + delta:offset + delta + 64]:
                value = ((value ^ b) * 16777619) & 0xffffffff
            elf_samples.append(dict(offset=str(address + delta), length=64, fnv32=value))
    tags = {}
    for i in range(phnum):
        kind, _, offset, _, _, filesz, _, _ = struct.unpack_from('<IIQQQQQQ', image, phoff + i * 56)
        if kind == 2:
            for j in range(offset, offset + filesz, 16):
                tag, value = struct.unpack_from('<QQ', image, j)
                if not tag:
                    break
                tags[tag] = value
    counts = dict(relative=0, own=0, imports=0, weak=0)
    relocation_samples = {name: [] for name in counts}
    for table, size in ((tags[7], tags[8]), (tags[23], tags[2])):
        for j in range(0x1000 + table, 0x1000 + table + size, 24):
            target, encoded, addend = struct.unpack_from('<QQQ', image, j)
            kind, symbol = encoded & 0xffffffff, encoded >> 32
            if kind == 1027:
                value, category = (BASE + addend) & ((1 << 64) - 1), 'relative'
            elif kind in (257, 1025, 1026):
                _, symbol_info, _, defined, symbol_value, _ = struct.unpack_from('<IBBHQQ', image, 0x1000 + tags[6] + symbol * 24)
                if defined:
                    value, category = BASE + symbol_value, 'own'
                elif symbol_info >> 4 == 2:
                    value, category = 0, 'weak'
                else:
                    value, category = 0x7400000000 + symbol * 256, 'imports'
                if kind == 257:
                    value = (value + addend) & ((1 << 64) - 1)
            else:
                raise ValueError(f'Unsupported relocation type: {kind}')
            counts[category] += 1
            samples = relocation_samples[category]
            if len(samples) == 4:
                samples.pop(2)
            samples.append(dict(offset=str(target), expected=str(value)))
    relocations = dict(counts=counts, samples=[sample for values in relocation_samples.values() for sample in values])
    (args.out / 'elf-manifest.json').write_text(json.dumps(dict(size=len(image), segments=segments,
        samples=elf_samples, relocations=relocations,
        scope='ELF mapping, relocations to symbolic import slots, and nninitStartup; no constructors, nnMain, renderer or gameplay.'), indent=2) + '\n', encoding='utf-8')
    # Bootstrap slice: the actual nninitStartup body plus its three PLT imports.
    # The native lifter includes unreachable padding after RET and declares an
    # nnMain fallthrough there. A fail-fast boundary satisfies that dead edge;
    # it must never execute, and does not pretend that nnMain is implemented.
    startup_file = native / 'work/gen_eh_o2/c_003b.c'
    startup = next(match.group(0) for match in FUNCTION.finditer(startup_file.read_text(encoding='utf-8')) if match.group(1) == '71002c55e0')
    calls = sorted(set(re.findall(r'\bf_([0-9a-f]+)\(c\)', startup)))
    if calls != ['71002c5620', '71039c1460', '71039c1470', '71039c1480']:
        raise ValueError(f'Unexpected startup call graph: {calls}')
    plt = (native / 'work/gen_eh_o2/c_04d0.c').read_text(encoding='utf-8')
    wrappers = [re.search(r'void f_' + address + r'\(Ctx \*c\) \{ host_import\(c, \d+\); \}', plt).group(0) for address in calls[1:]]
    bootstrap = ['#include "runtime.h"', 'void f_71002c5620(Ctx *c) { (void)c; guest_trap("nnMain is not ported", 0x71002c5620ULL); }', *wrappers, startup]
    (args.out / 'game_startup.c').write_text('\n\n'.join(bootstrap) + '\n', encoding='utf-8')
    print(f'Prepared {len(selected)} real game functions, {len(tests)} reference cases; output {args.out}')


if __name__ == '__main__':
    main()
