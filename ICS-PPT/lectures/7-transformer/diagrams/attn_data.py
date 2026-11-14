"""The attention example shared by the figures of part two.

"a fluffy blue creature roamed the verdant forest", one head that lets
adjectives update the nouns after them. SCORES[k][q] is the dot product of
the key of word k with the query of word q (keys are rows, queries columns,
as in the video's grid). The numbers are set by hand for the example: the
noun queries match the adjective keys, every other pair stays small, and the
creature column normalises to fluffy 0.42, blue 0.58, the weights the video
shows, with or without the causal mask. (The video's raw scores are about
100 times larger and unscaled; the small numbers here keep the cells
readable.)
"""
import math

WORDS = "a fluffy blue creature roamed the verdant forest".split()
N = len(WORDS)
CREATURE, FOREST = 3, 7


def _filler(k, q):
    return ((k * 5 + q * 3) % 7 - 3) * 0.3


def _score(k, q):
    if q == CREATURE:
        return {0: -2.5, 1: 4.0, 2: 4.32, 3: -1.0, 4: -2.4, 5: -3.1, 6: -2.0, 7: -2.6}[k]
    if q == FOREST:
        return {0: -0.9, 1: 0.6, 2: 0.2, 3: -1.1, 4: -0.4, 5: -0.5, 6: 4.6, 7: 0.5}[k]
    return _filler(k, q)


SCORES = [[_score(k, q) for q in range(N)] for k in range(N)]


def softmax(xs):
    m = max(xs)
    es = [math.exp(x - m) if x != -math.inf else 0.0 for x in xs]
    s = sum(es)
    return [e / s for e in es]


def column(q, causal=True):
    """The weights of query q over all keys, masked when causal."""
    xs = [SCORES[k][q] if (k <= q or not causal) else -math.inf for k in range(N)]
    return softmax(xs)


def pattern(causal=True):
    """WEIGHTS[k][q]: each column sums to one."""
    cols = [column(q, causal) for q in range(N)]
    return [[cols[q][k] for q in range(N)] for k in range(N)]


if __name__ == "__main__":
    for q in range(N):
        assert abs(sum(column(q)) - 1) < 1e-12
        assert all(w == 0 for w in column(q)[q + 1:])
    print([round(w, 3) for w in column(CREATURE)])
    print([round(w, 3) for w in column(FOREST)])
