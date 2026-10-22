"""Read-only AArch64 oracle for the user's SDK next-prime implementation."""
import json
from pathlib import Path
import struct
from unicorn import Uc, UC_ARCH_ARM64, UC_MODE_ARM
from unicorn.arm64_const import UC_ARM64_REG_X0, UC_ARM64_REG_X30, UC_ARM64_REG_SP, UC_ARM64_REG_PC

ROOT = Path(__file__).resolve().parents[1]
BASE, STACK, RETURN = 0x7500000000, 0x7300100000, 0x7300000000


def main():
    image = (ROOT / 'build/native/binaries/13.0.4/sdk.elf').read_bytes()
    uc = Uc(UC_ARCH_ARM64, UC_MODE_ARM)
    uc.mem_map(BASE, 0xcfb000)
    phoff = struct.unpack_from('<Q', image, 32)[0]
    phnum = struct.unpack_from('<H', image, 56)[0]
    for i in range(phnum):
        kind, _, offset, address, _, size, _, _ = struct.unpack_from('<IIQQQQQQ', image, phoff + i * 56)
        if kind == 1:
            uc.mem_write(BASE + address, image[offset:offset + size])
    uc.mem_map(STACK, 65536); uc.mem_map(RETURN, 4096)
    cases = []
    for value in (0, 1, 2, 3, 4, 5, 6, 10, 11, 100, 1000, 65536):
        uc.reg_write(UC_ARM64_REG_X0, value)
        uc.reg_write(UC_ARM64_REG_SP, STACK + 65520)
        uc.reg_write(UC_ARM64_REG_X30, RETURN)
        uc.emu_start(0x7500463f60, RETURN, count=1000000)
        if uc.reg_read(UC_ARM64_REG_PC) != RETURN:
            raise RuntimeError('SDK reference did not return')
        cases.append(dict(input=value, output=uc.reg_read(UC_ARM64_REG_X0)))
    path = ROOT / 'build/web-sdk/prime-reference.json'
    path.write_text(json.dumps(cases, indent=2) + '\n')
    print(json.dumps(cases))


if __name__ == '__main__':
    main()
