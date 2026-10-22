"""Split the user's already lifted SDK into independently compilable Wasm units.

Generated SDK code remains under ignored build/. This does not supply an SDK
image or implement the host callees, loader or exception bridges.
"""
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'build/web-sdk'
PATTERN = re.compile(r'^static void (f_[0-9a-f]+)\(Ctx \*c\) \{(?:[^\n]*\}|\n.*?^\})', re.M | re.S)


def main():
    source = (ROOT / 'build/native/work/sdklift_gen.c').read_text(encoding='utf-8')
    functions = [(match.group(1), match.group(0).removeprefix('static ')) for match in PATTERN.finditer(source)]
    expected = int(re.search(r'const int sdk_fn_count = (\d+);', source).group(1))
    if len(functions) != expected:
        raise ValueError(f'Parsed {len(functions)} SDK functions, expected {expected}')
    OUT.mkdir(parents=True, exist_ok=True)
    header = '#include "sdklift.h"\n' + '\n'.join(f'void {name}(Ctx *c);' for name, _ in functions) + '\n'
    (OUT / 'sdk_functions.h').write_text(header, encoding='utf-8')
    units = []
    for i in range(0, len(functions), 48):
        name = f'sdk_{i // 48:04d}.c'
        (OUT / name).write_text('#include "sdk_functions.h"\n#define call_indirect sdk_dispatch\n' +
                                '\n\n'.join(body for _, body in functions[i:i + 48]) + '\n', encoding='utf-8')
        units.append(name)
    tables = source[source.index('const char *const sdk_host_names[]'):]
    tables = re.sub(r'^static void sdk_call_indirect[^\n]*\n?', '', tables, flags=re.M)
    (OUT / 'sdk_table.c').write_text('#include "sdk_functions.h"\n' + tables, encoding='utf-8')
    units.append('sdk_table.c')
    (OUT / 'manifest.json').write_text(json.dumps(dict(functions=len(functions), hostCallees=68, units=units), indent=2) + '\n', encoding='utf-8')
    # Self-contained SDK leaves can be bound immediately, before the SDK image,
    # service IPC and exception bridges are ported. Keep original function bodies.
    leaf = []
    for name, body in functions:
        executable = body.split('\n', 1)[1]
        if re.search(r'\b(?:f_[0-9a-f]+|call_indirect|sdk_host_call|sdk_trap_call|SDK_SETJMP|sdk_longjmp)\s*\(', executable):
            continue
        if re.search(r'0x75[0-9a-f]+ULL', re.sub(r'  c->fn = [^\n]+', '', executable)):
            continue
        if len(re.findall(r'^L_', body, re.M)) > 128:
            continue
        leaf.append((name, body))
    binding = dict(re.findall(r'\{0x[0-9a-f]+ULL, (f_[0-9a-f]+), "([^"\n]+)"\}', tables))
    leaf_source = '#include "runtime.h"\n#include "hostrt.h"\n' + '\n\n'.join(body for _, body in leaf)
    leaf_source += '\nconst ImportDef web_sdk_leaf_imports[] = {\n' + '\n'.join('{' + json.dumps(binding[name]) + ', ' + name + '},' for name, _ in leaf if name in binding) + '\n};\n'
    leaf_source += 'const unsigned web_sdk_leaf_count = sizeof(web_sdk_leaf_imports)/sizeof(web_sdk_leaf_imports[0]);\n'
    (OUT / 'sdk_leaf.c').write_text(leaf_source, encoding='utf-8')
    print(f'Isolated {len(leaf)} SDK leaves without host/image dependencies')
    print(f'Prepared {len(functions)} real SDK functions in {len(units)} units')
    from check_sdk_prime import main as prepare_prime_reference
    prepare_prime_reference()


if __name__ == '__main__':
    main()
