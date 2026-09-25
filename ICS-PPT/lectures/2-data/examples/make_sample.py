#!/usr/bin/env python3
"""Fetch two small slices of real Llama-3.2-1B weights for the quantization demos.

The slides compare Q4_0 / Q4_1 / Q4_K on real numbers rather than on a synthetic
distribution, and these two arrays are what they run on:

    ext/w-down-proj.bf16   4096 weights from layer 0's ffn down projection
                           (roughly symmetric around 0 — the common case)
    ext/w-final-norm.bf16  2048 weights from the final RMSNorm
                           (all positive, mean 2.35 — the asymmetric case)

Both are raw little-endian BF16, no header. They come out of the safetensors
file with one HTTP range request each after two requests for the safetensors header.
The tensor payload is 12 KB; the total transfer also includes the file's JSON header,
which supplies each tensor's byte offset.

The repository carries the two files, so this script only needs to run when
they are regenerated. See ext/PROVENANCE.md for the source and the license.
"""

import json
import pathlib
import struct
import urllib.request

REPO = "unsloth/Llama-3.2-1B-Instruct"      # an ungated mirror of the same weights
URL = f"https://huggingface.co/{REPO}/resolve/main/model.safetensors"
WANT = [("model.layers.0.mlp.down_proj.weight", 4096, "w-down-proj.bf16"),
        ("model.norm.weight", 2048, "w-final-norm.bf16")]


def get(start: int, length: int) -> bytes:
    """One HTTP range request: bytes [start, start + length) of the model file."""
    req = urllib.request.Request(URL, headers={"Range": f"bytes={start}-{start + length - 1}"})
    with urllib.request.urlopen(req) as r:
        return r.read()


def main() -> None:
    n = struct.unpack("<Q", get(0, 8))[0]          # header length: 8 bytes, little-endian
    header = json.loads(get(8, n))
    base = 8 + n                                   # the tensor data starts right after
    out = pathlib.Path(__file__).resolve().parent / "ext"
    for name, count, filename in WANT:
        off = header[name]["data_offsets"][0]
        data = get(base + off, 2 * count)
        (out / filename).write_bytes(data)
        print(f"{filename}  {len(data)} bytes  from {name}")


if __name__ == "__main__":
    main()
