#!/usr/bin/env python3
"""Write a small model to standard output, for make test.

    python3 tools/mkmodel.py [SEED] > build/tiny.gguf
    ./mini-ollama build/tiny.gguf tests/workload.txt

The file is GGUF version 3 with the keys and tensor names frame/model.c
reads: matrices in Q4_0, vectors as floats, a byte-level BPE vocabulary with
its list of merges. The weights are random: 4-bit values and scales from
xorshift32, started from SEED (default 1), so the same seed gives the same
file on every machine. The model has 3 layers, 4 query heads and 2 key
heads of 16 floats, so one token takes 3 * 2 * 32 * 4 = 768 bytes of KV
cache. It answers with printable ASCII characters that mean nothing; it is
for checking the program, token by token.

Vocabulary, in this order:
  - the 256 single bytes. A token's text has one character per byte: a
    printable byte stands for itself, each of the other 68 for one
    character from U+0100 on (a space is U+0120). The printable bytes come
    first, as in the vocabulary of the real model;
  - <|im_start|> and <|im_end|>, which ends an answer;
  - the tokens the list MERGES builds, among them "user" and "assistant".

token_embd is both the first and the last layer. Its rows are all zero
except those of the printable ASCII bytes 0x20 .. 0x7e and of <|im_end|>:
every other token scores exactly 0 and is chosen only when none of these
scores above 0. The scales of the row of <|im_end|> are END_GAIN times
larger, so that some answers end before their last token: the conversation
then caches fewer tokens than it has blocks reserved for, and the next turn
starts from the tokens cached.
"""
import math
import struct
import sys

N_LAYER = 3
N_EMBD = 64
N_FF = 128
N_HEAD = 4
N_HEAD_KV = 2
HEAD_DIM = 16
ROPE_BASE = 10000.0
EPS = 1e-6
ALIGNMENT = 32
END_GAIN = 1.25     # the scales of the row of <|im_end|>, see above
QK_GAIN = 2.0       # the gains of the query and key normalizations: the
                    # scores of the attention then differ more from token
                    # to token, and so do the answers

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
        state = self.state
        state ^= (state << 13) & 0xffffffff
        state ^= state >> 17
        state ^= (state << 5) & 0xffffffff
        self.state = state
        return state

    def unit(self):                             # a number in [0, 1)
        return self.next() / 2.0 ** 32


def printable(byte):                            # as in frame/tokenizer.c
    return 33 <= byte <= 126 or 161 <= byte <= 172 or byte >= 174


def vocabulary():
    """The token texts, and the tokens of the bytes 0x20 .. 0x7e."""
    shown = [byte for byte in range(256) if printable(byte)]
    other = [byte for byte in range(256) if not printable(byte)]
    order = shown + other
    tokens = [chr(byte) for byte in shown]
    tokens += [chr(256 + index) for index in range(len(other))]
    ascii_tokens = {token for token, byte in enumerate(order) if 0x20 <= byte <= 0x7e}
    tokens += ["<|im_start|>", "<|im_end|>"]
    tokens += [left + right for left, right in MERGES]
    return tokens, ascii_tokens


def q4_matrix(rng, rows, cols, live=None, boosted=-1):
    """rows x cols weights in Q4_0, one row after another. 32 weights take
    18 bytes: a half-precision scale, then 16 bytes with weight j in the
    low 4 bits of byte j and weight j + 16 in the high 4 bits. A stored
    value q means the weight scale * (q - 8). Rows outside live are zero,
    the scales of row boosted are END_GAIN times larger.

    (q - 8)^2 is 21.5 on average, so with scale = 1 / sqrt(21.5 * cols) a
    row times a vector of root mean square 1 is about 1 in size."""
    scale = 1 / math.sqrt(21.5 * cols)
    out = bytearray()
    for row in range(rows):
        for block in range(cols // 32):
            gain = END_GAIN if row == boosted else 1.0
            out += struct.pack("<e", gain * scale * (0.75 + 0.5 * rng.unit()))
            if live is None or row in live:
                out += bytes(rng.next() >> 24 for byte in range(16))
            else:
                out += b"\x88" * 16             # q = 8 in both halves
    return bytes(out)


def gains(rng, count, factor=1.0):
    """count floats between 0.9 and 1.1, times factor: the gain of a
    normalization."""
    return struct.pack("<%df" % count,
                       *[factor * (0.9 + 0.2 * rng.unit()) for index in range(count)])


def string(text):
    encoded = text.encode("utf-8")
    return struct.pack("<Q", len(encoded)) + encoded


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
    end = tokens.index("<|im_end|>")
    embd, ff, head_dim = N_EMBD, N_FF, HEAD_DIM
    q_dim, kv_dim = N_HEAD * head_dim, N_HEAD_KV * head_dim

    pairs = [
        pair("general.architecture", STR, "tiny"),
        pair("general.alignment", U32, ALIGNMENT),
        pair("tiny.block_count", U32, N_LAYER),
        pair("tiny.embedding_length", U32, embd),
        pair("tiny.feed_forward_length", U32, ff),
        pair("tiny.attention.head_count", U32, N_HEAD),
        pair("tiny.attention.head_count_kv", U32, N_HEAD_KV),
        pair("tiny.attention.key_length", U32, head_dim),
        pair("tiny.attention.layer_norm_rms_epsilon", F32, EPS),
        pair("tiny.rope.freq_base", F32, ROPE_BASE),
        pair("tokenizer.ggml.tokens", ARR, tokens),
        pair("tokenizer.ggml.merges", ARR, [left + " " + right for left, right in MERGES]),
        pair("tokenizer.ggml.eos_token_id", U32, end),
    ]

    # (name, columns, rows, bytes): a matrix is rows x columns in Q4_0, a
    # vector has no rows and is stored as floats.
    tensors = [
        ("token_embd.weight", embd, len(tokens),
         q4_matrix(rng, len(tokens), embd, ascii_tokens | {end}, end)),
        ("output_norm.weight", embd, 0, gains(rng, embd)),
    ]
    for layer in range(N_LAYER):
        tensors += [
            ("blk.%d.attn_norm.weight" % layer, embd, 0, gains(rng, embd)),
            ("blk.%d.attn_q.weight" % layer, embd, q_dim, q4_matrix(rng, q_dim, embd)),
            ("blk.%d.attn_k.weight" % layer, embd, kv_dim, q4_matrix(rng, kv_dim, embd)),
            ("blk.%d.attn_v.weight" % layer, embd, kv_dim, q4_matrix(rng, kv_dim, embd)),
            ("blk.%d.attn_q_norm.weight" % layer, head_dim, 0, gains(rng, head_dim, QK_GAIN)),
            ("blk.%d.attn_k_norm.weight" % layer, head_dim, 0, gains(rng, head_dim, QK_GAIN)),
            ("blk.%d.attn_output.weight" % layer, q_dim, embd, q4_matrix(rng, embd, q_dim)),
            ("blk.%d.ffn_norm.weight" % layer, embd, 0, gains(rng, embd)),
            ("blk.%d.ffn_gate.weight" % layer, embd, ff, q4_matrix(rng, ff, embd)),
            ("blk.%d.ffn_up.weight" % layer, embd, ff, q4_matrix(rng, ff, embd)),
            ("blk.%d.ffn_down.weight" % layer, ff, embd, q4_matrix(rng, embd, ff)),
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
