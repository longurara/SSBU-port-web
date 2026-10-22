"""Synthetic container and output-path checks; no game data or keys required."""
from pathlib import Path
import struct
import tempfile
import unittest

from inspect_xci import Nsp, Xci, open_package


def pfs(entries):
    strings, table, data = bytearray(), bytearray(), bytearray()
    for name, payload in entries:
        offset = len(strings); strings += name.encode() + b'\0'
        table += struct.pack('<QQII', len(data), len(payload), offset, 0)
        data += payload
    return struct.pack('<4sIII', b'PFS0', len(entries), len(strings), 0) + table + strings + data


def hfs(entries):
    strings, table, data = bytearray(), bytearray(), bytearray()
    for name, payload in entries:
        offset = len(strings); strings += name.encode() + b'\0'
        table += struct.pack('<QQII8x32s', len(data), len(payload), offset, 0, bytes(32))
        data += payload
    return struct.pack('<4sIII', b'HFS0', len(entries), len(strings), 0) + table + strings + data


class PackageTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)

    def tearDown(self): self.temp.cleanup()

    def test_nsp_bounds_and_ticket_selection(self):
        ticket_a, ticket_b = bytearray(704), bytearray(704)
        ticket_a[0x2a0:0x2b0] = b'A' * 16
        ticket_b[0x2a0:0x2b0] = b'B' * 16
        path = self.root / 'fixture.nsp'
        path.write_bytes(pfs([('fixture.nca', b'example'), ('a.tik', ticket_a), ('b.tik', ticket_b)]))
        package = open_package(path)
        try:
            self.assertIsInstance(package, Nsp)
            self.assertEqual(package.read('fixture.nca', 1, 3), b'xam')
            for offset, count in [(-1, 1), (0, -1), (7, 1), (6, 2)]:
                with self.assertRaises(ValueError): package.read('fixture.nca', offset, count)
            package.selected_rights = b'B' * 16
            self.assertEqual(package.ticket(), ticket_b)
            package.selected_rights = b'C' * 16
            self.assertIsNone(package.ticket())
        finally: package.f.close()

    def test_xci_secure_partition(self):
        header = bytearray(512); header[0x100:0x104] = b'HEAD'
        struct.pack_into('<Q', header, 0x130, 512)
        path = self.root / 'fixture.xci'
        path.write_bytes(header + hfs([('secure', hfs([('fixture.nca', b'synthetic')]))]))
        package = open_package(path)
        try:
            self.assertIsInstance(package, Xci)
            self.assertEqual(package.read('fixture.nca', 0, 9), b'synthetic')
        finally: package.f.close()

    def test_truncated_and_duplicate_entries_rejected(self):
        for name, blob in [('truncated', pfs([('item', b'123')])[:-1]),
                           ('duplicate', pfs([('same', b'a'), ('same', b'b')])),
                           ('bad-magic', bytes(512))]:
            path = self.root / name; path.write_bytes(blob)
            with self.subTest(name=name), self.assertRaises(ValueError): open_package(path)


if __name__ == '__main__': unittest.main()
