#!/usr/bin/env python3
"""Print the structure of a safetensors file: header, tensors, byte accounts.

Usage:
    sthead.py <model.safetensors>            one line per top-level group
    sthead.py <model.safetensors> <prefix>   one line per tensor under <prefix>
"""
import collections
import json
import struct
import sys


def read_header(path):
    with open(path, "rb") as f:
        n = struct.unpack("<Q", f.read(8))[0]
        hdr = json.loads(f.read(n))
    hdr.pop("__metadata__", None)
    return n, hdr


def nbytes(rec):
    lo, hi = rec["data_offsets"]
    return hi - lo


def main(argv):
    path = argv[1]
    n, hdr = read_header(path)
    if len(argv) > 2:
        prefix = argv[2]
        for name in sorted(hdr):
            if name.startswith(prefix):
                r = hdr[name]
                print("%-30s %-5s %-14s %10d" %
                      (name[len(prefix):], r["dtype"], r["shape"], nbytes(r)))
        return 0
    print("header %d bytes, %d tensors" % (n, len(hdr)))
    group = collections.Counter()
    size = collections.Counter()
    for name, r in hdr.items():
        key = ".".join(name.split(".")[:3])
        group[key] += 1
        size[key] += nbytes(r)
    for key in sorted(group):
        print("%-40s %4d %12d" % (key, group[key], size[key]))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
