#!/usr/bin/env python3
"""Where a GGUF file's bytes go: one line per tensor type, and bits per weight.

Usage: gguf_bits.py [file]   (default: the largest blob ollama has downloaded)
"""

import collections
import glob
import os
import struct
import sys

# name, weights per block, bytes per block. A block is the unit a quantized
# type stores: 32 or 256 weights sharing one scale, written as one struct.
TYPES = {0: ("F32", 1, 4), 1: ("F16", 1, 2), 2: ("Q4_0", 32, 18),
         8: ("Q8_0", 32, 34), 10: ("Q2_K", 256, 84), 11: ("Q3_K", 256, 110),
         12: ("Q4_K", 256, 144), 13: ("Q5_K", 256, 176), 14: ("Q6_K", 256, 210),
         30: ("BF16", 1, 2)}

SCALAR = {0: 1, 1: 1, 2: 2, 3: 2, 4: 4, 5: 4, 6: 4, 7: 1, 10: 8, 11: 8, 12: 8}


class Reader:
    def __init__(self, f):
        self.f = f

    def u32(self):
        return struct.unpack("<I", self.f.read(4))[0]

    def u64(self):
        return struct.unpack("<Q", self.f.read(8))[0]

    def string(self):
        return self.f.read(self.u64())

    def skip_value(self, kind):
        """Metadata values are not the subject here; step over them."""
        if kind == 8:
            self.string()
        elif kind == 9:
            item, count = self.u32(), self.u64()
            for _ in range(count):
                self.skip_value(item)
        else:
            self.f.read(SCALAR[kind])


def main(path):
    with open(path, "rb") as f:
        r = Reader(f)
        assert r.f.read(4) == b"GGUF", "not a GGUF file"
        version, tensors, pairs = r.u32(), r.u64(), r.u64()
        for _ in range(pairs):
            r.string()
            r.skip_value(r.u32())

        totals = collections.defaultdict(lambda: [0, 0, 0])   # count, weights, bytes
        for _ in range(tensors):
            r.string()
            dims = [r.u64() for _ in range(r.u32())]
            name, per_block, block_bytes = TYPES[r.u32()]
            r.u64()                                           # data offset
            weights = 1
            for d in dims:
                weights *= d
            row = totals[name]
            row[0] += 1
            row[1] += weights
            row[2] += weights // per_block * block_bytes
        header = f.tell()

    print(f"{os.path.basename(path)[:24]}  GGUF v{version}  {tensors} tensors")
    print(f"{'type':6s} {'tensors':>8s} {'weights':>14s} {'bytes':>14s} {'bits/w':>7s}")
    for name, (count, weights, size) in sorted(totals.items(), key=lambda kv: -kv[1][2]):
        print(f"{name:6s} {count:8d} {weights:14d} {size:14d} {size * 8 / weights:7.3f}")
    weights = sum(row[1] for row in totals.values())
    size = sum(row[2] for row in totals.values())
    print(f"{'all':6s} {tensors:8d} {weights:14d} {size:14d} {size * 8 / weights:7.3f}")
    print(f"metadata + tensor table {header} bytes, file {os.path.getsize(path)} bytes "
          f"= {os.path.getsize(path) / 2 ** 30:.2f} GiB")


def find_model():
    """The file named on the command line, else ollama's largest blob."""
    if len(sys.argv) > 1:
        return sys.argv[1]
    blobs = glob.glob(os.path.expanduser("~/.ollama/models/blobs/sha256-*"))
    if not blobs:
        sys.exit("no model found: pass a .gguf path, or run `ollama pull llama3.2`")
    return max(blobs, key=os.path.getsize)


if __name__ == "__main__":
    main(find_model())
