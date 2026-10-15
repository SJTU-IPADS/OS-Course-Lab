#!/usr/bin/env python3
"""Write a tiny model for guest/mini-ollama to standard output.

    python3 tools/mkmodel.py [SEED] > guest/tiny.gguf
    ./mini-cpu guest/mini-ollama "hello" < guest/tiny.gguf

The file is GGUF version 3 with the keys and tensor names guest/ollama/model.c
reads: matrices in Q4_0, vectors as floats, a byte-level BPE vocabulary with
its list of merges. The weights are random: 4-bit values and scales from
xorshift32, started from SEED (default 1), so the same seed gives the same
file on every machine. The model answers with printable ASCII characters
that mean nothing; it is for checking the program, token by token.

Vocabulary, in this order:
  - the 256 single bytes. A token's text has one character per byte: a
    printable byte stands for itself, each of the other 68 for one
    character from U+0100 on (a space is U+0120). The printable bytes come
    first, as in the vocabulary of the real model;
  - <|im_start|> and <|im_end|>, which ends an answer;
  - the tokens the list MERGES builds, among them "user" and "assistant".

token_embd is both the first and the last layer. Its rows are all zero
except those of the printable ASCII bytes 0x20 .. 0x7e: every other token
scores exactly 0 and is chosen only when no ASCII token scores above 0.
"""
import math
import struct
import sys

N_LAYER = 2
N_EMBD = 32
N_FF = 64
N_HEAD = 2
N_HEAD_KV = 1
HEAD_DIM = 16
ROPE_BASE = 10000.0
EPS = 1e-6
ALIGNMENT = 32

U32, F32, STR, ARR = 4, 6, 8, 9                 # GGUF value types
TENSOR_F32, TENSOR_Q4_0 = 0, 2                  # GGUF tensor types

SP = "\u0120"                                   # a space inside a token's text
MERGES = [                                      # the earliest merge first
    ("t", "h"), ("h", "e"), ("i", "n"), ("e", "r"), ("a", "n"), ("th", "e"),
    (SP, "t"), (SP, "a"), (SP, "the"), (SP, "in"),
    ("u", "s"), ("us", "er"),                                   # user
    ("s", "t"), ("a", "s"), ("as", "s"), ("i", "st"), ("an", "t"),
    ("ass", "ist"), ("assist", "ant"),                          # assistant
]


class Xorshift32:
    def __init__(self, seed):
        self.state = (seed * 2654435761 ^ 0x9e3779b9) & 0xffffffff or 1

    def next(self):
        s = self.state
        s ^= (s << 13) & 0xffffffff
        s ^= s >> 17
        s ^= (s << 5) & 0xffffffff
        self.state = s
        return s

    def unit(self):                             # a number in [0, 1)
        return self.next() / 2.0 ** 32


def printable(b):                               # as in guest/ollama/tokenizer.c
    return 33 <= b <= 126 or 161 <= b <= 172 or b >= 174


def vocabulary():
    """The token texts, and the tokens of the bytes 0x20 .. 0x7e."""
    shown = [b for b in range(256) if printable(b)]
    other = [b for b in range(256) if not printable(b)]
    order = shown + other
    tokens = [chr(b) for b in shown] + [chr(256 + i) for i in range(len(other))]
    ascii_tokens = {i for i, b in enumerate(order) if 0x20 <= b <= 0x7e}
    tokens += ["<|im_start|>", "<|im_end|>"]
    tokens += [left + right for left, right in MERGES]
    return tokens, ascii_tokens


def q4_matrix(rng, rows, cols, live=None):
    """rows x cols weights in Q4_0, one row after another. 32 weights take
    18 bytes: a half-precision scale d, then 16 bytes with weight j in the
    low 4 bits of byte j and weight j + 16 in the high 4 bits. A stored
    value q means the weight d * (q - 8). Rows outside live are zero.

    (q - 8)^2 is 21.5 on average, so with d = 1 / sqrt(21.5 * cols) a row
    times a vector of root mean square 1 is about 1 in size."""
    d = 1 / math.sqrt(21.5 * cols)
    out = bytearray()
    for r in range(rows):
        for _ in range(cols // 32):
            out += struct.pack("<e", d * (0.75 + 0.5 * rng.unit()))
            if live is None or r in live:
                out += bytes(rng.next() >> 24 for _ in range(16))
            else:
                out += b"\x88" * 16             # q = 8 in both halves
    return bytes(out)


def gains(rng, n):
    """n floats between 0.9 and 1.1: the gain of a normalization."""
    return struct.pack("<%df" % n, *[0.9 + 0.2 * rng.unit() for _ in range(n)])


def string(s):
    b = s.encode("utf-8")
    return struct.pack("<Q", len(b)) + b


def pair(key, kind, value):
    if kind == U32:
        body = struct.pack("<I", value)
    elif kind == F32:
        body = struct.pack("<f", value)
    elif kind == STR:
        body = string(value)
    else:                                       # an array of strings
        body = struct.pack("<IQ", STR, len(value)) + b"".join(map(string, value))
    return string(key) + struct.pack("<I", kind) + body


def main(argv):
    if len(argv) > 1 or (argv and not argv[0].isdigit()):
        sys.exit("usage: mkmodel.py [SEED] > tiny.gguf")
    rng = Xorshift32(int(argv[0]) if argv else 1)
    tokens, ascii_tokens = vocabulary()
    e, f, d = N_EMBD, N_FF, HEAD_DIM
    q, kv = N_HEAD * d, N_HEAD_KV * d

    pairs = [
        pair("general.architecture", STR, "tiny"),
        pair("general.alignment", U32, ALIGNMENT),
        pair("tiny.block_count", U32, N_LAYER),
        pair("tiny.embedding_length", U32, e),
        pair("tiny.feed_forward_length", U32, f),
        pair("tiny.attention.head_count", U32, N_HEAD),
        pair("tiny.attention.head_count_kv", U32, N_HEAD_KV),
        pair("tiny.attention.key_length", U32, d),
        pair("tiny.attention.layer_norm_rms_epsilon", F32, EPS),
        pair("tiny.rope.freq_base", F32, ROPE_BASE),
        pair("tokenizer.ggml.tokens", ARR, tokens),
        pair("tokenizer.ggml.merges", ARR, [left + " " + right for left, right in MERGES]),
        pair("tokenizer.ggml.eos_token_id", U32, tokens.index("<|im_end|>")),
    ]

    # (name, columns, rows, bytes): a matrix is rows x columns in Q4_0, a
    # vector has no rows and is stored as floats.
    tensors = [
        ("token_embd.weight", e, len(tokens), q4_matrix(rng, len(tokens), e, ascii_tokens)),
        ("output_norm.weight", e, 0, gains(rng, e)),
    ]
    for l in range(N_LAYER):
        tensors += [
            ("blk.%d.attn_norm.weight" % l, e, 0, gains(rng, e)),
            ("blk.%d.attn_q.weight" % l, e, q, q4_matrix(rng, q, e)),
            ("blk.%d.attn_k.weight" % l, e, kv, q4_matrix(rng, kv, e)),
            ("blk.%d.attn_v.weight" % l, e, kv, q4_matrix(rng, kv, e)),
            ("blk.%d.attn_q_norm.weight" % l, d, 0, gains(rng, d)),
            ("blk.%d.attn_k_norm.weight" % l, d, 0, gains(rng, d)),
            ("blk.%d.attn_output.weight" % l, q, e, q4_matrix(rng, e, q)),
            ("blk.%d.ffn_norm.weight" % l, e, 0, gains(rng, e)),
            ("blk.%d.ffn_gate.weight" % l, e, f, q4_matrix(rng, f, e)),
            ("blk.%d.ffn_up.weight" % l, e, f, q4_matrix(rng, f, e)),
            ("blk.%d.ffn_down.weight" % l, f, e, q4_matrix(rng, e, f)),
        ]

    # The directory gives each tensor its offset in the data area; both the
    # data area and every tensor in it start at a multiple of ALIGNMENT.
    directory = data = b""
    for name, cols, rows, blob in tensors:
        dims = [cols, rows] if rows else [cols]
        directory += string(name) + struct.pack("<I%dQ" % len(dims), len(dims), *dims)
        directory += struct.pack("<IQ", TENSOR_Q4_0 if rows else TENSOR_F32, len(data))
        data += blob + b"\0" * (-len(blob) % ALIGNMENT)

    head = b"GGUF" + struct.pack("<IQQ", 3, len(tensors), len(pairs))
    head += b"".join(pairs) + directory
    head += b"\0" * (-len(head) % ALIGNMENT)
    sys.stdout.buffer.write(head + data)


if __name__ == "__main__":
    main(sys.argv[1:])
