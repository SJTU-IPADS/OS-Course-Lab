"""The grid of part three: token columns and layer rows.

recompute.py, prefix-invariance.py and kv-cache.py draw one picture on
consecutive pages: the positions of "a fluffy blue creature roamed the" as
columns, layers 1..3 as rows from the bottom up, the token chips under the
columns. query-lifetime.py puts positions 1..8 on the same columns. Every
coordinate of that picture is set here, so the columns, the rows and the
chips stay in place when the page flips.

- Canvas: W x H for every figure on the grid and every frame of it.
- Columns: position i (0-based) is centred at cx(i). A group of vectors at
  one position is centred on cx(i): a hidden state, a k-v pair (k at
  cx - STEP / 2, v at cx + STEP / 2), a q-k-v triple (q at cx - STEP, k at
  cx, v at cx + STEP).
- Rows: layer l is centred at ROW_Y[l]. The chips have their top at
  CHIP_Y; the labels 「第 l 层」 end at LABEL_X (query-lifetime.py puts its
  「第 t 步」 there).
- Legend (prefix-invariance.py, kv-cache.py): right of column 6, one entry
  per line from LEGEND_Y down. The entries q, k, v keep their lines in
  every frame; a frame that has no q leaves its line empty.
- X0 balances the frames: the labels on the left start near x = 85, and
  kv-cache-4, the widest frame, ends near x = 1030.
Not a figure: render.sh skips it.
"""
from tk import *

WORDS = "a fluffy blue creature roamed the".split()

H = 380
PITCH, X0 = 110, 190               # column pitch, centre of position 1
ROW_Y = {1: 292, 2: 188, 3: 84}    # centre of each layer's row
CHIP_Y, CHIP_H, CHIP_SIZE = 344, 34, 18
LABEL_X = X0 - 50                  # right end of 「第 l 层」

CELLS, CH, MW = 4, 11, 13          # a q, k or v vector: 4 cells, 13 px wide
STEP = 18                          # centre to centre of two vectors of one position
HALF = CELLS * CH / 2 + 2          # half height of a vector with its border


def cx(i):
    return X0 + i * PITCH


LEGEND_X = cx(5) + 112
LEGEND_Y = ROW_Y[1] - 26           # baseline of the first entry
QKV = [("q", "查询", Q, FILL_Q), ("k", "键", K, FILL_K), ("v", "值", V, FILL_V)]


def tint(color_hex, t):
    base = tuple(int(color_hex[k:k + 2], 16) for k in (1, 3, 5))
    r, g, b_ = (round(255 + (c - 255) * t) for c in base)
    return f"#{r:02x}{g:02x}{b_:02x}"


def mvec(x, y, seed, color, cells=CELLS, ch=CH, mw=MW, pad=2, width=1.6, dash=None):
    """A column vector of shaded cells centred at (x, y). With dash, only
    the empty outline of its slot."""
    top, left = y - cells * ch / 2, x - mw / 2
    frame = (left - pad, top - pad, mw + 2 * pad, cells * ch + 2 * pad)
    if dash:
        return [rect(*frame, "none", LINE, rx=pad + 1, width=1.4, dash=dash)]
    out = [rect(*frame, WHITE, color, rx=pad + 1, width=width)]
    for k in range(cells):
        t = 0.15 + 0.6 * abs(pseudo(seed, k, 0))
        out.append(rect(left, top + k * ch, mw, ch, tint(color, t), WHITE, rx=0, width=1))
    return out


def layer_labels():
    return [text(LABEL_X, y + 7, f"第 {layer} 层", 19, INK, anchor="end")
            for layer, y in ROW_Y.items()]


def token(i, role="data"):
    return chip(cx(i), CHIP_Y, WORDS[i], role, size=CHIP_SIZE, h=CHIP_H, anchor="middle")[0]


def legend(items, x=LEGEND_X, y=LEGEND_Y):
    """One entry per line, downwards from y; None leaves its line empty."""
    out = []
    for item in items:
        if item:
            sym, name, c, f = item
            out.append(rect(x, y - 15, 16, 18, f, c, rx=3, width=1.6))
            out += math(x + 24, y, sym, 20, c, anchor="start")
            out.append(text(x + 38, y, name, 18, c, anchor="start"))
        y += 32
    return out
