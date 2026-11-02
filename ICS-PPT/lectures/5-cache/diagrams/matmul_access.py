#!/usr/bin/env python3
"""Matrix multiplication C = A B: the definition, and the inner loop of each class.

matmul-def shows what one element of C needs: row i of A and column j of B; the
elements in use are orange, the colour of requested data on every figure.

matmul-ijk-1 ... -3, matmul-kij-1 ... -3 and matmul-jki-1 ... -3 replay the first
eight iterations of the inner loop of each class on a fully associative LRU cache
with 64-byte blocks, and draw iterations 1, 2 and 8. The access stream is generated
from the loop and fed to the cache, so the hit or miss of every reference and the
miss counts come from the simulation. Each matrix is a double array with n = 1024, rows 64-byte aligned; the
drawing is its top-left 8 x 8 corner, one row of 8 doubles being one block. The
cache is empty when the inner loop starts (n is large, so the blocks of an earlier
run of the loop are gone).

A frame draws one iteration: the element requested in each matrix is orange, ringed
green (hit) or red (miss); a block that is in the cache is light blue; the numbers
mark the iterations that already read the element. After eight iterations (one
block of a row) the miss counts are 1, 8, 0 for ijk, 0, 1, 1 for kij and 8, 0, 8
for jki, i.e. 1.125, 0.25 and 2.0 misses per iteration.
Run it to refresh ../assets/matmul-*.svg.
"""

from fractions import Fraction

from cachekit import badge
from cdraw import *

W = 1000
N = 8                                    # cells per side of the corner that is drawn
CW, RH = 30, 18                          # cell width, row height
SIDE_W, SIDE_H = N * CW, N * RH
CENTERS = {"A": 175, "B": 500, "C": 825}
TOP = 34
FRAMES = 8                               # iterations simulated, one block of a row: 8 doubles
SHOWN = (1, 2, FRAMES)                   # the iterations that get a frame

# ---- the cache -----------------------------------------------------------------

BLOCK, ELEM = 64, 8                      # bytes per block, bytes per double
LD = 1024                                # n, the length of a row in doubles
BASE = {"A": 0, "B": LD * LD * ELEM, "C": 2 * LD * LD * ELEM}
CAPACITY = 32                            # blocks; more than the eight iterations use


class Cache:
    """Fully associative, LRU, 64-byte blocks."""

    def __init__(self):
        self.lines = []                  # block numbers, least recently used first

    def access(self, addr):
        blk = addr // BLOCK
        hit = blk in self.lines
        if hit:
            self.lines.remove(blk)
        elif len(self.lines) == CAPACITY:
            self.lines.pop(0)
        self.lines.append(blk)
        return hit

    def holds(self, addr):
        return addr // BLOCK in self.lines


def address(name, row, col):
    return BASE[name] + (row * LD + col) * ELEM


# fixed loop variables of the drawn iterations: i, j, k are 3, 2, 5
I, J, K = 3, 2, 5

# per class: the inner variable, the matrices read in the loop body (name, row, col as
# functions of the inner variable t), and the matrices that live in a register
CLASSES = {
    "ijk": dict(var="k", reads=[("A", lambda t: (I, t)), ("B", lambda t: (t, J))],
                writes=[], reg=("C", "sum 在寄存器中")),
    "kij": dict(var="j", reads=[("B", lambda t: (K, t)), ("C", lambda t: (I, t))],
                writes=["C"], reg=("A", "r 在寄存器中")),
    "jki": dict(var="i", reads=[("A", lambda t: (t, K)), ("C", lambda t: (t, J))],
                writes=["C"], reg=("B", "r 在寄存器中")),
}
LABELS = {"A": "A[i][k]", "B": "B[k][j]", "C": "C[i][j]"}


def simulate(cls):
    """Run the first FRAMES iterations; returns one record per iteration.

    A record: {'at': {name: (row, col)}, 'hit': {name: bool}, 'held': {name: set of rows}}.
    'hit' is the result of the read; a write to the element just read must hit.
    """
    spec = CLASSES[cls]
    cache = Cache()
    out = []
    for t in range(FRAMES):
        at, hit = {}, {}
        for name, pos in spec["reads"]:
            row, col = pos(t)
            at[name] = (row, col)
            hit[name] = cache.access(address(name, row, col))
            if name in spec["writes"]:
                assert cache.access(address(name, row, col)), "the write follows the read"
        held = {name: {r for r in range(N) if cache.holds(address(name, r, 0))}
                for name in "ABC"}
        out.append(dict(at=at, hit=hit, held=held))
    return out


def misses(records, upto, name):
    return sum(1 for rec in records[:upto] if name in rec["hit"] and not rec["hit"][name])


# ---- drawing -------------------------------------------------------------------

def grid(name, x, rec, upto, records, reg_note=None):
    """One matrix corner. rec is the record of the current iteration."""
    out = [text(x + SIDE_W / 2, TOP - 12, LABELS[name], 19, INK, "bold")]
    visited = {}
    for t, past in enumerate(records[:upto]):
        if name in past["at"]:
            visited[past["at"][name]] = t
    held = rec["held"][name]
    now = rec["at"].get(name)
    for r in range(N):
        for c in range(N):
            x0, y0 = x + c * CW, TOP + r * RH
            if (r, c) == now:
                out.append(rect(x0, y0, CW, RH, FILL_ORANGE, ORANGE, rx=0, width=2))
                out.append(text(x0 + CW / 2, y0 + RH / 2 + 5, str(visited[(r, c)]), 14, INK,
                                "bold", font=MONO))
                continue
            fill = FILL_BLUE if r in held else WHITE
            out.append(rect(x0, y0, CW, RH, fill, LINE, rx=0, width=1))
            if (r, c) in visited:
                out.append(text(x0 + CW / 2, y0 + RH / 2 + 5, str(visited[(r, c)]), 13, MUTED,
                                font=MONO))
    for r in range(N):                                   # each row is one block
        out.append(rect(x, TOP + r * RH, SIDE_W, RH, "none", MUTED, rx=0, width=1.6))
    out.append(rect(x, TOP, SIDE_W, SIDE_H, "none", INK, rx=0, width=2.2))
    if now is not None:
        r, c = now
        ring = GREEN if rec["hit"][name] else RED
        out.append(rect(x + c * CW - 2, TOP + r * RH - 2, CW + 4, RH + 4, "none", ring, rx=0,
                        width=3))
    if reg_note:
        cx, cy = x + SIDE_W / 2, TOP + SIDE_H / 2
        out.append(rect(cx - 82, cy - 15, 164, 30, WHITE, MUTED, rx=15, width=1.4))
        out.append(text(cx, cy + 6, reg_note, 16, MUTED, "bold"))
    return out


def result(name, x, rec, upto, records):
    """The row under a matrix: this iteration's result and the count so far."""
    cx = x + SIDE_W / 2
    y = TOP + SIDE_H + 25
    m = misses(records, upto, name)
    colour = RED if m else GREEN
    if name in rec["hit"]:
        out = badge(cx - 52, y, "hit" if rec["hit"][name] else "miss",
                    "hit" if rec["hit"][name] else "miss", size=16, w=66)
        out.append(text(cx + 22, y + 6, f"累计 miss {m}", 17, colour, "bold", "start"))
    else:
        out = [text(cx, y + 6, f"累计 miss {m}", 17, colour, "bold")]
    return out


def status(cls, n, records):
    """The sentence under the figure: what iteration n did."""
    spec = CLASSES[cls]
    rec = records[n - 1]
    var = spec["var"]
    if n == FRAMES:
        parts = [f"{name} miss {misses(records, FRAMES, name)} 次" for name in "ABC"
                 if name in rec["hit"]]
        return f"{FRAMES} 次迭代：" + "，".join(parts)
    parts = []
    for name, _ in spec["reads"]:
        row, col = rec["at"][name]
        ref = {"A": f"A[{row if var == 'i' else 'i'}][{col if var == 'k' else 'k'}]",
               "B": f"B[{row if var == 'k' else 'k'}][{col if var == 'j' else 'j'}]",
               "C": f"C[{row if var == 'i' else 'i'}][{col if var == 'j' else 'j'}]"}[name]
        parts.append(f"{ref} " + ("在已取入的块中，hit" if rec["hit"][name]
                                  else "miss，取入它所在的块"))
    return f"{var} = {n - 1}：" + "。".join(parts)


def frame(cls, n):
    records = simulate(cls)
    rec = records[n - 1]
    spec = CLASSES[cls]
    out = []
    for name in "ABC":
        x = CENTERS[name] - SIDE_W / 2
        reg = spec["reg"][1] if spec["reg"][0] == name else None
        out += grid(name, x, rec, n, records, reg)
        out += result(name, x, rec, n, records)
    out.append(text(W / 2, TOP + SIDE_H + 64, status(cls, n, records), 19, INK, "bold"))
    out += legend(TOP + SIDE_H + 94)
    return out


def legend(y):
    """What the fills mean, one line."""
    out = [rect(150, y - 14, 20, 16, FILL_ORANGE, ORANGE, rx=0, width=1.6),
           text(178, y, "当前访问的元素", 15, MUTED, anchor="start"),
           rect(330, y - 14, 20, 16, FILL_BLUE, LINE, rx=0, width=1.6),
           text(358, y, "已在缓存中的块", 15, MUTED, anchor="start"),
           text(500, y, "数字：访问该元素的迭代", 15, MUTED, anchor="start"),
           text(720, y, "每行 8 个 double 是一个 64 字节的块", 15, MUTED, anchor="start")]
    return out


def totals(cls):
    """Misses per iteration of each matrix and in all, from the simulation."""
    records = simulate(cls)
    per = {name: Fraction(misses(records, FRAMES, name), FRAMES) for name in "ABC"}
    return per, sum(per.values())


# ---- the definition ------------------------------------------------------------

DEF_CW = 26


def definition():
    side = N * DEF_CW
    xs = {"A": 70, "B": 396, "C": 722}
    top = 50
    hot = {"A": lambda r, c: r == 3, "B": lambda r, c: c == 4, "C": lambda r, c: (r, c) == (3, 4)}
    body = []
    for name in "ABC":
        x = xs[name]
        body.append(text(x + side / 2, top - 18, name, 22, INK, "bold"))
        for r in range(N):
            for c in range(N):
                on = hot[name](r, c)
                body.append(rect(x + c * DEF_CW, top + r * DEF_CW, DEF_CW, DEF_CW,
                                 FILL_ORANGE if on else WHITE, ORANGE if on else LINE, rx=0,
                                 width=1.8 if on else 1))
        body.append(rect(x, top, side, side, "none", INK, rx=0, width=2.2))
    y = top + 3 * DEF_CW + DEF_CW / 2
    body.append(arrow(xs["A"] + DEF_CW / 2, y, xs["A"] + side - DEF_CW / 2, y, INK, 2.4))
    cx = xs["B"] + 4 * DEF_CW + DEF_CW / 2
    body.append(arrow(cx, top + DEF_CW / 2, cx, top + side - DEF_CW / 2, INK, 2.4))
    body.append(text((xs["A"] + side + xs["B"]) / 2, top + side / 2 + 12, "×", 40, INK, "bold"))
    body.append(text((xs["B"] + side + xs["C"]) / 2, top + side / 2 + 12, "=", 40, INK, "bold"))
    y = top + side + 34
    body.append(text(xs["A"] + side / 2, y, "第 i 行", 20, INK, "bold"))
    body.append(text(xs["B"] + side / 2, y, "第 j 列", 20, INK, "bold"))
    body.append(text(xs["C"] + side / 2, y, "第 i 行第 j 列的元素", 20, INK, "bold"))
    body.append(text(xs["A"] + side / 2, y + 28, "n 个元素", 17, MUTED))
    body.append(text(xs["B"] + side / 2, y + 28, "n 个元素", 17, MUTED))
    body.append(text(xs["C"] + side / 2, y + 28, "n 个乘积之和", 17, MUTED))
    save("matmul-def", W, y + 48, body)


if __name__ == "__main__":
    definition()
    height = TOP + SIDE_H + 112
    for cls in CLASSES:
        for shown, n in enumerate(SHOWN, 1):
            save(f"matmul-{cls}-{shown}", W, height, frame(cls, n))
        per, total = totals(cls)
        print(cls, {k: str(v) for k, v in per.items()}, "total", total, float(total))
