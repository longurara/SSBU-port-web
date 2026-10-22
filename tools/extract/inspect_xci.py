"""Read XCI/NSP metadata and ExeFS NSO headers without extracting game content.

    python tools/extract/inspect_xci.py <file.xci> [--report build/xci-inspection.json]

Uses the existing romfs_extract.py key/decryption code. Keys are read locally
from ~/.switch/prod.keys and never included in output. HFS0 structure follows
the XCI support proposed in upstream PR #38; payload hashes are not verified.
"""
import argparse
import json
from pathlib import Path
import struct
import sys
from types import SimpleNamespace


class Xci:
    def __init__(self, path):
        self.path = Path(path)
        self.size = self.path.stat().st_size
        self.f = self.path.open('rb')
        try:
            self._parse()
        except BaseException:
            self.f.close()
            raise

    def _parse(self):
        header = self.read_at(0, 0x200)
        if header[0x100:0x104] != b'HEAD': raise ValueError('Not an XCI HEAD header')
        root = struct.unpack_from('<Q', header, 0x130)[0]
        self.partitions = self.hfs(root)
        if 'secure' not in self.partitions: raise ValueError('Missing secure partition')
        self.files = self.hfs(*self.partitions['secure'])
        self.selected = None
        self.selected_rights = None

    def read_at(self, offset, size):
        if offset < 0 or size < 0 or offset > self.size or size > self.size - offset:
            raise ValueError('Container entry extends beyond current file size')
        self.f.seek(offset)
        data = self.f.read(size)
        if len(data) != size: raise ValueError('Short read (file may still be downloading)')
        return data

    def hfs(self, offset, extent=None):
        magic, count, strings_size, _ = struct.unpack('<4sIII', self.read_at(offset, 16))
        if magic != b'HFS0' or count > 8192 or strings_size > 1024 * 1024:
            raise ValueError('Invalid or excessive HFS0 table')
        header_size = 16 + 64 * count + strings_size
        if extent is not None and header_size > extent: raise ValueError('HFS0 table exceeds partition')
        table = self.read_at(offset + 16, count * 64)
        strings = self.read_at(offset + 16 + count * 64, strings_size)
        entries = {}
        for i in range(count):
            relative, size, name_offset = struct.unpack_from('<QQI', table, i * 64)
            end = strings.find(b'\0', name_offset)
            if name_offset >= strings_size or end < 0: raise ValueError('Invalid HFS0 filename')
            name = strings[name_offset:end].decode('utf8')
            absolute = offset + header_size + relative
            limit = offset + extent if extent is not None else self.size
            if absolute > limit or size > limit - absolute: raise ValueError('Truncated HFS0 entry')
            if name in entries: raise ValueError('Duplicate HFS0 filename')
            entries[name] = absolute, size
        return entries

    def read(self, name, offset, size):
        base, length = self.files[name]
        if offset < 0 or size < 0 or offset > length or size > length - offset:
            raise ValueError('Read exceeds NCA container entry')
        return self.read_at(base + offset, size)

    def program_nca(self): return self.selected
    def ticket(self):
        for name, (_, size) in self.files.items():
            if name.endswith('.tik') and 0x2b0 <= size <= 65536:
                ticket = self.read(name, 0, size)
                if self.selected_rights is None or ticket[0x2a0:0x2b0] == self.selected_rights:
                    return ticket
        return None


class Nsp(Xci):
    """Reuse bounded package reads and ticket selection with a PFS0 file table."""
    def _parse(self):
        magic, count, strings_size, _ = struct.unpack('<4sIII', self.read_at(0, 16))
        if magic != b'PFS0' or count > 8192 or strings_size > 1024 * 1024:
            raise ValueError('Invalid NSP PFS0 table')
        table = self.read_at(16, count * 24)
        strings = self.read_at(16 + count * 24, strings_size)
        base = 16 + count * 24 + strings_size
        self.files = {}
        for i in range(count):
            relative, size, name_offset = struct.unpack_from('<QQI', table, i * 24)
            end = strings.find(b'\0', name_offset)
            if name_offset >= strings_size or end < 0: raise ValueError('Invalid NSP filename')
            name = strings[name_offset:end].decode('utf8')
            absolute = base + relative
            if absolute > self.size or size > self.size - absolute: raise ValueError('Truncated NSP entry')
            if name in self.files: raise ValueError('Duplicate NSP filename')
            self.files[name] = absolute, size
        self.partitions = {'PFS0': (0, self.size)}
        self.selected = self.selected_rights = None


def open_package(path):
    with open(path, 'rb') as f: magic = f.read(4)
    return Nsp(path) if magic == b'PFS0' else Xci(path)


def pfs_files(nca, sec):
    offset = struct.unpack_from('<Q', sec['fs'], 0x40)[0]
    header = nca.read(sec, offset, 16)
    if header[:4] != b'PFS0': raise ValueError('No PFS0 at section data offset')
    count, strings_size = struct.unpack_from('<II', header, 4)
    if count > 8192 or strings_size > 1024 * 1024: raise ValueError('Excessive PFS0 metadata')
    table = nca.read(sec, offset + 16, count * 24 + strings_size)
    strings = table[count * 24:]
    base = offset + 16 + count * 24 + strings_size
    entries = {}
    for i in range(count):
        relative, size, name_offset = struct.unpack_from('<QQI', table, i * 24)
        end = strings.find(b'\0', name_offset)
        if name_offset >= len(strings) or end < 0: raise ValueError('Invalid PFS0 filename')
        if base + relative > sec['size'] or size > sec['size'] - base - relative:
            raise ValueError('PFS0 entry exceeds section')
        entries[strings[name_offset:end].decode('utf8')] = base + relative, size
    return entries


def inspect(path):
    # Import only after CLI parsing; existing module reads keys without printing.
    import romfs_extract as rx

    class Nca(rx.Nca):
        def read(self, sec, offset, size, ctr_val=None):
            if sec['crypto'] == 1:
                return self.nsp.read(self.name, sec['offset'] + offset, size)
            return super().read(sec, offset, size, ctr_val)

    package = open_package(path)
    report = {'path': str(package.path.resolve()), 'bytes': package.size,
              'format': type(package).__name__,
              'partitions': list(package.partitions), 'contents': [],
              'requiredVersion': '13.0.4', 'payloadHashesVerified': False,
              'scope': 'Read-only metadata and NSO headers; no game content extracted'}
    try:
        for name, (_, size) in package.files.items():
            if not name.endswith('.nca'): continue
            package.selected = name
            decrypted_header = rx.xts_decrypt(rx.KEYS['header_key'], package.read(name, 0, 0xc00), 0)
            if decrypted_header[0x200:0x204] != b'NCA3': raise ValueError('Cannot decrypt an NCA header')
            package.selected_rights = decrypted_header[0x230:0x240]
            nca = Nca(package)
            kind = nca.header[0x205]
            title = struct.unpack_from('<Q', nca.header, 0x210)[0]
            item = {'name': name, 'bytes': size, 'contentType': kind, 'titleId': f'{title:016x}',
                    'sections': [{'type': s['type'], 'crypto': s['crypto'], 'bytes': s['size']} for s in nca.sections]}
            if kind in (0, 1):
                sec = nca.section(1)
                entries = pfs_files(nca, sec)
                if kind == 0:
                    item['executables'] = {}
                    for filename, (offset, length) in entries.items():
                        if filename not in ('main', 'sdk'): continue
                        header = nca.read(sec, offset, min(length, 0x100))
                        if header[:4] != b'NSO0': raise ValueError('Expected NSO executable header')
                        text_size = struct.unpack_from('<I', header, 0x18)[0]
                        wanted = {'main': 0x39c7e90, 'sdk': 0x540950}[filename]
                        item['executables'][filename] = {'textBytes': text_size,
                            'expectedTextBytes': wanted, 'matchesRequiredTextSize': text_size == wanted,
                            'buildId': header[0x40:0x60].hex()}
                else:
                    item['metadata'] = []
                    for filename, (offset, length) in entries.items():
                        if not filename.endswith('.cnmt'): continue
                        header = nca.read(sec, offset, min(length, 0x20))
                        title_id, version = struct.unpack_from('<QI', header)
                        item['metadata'].append({'titleId': f'{title_id:016x}', 'versionRaw': version, 'type': header[0xc]})
            elif kind == 2:
                sec = nca.section(0)
                level6 = struct.unpack_from('<Q', sec['fs'], 0x8 + 0x10 + 5 * 0x18)[0]
                reader = SimpleNamespace(read=lambda offset, count: nca.read(sec, offset, count))
                for filename, offset, length in rx.walk(reader, level6):
                    if filename.endswith('control.nacp') and length >= 0x3070:
                        item['displayVersion'] = nca.read(sec, offset + 0x3060, 16).split(b'\0', 1)[0].decode('utf8')
            report['contents'].append(item)
        return report
    finally:
        package.f.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('xci')
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    try:
        report = inspect(args.xci)
    except Exception as error:
        # Do not dump key material or decrypted buffers in a traceback.
        print(f'Inspection failed ({type(error).__name__}): {error}', file=sys.stderr)
        return 1
    rendered = json.dumps(report, indent=2)
    print(rendered)
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(rendered + '\n', encoding='utf8')
    return 0


if __name__ == '__main__': sys.exit(main())
