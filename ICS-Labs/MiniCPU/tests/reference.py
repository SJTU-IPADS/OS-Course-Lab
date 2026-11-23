#!/usr/bin/env python3
"""Compute the expected results of guest programs independently, to check
the emulator against.

Standard library only, and no code shared with guest/*.c: the two sides are
far less likely to agree on a wrong result than on the right one.

    python3 tests/reference.py sum [n]
    python3 tests/reference.py matmul [n]
    python3 tests/reference.py simd [n]
    python3 tests/reference.py tinytf [seed [n]]
"""
import sys


def sum_ref(count):
    total = 0
    for number in range(1, count + 1):
        total += number
    return total


def matmul_ref(size):
    left = [[row + 2 * col for col in range(size)] for row in range(size)]
    right = [[3 * row - col + 1 for col in range(size)] for row in range(size)]
    product = [[sum(left[row][inner] * right[inner][col] for inner in range(size))
                for col in range(size)]
               for row in range(size)]
    return sum(sum(line) for line in product)


def simd_ref(count):
    total = 5 + sum((index % 7 - 3) * (2 * index + 1) for index in range(count))
    return (total + 2**31) % 2**32 - 2**31      # an int in the guest


def tinytf_ref(seed, count):
    """guest/tinytf: one attention layer and one MLP in integers. Written
    from its description: weights from xorshift32, products scaled by >> 4,
    activations clamped to [-1024, 1024], attention weights
    min(max(q.k >> 16, 0), 15) + 1, the average truncated toward zero."""
    VOCAB, DIM, HIDDEN, PROMPT = 16, 16, 36, 4
    LONGEST = PROMPT + 12
    if not 1 <= count <= LONGEST - PROMPT:
        count = 8
    rng = [(seed * 2654435761 ^ 0x9e3779b9) & 0xffffffff or 1]

    def weight():
        state = rng[0]
        state ^= (state << 13) & 0xffffffff
        state ^= state >> 17
        state ^= (state << 5) & 0xffffffff
        rng[0] = state
        return state % 16 - 8

    def matrix(rows, cols):
        return [[weight() for col in range(cols)] for row in range(rows)]

    def dot(row, vector, bias=0):
        total = bias + sum(left * right for left, right in zip(row, vector))
        assert -2**31 <= total < 2**31
        return total

    def mul(weights, vector, bias=None):
        return [dot(row, vector, bias[index] if bias else 0) >> 4
                for index, row in enumerate(weights)]

    def clamp(value):
        return max(-1024, min(1024, value))

    emb, pos = matrix(VOCAB, DIM), matrix(LONGEST, DIM)
    wq, wk, wv, wo = (matrix(DIM, DIM), matrix(DIM, DIM),
                      matrix(DIM, DIM), matrix(DIM, DIM))
    w1, b1 = matrix(HIDDEN, DIM), matrix(1, HIDDEN)[0]
    w2, b2 = matrix(DIM, HIDDEN), matrix(1, DIM)[0]
    un = matrix(VOCAB, DIM)
    tok = [weight() + 8 for index in range(PROMPT)]

    keys, vals, total = [], [], 0
    for step in range(PROMPT + count - 1):
        residual = [left + right for left, right in zip(emb[tok[step]], pos[step])]
        query = mul(wq, residual)
        keys.append(mul(wk, residual))
        vals.append(mul(wv, residual))
        scores = [min(max(dot(query, key) >> 16, 0), 15) + 1 for key in keys]
        attended = [sum(score * value[col] for score, value in zip(scores, vals))
                    for col in range(DIM)]
        attended = [abs(weighted) // sum(scores) * (1 if weighted >= 0 else -1)
                    for weighted in attended]
        residual = [clamp(left + right)
                    for left, right in zip(residual, mul(wo, attended))]
        hidden = [max(value, 0) for value in mul(w1, residual, b1)]
        residual = [clamp(left + right)
                    for left, right in zip(residual, mul(w2, hidden, b2))]
        if step >= PROMPT - 1:
            logits = [dot(row, residual) for row in un]
            total += sum(logits)
            tok.append(logits.index(max(logits)))
    return "prompt = %s\ntokens = %s\nlogits = %d" % (
        " ".join(map(str, tok[:PROMPT])), " ".join(map(str, tok[PROMPT:])), total)


which = sys.argv[1] if len(sys.argv) > 1 else "matmul"
if which == "sum":
    print(sum_ref(int(sys.argv[2]) if len(sys.argv) > 2 else 100))
elif which == "matmul":
    print(matmul_ref(int(sys.argv[2]) if len(sys.argv) > 2 else 24))
elif which == "simd":
    print(simd_ref(int(sys.argv[2]) if len(sys.argv) > 2 else 19))
elif which == "tinytf":
    print(tinytf_ref(int(sys.argv[2]) if len(sys.argv) > 2 else 1,
                     int(sys.argv[3]) if len(sys.argv) > 3 else 8))
else:
    sys.exit("unknown program: " + which)
