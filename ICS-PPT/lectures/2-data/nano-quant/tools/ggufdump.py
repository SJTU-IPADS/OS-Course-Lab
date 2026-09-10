#!/usr/bin/env python3
"""读一个 GGUF，把键值对与张量目录打出来，顺便检查排布是否自洽。

    python3 tools/ggufdump.py model.gguf            只看概况
    python3 tools/ggufdump.py model.gguf --kv       连键值对一起看
    python3 tools/ggufdump.py model.gguf --tensors  连张量目录一起看

检查三件事：目录里的偏移都对齐到 general.alignment；相邻张量的区间首尾相接；
最后一个张量的末端与文件长度相符。任一条不成立就非零退出。
"""
import argparse
import struct
import sys

U8, I8, U16, I16, U32, I32, F32, BOOL, STR, ARR, U64, I64, F64 = range(13)
FIX = {U8: '<B', I8: '<b', U16: '<H', I16: '<h', U32: '<I', I32: '<i',
       F32: '<f', BOOL: '<B', U64: '<Q', I64: '<q', F64: '<d'}
TYPES = {0: 'F32', 1: 'F16', 2: 'Q4_0', 3: 'Q4_1', 12: 'Q4_K', 14: 'Q6_K'}
BLOCK = {'F32': (1, 4), 'F16': (1, 2), 'Q4_0': (32, 18), 'Q4_1': (32, 20),
         'Q4_K': (256, 144), 'Q6_K': (256, 210)}


class R:
    def __init__(self, b):
        self.b, self.p = b, 0

    def take(self, n):
        v = self.b[self.p:self.p + n]
        self.p += n
        return v

    def u32(self): return struct.unpack('<I', self.take(4))[0]
    def u64(self): return struct.unpack('<Q', self.take(8))[0]
    def string(self): return self.take(self.u64()).decode('utf-8', 'replace')

    def value(self, t):
        if t == STR:
            return self.string()
        if t == ARR:
            et, n = self.u32(), self.u64()
            return [self.value(et) for _ in range(n)]
        f = FIX[t]
        v = struct.unpack(f, self.take(struct.calcsize(f)))[0]
        return bool(v) if t == BOOL else v


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('path')
    ap.add_argument('--kv', action='store_true')
    ap.add_argument('--tensors', action='store_true')
    a = ap.parse_args()

    raw = open(a.path, 'rb').read()
    r = R(raw)
    if r.take(4) != b'GGUF':
        sys.exit('不是 GGUF')
    ver, n_tensor, n_kv = r.u32(), r.u64(), r.u64()

    kvs = {}
    for _ in range(n_kv):
        k = r.string()
        kvs[k] = r.value(r.u32())

    tensors = []
    for _ in range(n_tensor):
        name = r.string()
        nd = r.u32()
        ne = [r.u64() for _ in range(nd)]
        t = r.u32()
        off = r.u64()
        tensors.append((name, ne, TYPES.get(t, '?%d' % t), off))

    align = kvs.get('general.alignment', 32)
    data_start = (r.p + align - 1) // align * align

    print('版本 %d，%d 个张量，%d 个键值对' % (ver, n_tensor, n_kv))
    print('目录结束于 %d，数据区起点 %d，文件 %d 字节' % (r.p, data_start, len(raw)))

    if a.kv:
        for k, v in kvs.items():
            s = repr(v)
            if len(s) > 90:
                s = '%s… 共 %d 项' % (s[:80], len(v) if isinstance(v, list) else 0)
            print('  %-48s %s' % (k, s))

    bad, cursor = 0, 0
    for name, ne, t, off in tensors:
        n = 1
        for d in ne:
            n *= d
        be, bb = BLOCK.get(t, (1, 0))
        nb = n // be * bb
        if off % align:
            print('  偏移不对齐：%s %d' % (name, off))
            bad += 1
        if off != cursor:
            print('  区间不连续：%s 应在 %d，实为 %d' % (name, cursor, off))
            bad += 1
        cursor = (off + nb + align - 1) // align * align
        if a.tensors:
            print('  %-32s %-5s %-18s %10d @ %d' %
                  (name, t, 'x'.join(str(d) for d in ne), nb, off))
    if data_start + cursor != len(raw):
        print('  末端对不上：目录算出 %d，文件 %d' % (data_start + cursor, len(raw)))
        bad += 1
    print('数据区 %d 字节，检查%s' % (cursor, '通过' if bad == 0 else '发现 %d 处不一致' % bad))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
