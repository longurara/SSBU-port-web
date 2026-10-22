"""Control-flow preservation cases for generated-C pruning."""
import struct
import unittest
import tempfile
from pathlib import Path
from prepare_runtime import BASE, reachable_body, prune_huge_dispatch, load_bucket, computed_addresses, computed_code_pointers, pointer_table_targets, callback_pointer_tables


def prune(words, statements):
    image = bytearray(4096 + 4 * len(words))
    for i, word in enumerate(words):
        struct.pack_into('<I', image, 4096 + i * 4, word)
    body = 'void f_7100000000(Ctx *c) {\n'
    body += ''.join(f'L_{BASE + i * 4:x}: {statement}\n' for i, statement in enumerate(statements))
    body += f'  f_{BASE + len(words) * 4:x}(c); return;\n}}'
    return reachable_body('f_7100000000', body, image)


class ReachabilityTests(unittest.TestCase):
    def test_callback_tables_are_targeted_not_recursive(self):
        data=BASE+0x4000000;a,b,c,d=(BASE+i for i in (0x100,0x200,0x300,0x400))
        relocations={data:a,data+8:b,data+32:b,data+40:c,data+64:c,data+72:d}
        self.assertEqual(callback_pointer_tables([a],relocations),{data:[a,b]})
        self.assertEqual(callback_pointer_tables([b],relocations),{data:[a,b],data+32:[b,c]})
        self.assertEqual(callback_pointer_tables([d,a],relocations),{data:[a,b],data+64:[c,d]})

    def test_callback_tables_do_not_truncate_large_groups_or_cross_gaps(self):
        data=BASE+0x4000000;a,b=BASE+0x100,BASE+0x200
        relocations={data:a,data+8:b,data+16:a,data+40:a,data+48:b}
        self.assertEqual(callback_pointer_tables([a],relocations,limit=2),{data+40:[a,b]})
        self.assertEqual(callback_pointer_tables([],relocations),{})
        self.assertEqual(callback_pointer_tables([a],{data+1:a}),{})
        with self.assertRaises(ValueError):callback_pointer_tables([a],relocations,limit=0)

    def test_code_and_data_addresses_are_discovered_without_rewriting(self):
        body='c->x[1] = (uint64_t)(0x710353c000ULL); c->x[1] = (uint64_t)(c->x[1] + 3440ULL);\n'
        body+='c->x[2] = (uint64_t)(0x7104000000ULL); c->x[2] = (uint64_t)(c->x[2] + 16ULL);'
        self.assertEqual(list(computed_addresses(body)),[BASE+0x353cd70,BASE+0x4000010])
        self.assertEqual(list(computed_code_pointers(body)),[BASE+0x353cd70])
        # An intervening unknown register assignment must stop constant tracking.
        body='c->x[1] = (uint64_t)(0x7104000000ULL); c->x[1] = unknown; c->x[1] = (uint64_t)(c->x[1] + 16ULL);'
        self.assertEqual(list(computed_addresses(body)),[])
        # GOT ADRP+LDR loads resolve code and data pointers and allow +16 vtable offsets.
        got_body=(
            'c->x[1] = (uint64_t)(0x71052a2000ULL); '
            '{ uint64_t a = c->x[1] + 632ULL; c->x[1] = (uint64_t)(rd64(a)); }\n'
            'c->x[8] = (uint64_t)(0x71052a2000ULL); '
            '{ uint64_t a = c->x[8] + 640ULL; c->x[8] = (uint64_t)(rd64(a)); } '
            'c->x[8] = (uint64_t)(c->x[8] + 16ULL);'
        )
        p_relocs={0x71052a2278:0x7100027bf0}
        d_relocs={0x71052a2280:0x7104f815e8}
        self.assertEqual(
            list(computed_addresses(got_body,pointer_relocations=p_relocs,data_relocations=d_relocs)),
            [0x7100027bf0,0x7104f815e8,0x7104f815f8],
        )
        self.assertEqual(
            list(computed_code_pointers(got_body,pointer_relocations=p_relocs,data_relocations=d_relocs)),
            [0x7100027bf0],
        )

    def test_pointer_table_follows_only_contiguous_relocations_and_is_bounded(self):
        data=BASE+0x4000000;targets={data:BASE+0x100,data+8:BASE+0x200,data+24:BASE+0x300}
        self.assertEqual(pointer_table_targets(data,targets),[BASE+0x100,BASE+0x200])
        self.assertEqual(pointer_table_targets(data,targets,limit=1),[BASE+0x100])
        self.assertEqual(pointer_table_targets(data+1,targets),[])
        self.assertEqual(pointer_table_targets(data+16,targets),[])
        obj=BASE+0x5000000;obj2=BASE+0x5000010
        data_relocs={obj:data,obj2:obj}
        self.assertEqual(pointer_table_targets(obj,targets,data_relocations=data_relocs),[BASE+0x100,BASE+0x200])
        self.assertEqual(pointer_table_targets(obj2,targets,data_relocations=data_relocs),[])

    def test_optional_bucket_gap_does_not_hide_required_dependency(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory=Path(temporary);cache={}
            (directory/'c_0000.c').write_text('void f_7100000000(Ctx *c) { return; }\n')
            self.assertIn('f_7100000000',load_bucket(directory,0,cache))
            self.assertEqual(load_bucket(directory,1,cache,required=False),{})
            with self.assertRaisesRegex(FileNotFoundError,'Required native code bucket'):
                load_bucket(directory,1,cache)
    def test_ret_drops_padding_and_false_callee(self):
        result = prune([0xd65f03c0, 0], ['return;', 'guest_unimpl(0,0,"padding");'])
        self.assertNotIn('guest_unimpl', result)
        self.assertNotIn('f_7100000008(c)', result)

    def test_conditional_branch_keeps_both_paths(self):
        result = prune([0x54000040, 0xd65f03c0, 0xd65f03c0, 0],
                       ['if(cond(c,0)) goto L_7100000008;', 'return;', 'return;', 'bad();'])
        self.assertIn('L_7100000004:', result)
        self.assertIn('L_7100000008:', result)
        self.assertNotIn('bad()', result)

    def test_unconditional_branch_skips_unreachable_import(self):
        result = prune([0x14000002, 0x94000000, 0xd65f03c0],
                       ['goto L_7100000008;', 'f_dead(c);', 'return;'])
        self.assertNotIn('f_dead', result)
        self.assertIn('L_7100000008:', result)

    def test_call_keeps_return_continuation(self):
        result = prune([0x94000000, 0xd65f03c0], ['f_live(c);', 'return;'])
        self.assertIn('f_live(c)', result)
        self.assertIn('L_7100000004:', result)

    def test_live_fallthrough_is_retained(self):
        result = prune([0xd503201f], ['(void)c;'])
        self.assertIn('f_7100000004(c)', result)

    def test_indirect_dispatch_is_conservative(self):
        result = prune([0xd61f0100, 0xd65f03c0], ['goto L_dispatch;', 'return;'])
        self.assertIn('L_7100000004:', result)

    def test_prune_huge_dispatch_handles_multiple_jump_tables(self):
        jt1_base = BASE + 0x2000
        jt2_base = BASE + 0x2100
        target1 = BASE + 0x100
        target2 = BASE + 0x200
        image = bytearray(0x1000 + 0x3000)
        struct.pack_into('<i', image, 0x1000 + (jt1_base - BASE), target1 - jt1_base)
        struct.pack_into('<i', image, 0x1000 + (jt1_base - BASE) + 4, -0x10000)
        struct.pack_into('<i', image, 0x1000 + (jt2_base - BASE), target2 - jt2_base)
        struct.pack_into('<i', image, 0x1000 + (jt2_base - BASE) + 4, -0x10000)
        lines = ['void f_7100000000(Ctx *c) {', '  uint64_t jt = 0;']
        lines.append(f'L_{BASE:x}: c->x[9] = (uint64_t)(0x{BASE + 0x2000:x}ULL);')
        lines.append(f'L_{BASE + 4:x}: c->x[9] = (uint64_t)(c->x[9] + 0ULL);')
        lines.append(f'L_{BASE + 8:x}: {{ uint64_t a = c->x[9] + (((uint64_t)c->x[8]) << 2); c->x[8] = (uint64_t)((int64_t)(int32_t)rd32(a)); }}')
        lines.append(f'L_{BASE + 12:x}: {{ jt = c->x[8]; goto L_dispatch; }}')
        lines.append(f'L_{BASE + 16:x}: c->x[10] = (uint64_t)(0x{BASE + 0x2000:x}ULL);')
        lines.append(f'L_{BASE + 20:x}: c->x[10] = (uint64_t)(c->x[10] + 256ULL);')
        lines.append(f'L_{BASE + 24:x}: {{ uint64_t a = c->x[10] + (((uint64_t)c->x[8]) << 2); c->x[8] = (uint64_t)((int64_t)(int32_t)rd32(a)); }}')
        lines.append(f'L_{BASE + 28:x}: {{ jt = c->x[8]; goto L_dispatch; }}')
        lines.append(f'L_{BASE + 32:x}: {{ uint64_t a = c->x[0] + 12ULL; c->x[1] = (uint64_t)(rd32(a)); }}')
        lines.append(f'L_{BASE + 36:x}: {{ jt = c->x[2]; goto L_dispatch; }}')
        for i in range(10, 160):
            lines.append(f'L_{BASE + i * 4:x}: return;')
        lines.append('L_dispatch: switch (jt) {')
        for i in range(160):
            lines.append(f'  case 0x{BASE + i * 4:x}ULL: goto L_{BASE + i * 4:x};')
        lines.append('  default: call_indirect(c, jt); return;\n}\n}')
        pruned = prune_huge_dispatch('\n'.join(lines), image)
        self.assertIn(f'case 0x{target1:x}ULL: goto L_{target1:x};', pruned)
        self.assertIn(f'case 0x{target2:x}ULL: goto L_{target2:x};', pruned)
        self.assertNotIn(f'case 0x{BASE + 0x104:x}ULL:', pruned)

    def test_prune_huge_dispatch_ignores_rd64_tail_call_preceded_by_signed_rd32(self):
        image = bytearray(0x2000)
        lines = ['void f_7100000000(Ctx *c) {', '  uint64_t jt = 0;']
        lines.append(f'L_{BASE:x}: {{ uint64_t a = c->x[0] + 16ULL; c->x[9] = (uint64_t)((int64_t)(int32_t)rd32(a)); }}')
        lines.append(f'L_{BASE + 4:x}: {{ uint64_t a = c->x[8] + 24ULL; c->x[2] = (uint64_t)(rd64(a)); }}')
        lines.append(f'L_{BASE + 8:x}: {{ jt = c->x[2]; goto L_dispatch; }}')
        for i in range(3, 160):
            lines.append(f'L_{BASE + i * 4:x}: return;')
        lines.append('L_dispatch: switch (jt) {')
        for i in range(160):
            lines.append(f'  case 0x{BASE + i * 4:x}ULL: goto L_{BASE + i * 4:x};')
        lines.append('  default: call_indirect(c, jt); return;\n}\n}')
        pruned = prune_huge_dispatch('\n'.join(lines), image)
        self.assertNotIn('case 0x', pruned)
        self.assertIn('default: call_indirect(c, jt); return;', pruned)


if __name__ == '__main__':
    unittest.main()
