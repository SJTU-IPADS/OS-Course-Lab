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


def sum_ref(n):
    s = 0
    for i in range(1, n + 1):
        s += i
    return s


def matmul_ref(n):
    a = [[i + 2 * j for j in range(n)] for i in range(n)]
    b = [[3 * i - j + 1 for j in range(n)] for i in range(n)]
    c = [[sum(a[i][k] * b[k][j] for k in range(n)) for j in range(n)]
         for i in range(n)]
    return sum(sum(row) for row in c)


def simd_ref(n):
    s = 5 + sum((i % 7 - 3) * (2 * i + 1) for i in range(n))
    return (s + 2**31) % 2**32 - 2**31          # an int in the guest


def tinytf_ref(seed, n):
    """guest/tinytf: one attention layer and one MLP in integers. Written
    from its description: weights from xorshift32, products scaled by >> 4,
    activations clamped to [-1024, 1024], attention weights
    min(max(q.k >> 16, 0), 15) + 1, the average truncated toward zero."""
    V, D, H, P = 16, 16, 36, 4
    T = P + 12
    if not 1 <= n <= T - P:
        n = 8
    st = [(seed * 2654435761 ^ 0x9e3779b9) & 0xffffffff or 1]

    def weight():
        r = st[0]
        r ^= (r << 13) & 0xffffffff
        r ^= r >> 17
        r ^= (r << 5) & 0xffffffff
        st[0] = r
        return r % 16 - 8

    def matrix(r, c):
        return [[weight() for _ in range(c)] for _ in range(r)]

    def dot(w, x, b=0):
        s = b + sum(p * q for p, q in zip(w, x))
        assert -2**31 <= s < 2**31
        return s

    def mul(w, x, b=None):
        return [dot(row, x, b[i] if b else 0) >> 4 for i, row in enumerate(w)]

    def clamp(v):
        return max(-1024, min(1024, v))

    emb, pos = matrix(V, D), matrix(T, D)
    wq, wk, wv, wo = matrix(D, D), matrix(D, D), matrix(D, D), matrix(D, D)
    w1, b1 = matrix(H, D), matrix(1, H)[0]
    w2, b2 = matrix(D, H), matrix(1, D)[0]
    un = matrix(V, D)
    tok = [weight() + 8 for _ in range(P)]

    keys, vals, total = [], [], 0
    for t in range(P + n - 1):
        x = [e + p for e, p in zip(emb[tok[t]], pos[t])]
        q = mul(wq, x)
        keys.append(mul(wk, x))
        vals.append(mul(wv, x))
        es = [min(max(dot(q, k) >> 16, 0), 15) + 1 for k in keys]
        a = [sum(e * v[j] for e, v in zip(es, vals)) for j in range(D)]
        a = [abs(s) // sum(es) * (1 if s >= 0 else -1) for s in a]
        x = [clamp(p + q) for p, q in zip(x, mul(wo, a))]
        h = [max(v, 0) for v in mul(w1, x, b1)]
        x = [clamp(p + q) for p, q in zip(x, mul(w2, h, b2))]
        if t >= P - 1:
            logits = [dot(u, x) for u in un]
            total += sum(logits)
            tok.append(logits.index(max(logits)))
    return "prompt = %s\ntokens = %s\nlogits = %d" % (
        " ".join(map(str, tok[:P])), " ".join(map(str, tok[P:])), total)


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
