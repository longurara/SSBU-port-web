"""Exercise IEEE classification lowering without game bytes or GPU dependencies."""
import math
import random
import struct
import unittest

from shader_probe import instructions, lower


def fixture(width=1, float_bits=32):
    ops = [(19,[1]), (20,[2]), (22,[3,float_bits]), (33,[5,1])]
    boolean, floating = 2, 3
    if width > 1:
        ops += [(23,[7,2,width]), (23,[8,3,width])]
        boolean, floating = 7, 8
    ops += [(45,[floating,6]), (54,[1,10,0,5]), (248,[11]),
            (156,[boolean,12,6]), (157,[boolean,13,6]), (253,[]), (56,[])]
    words = [0x07230203,0x00010000,0,14,0]
    for opcode, args in ops:
        words += [((len(args)+1)<<16)|opcode, *args]
    return struct.pack('<%dI'%len(words), *words)


class ShaderLowering(unittest.TestCase):
    def evaluate(self, width, samples):
        result, changes = lower(fixture(width))
        words = struct.unpack('<%dI'%(len(result)//4), result)
        ops = list(instructions(words))
        self.assertFalse(any(op in (156,157) for op,_ in ops))
        self.assertEqual(changes[-1]['operations'],2)
        declared = [args[0] for op,args in ops if op in (19,20,21,22,23,33)]
        values = [args[1] for op,args in ops if op in (43,44,45,54,124,170,172,199)]
        self.assertEqual(len(declared+values),len(set(declared+values)))
        self.assertLess(max(declared+values),words[3])
        for bits in samples:
            env = {6:bits}
            for op,args in ops:
                if op==43: env[args[1]]=args[2]
                if op==44: env[args[1]]=[env[x] for x in args[2:]]
                if op==124: env[args[1]]=env[args[2]]
                if op in (170,172,199):
                    a,b=env[args[2]],env[args[3]]
                    def apply(x,y):
                        return x & y if op==199 else x==y if op==170 else x>y
                    env[args[1]]=list(map(apply,a,b)) if isinstance(a,list) else apply(a,b)
            lanes=bits if isinstance(bits,list) else [bits]
            floats=[struct.unpack('<f',struct.pack('<I',x))[0] for x in lanes]
            expected_nan=[math.isnan(x) for x in floats]
            expected_inf=[math.isinf(x) for x in floats]
            self.assertEqual(env[12],expected_nan if width>1 else expected_nan[0])
            self.assertEqual(env[13],expected_inf if width>1 else expected_inf[0])

    def test_scalar_special_values_and_random_bits(self):
        rng=random.Random(123)
        self.evaluate(1,[0,0x80000000,1,0x7f7fffff,0x7f800000,0xff800000,
                         0x7f800001,0x7fc00000,0xffc00001]+[rng.getrandbits(32) for _ in range(1000)])

    def test_vector_lanes(self):
        self.evaluate(4,[[0x7fc00000,0xff800000,0,1], [0xff800001,0x7f800000,0x3f800000,0x80000000]])

    def test_reject_other_float_widths(self):
        for bits in (16,64):
            with self.assertRaisesRegex(ValueError,'Only f32'):
                lower(fixture(float_bits=bits))


if __name__=='__main__':
    unittest.main()
