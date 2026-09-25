#!/usr/bin/env python3
"""Write examples/tiny.gguf: a real, minimal GGUF file.

The lecture hexdumps this file on stage. It is a genuine GGUF v3 header —
same magic, same little-endian integers, same length-prefixed strings as the
1.9 GB file ollama downloads — small enough that every byte on the slide can
be accounted for.
"""

import pathlib
import struct

GGUF_TYPE_UINT32 = 4
GGUF_TYPE_STRING = 8
GGML_TYPE_F16 = 1


def string(s):
    b = s.encode("utf-8")
    return struct.pack("<Q", len(b)) + b


def build():
    out = b"GGUF"                       # magic, four ASCII bytes
    out += struct.pack("<I", 3)         # version
    out += struct.pack("<Q", 1)         # tensor count
    out += struct.pack("<Q", 2)         # metadata key/value count

    out += string("general.architecture") + struct.pack("<I", GGUF_TYPE_STRING)
    out += string("llama")
    out += string("llama.block_count") + struct.pack("<I", GGUF_TYPE_UINT32)
    out += struct.pack("<I", 32)

    out += string("token_embd.weight")  # one tensor: name, shape, type, offset
    out += struct.pack("<I", 2)         # number of dimensions
    out += struct.pack("<QQ", 8, 4)     # 8 x 4 elements
    out += struct.pack("<I", GGML_TYPE_F16)
    out += struct.pack("<Q", 0)         # offset into the tensor-data section

    pad = (-len(out)) % 32              # tensor data starts 32-byte aligned
    out += b"\0" * pad
    out += struct.pack("<32e", *[i * 0.5 for i in range(32)])
    return out


if __name__ == "__main__":
    path = pathlib.Path(__file__).resolve().parent / "tiny.gguf"
    path.write_bytes(build())
    print(f"{path}  {path.stat().st_size} bytes")
