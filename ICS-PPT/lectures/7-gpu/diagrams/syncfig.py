"""The parts that race.py and fold.py share, so the two animations have one size.

Both draw the halving sum of 8 threads, with 2 threads to a warp: above, the
rows of cache[0..7], one per event; below, one copy of the loop for each of
threads 0 to 3, the line that thread has reached washed orange, with its own
s. A module, not a figure: render.sh does not run it.
"""

import pathlib
import sys
from collections import namedtuple

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_ORANGE, FILL_RED, INK, LINE, MUTED, ORANGE, RED,
                    WHITE, arrow, listing, mono, rect, text)

W, H = 1120, 520
X0, CW, CH = 210, 64, 34
TOP, PITCH = 40, 62
RIGHT = X0 + 8 * CW + 24
CODE_Y = 382
CODE_SIZE, LH = 12.5, 21
COPY_W, COPY_GAP, COPY_LINES = 264, 10, 5
VALUES = [3, 1, 4, 1, 5, 9, 2, 6]
ASSUMPTION = ["假定线程块有 8 个线程，每个 Warp 有 2 个线程：",
              "线程 0、1 属于 Warp 0，线程 2、3 属于 Warp 1"]


def header(assumption=ASSUMPTION):
    """The names of the cells, and the assumption the figure rests on."""
    out = [mono(X0 + t * CW + CW / 2, TOP - 10, f"cache[{t}]", 13, MUTED, anchor="middle")
           for t in range(8)]
    for n, note in enumerate(assumption):
        out.append(text(RIGHT, TOP + 14 + n * 18, note, 13.5, MUTED, anchor="start"))
    return out


Geo = namedtuple("Geo", "x0 cw ch top pitch right")
GEO = Geo(X0, CW, CH, TOP, PITCH, RIGHT)


def table(rows, k, right, result=None, geo=GEO):
    """The first k rows. A row is (step, thread note, values, written cells, stale
    cells, dead cells); a stale cell is read before its writer has run. ``right``
    is the two-line note for the newest row, or None; ``result`` is "bad" (red) or
    "good" (bold) when the note states the final sum. ``geo`` places the table."""
    x0, cw, ch, top, pitch, note_x = geo
    out = []
    for r, (s, note, v, written, stale, dead) in enumerate(rows[:k]):
        y = top + r * pitch
        now = r == k - 1
        if s is None:
            out.append(text(x0 - 20, y + ch * 0.68, note, 16, INK, "bold", "end"))
        else:
            out.append(mono(x0 - 20, y + ch * 0.44, f"s = {s}", 16, INK, "bold", "end"))
            out.append(text(x0 - 20, y + ch * 1.0, note, 14, MUTED, anchor="end"))
            # arrows from the cells of the row above that a written cell adds up
            for t in written:
                for frm, bad in ((t, False), (t + s, t + s in stale)):
                    hue = RED if bad else ORANGE if now else MUTED
                    xa = x0 + frm * cw + cw / 2
                    xb = x0 + t * cw + cw / 2 + (6 if frm != t else 0)
                    out.append(arrow(xa, y - pitch + ch + 2, xb, y - 3, hue, 1.6))
        for t in range(8):
            x = x0 + t * cw
            hot = t in written and now
            late = t in stale and now
            fill = FILL_ORANGE if hot else FILL_RED if late else \
                WHITE if t in dead else FILL_BLUE
            stroke = ORANGE if hot else RED if late else LINE if t in dead else BLUE
            out.append(rect(x, y, cw, ch, fill, stroke, rx=2, width=1.2,
                            dash="4 3" if t in dead else None))
            out.append(mono(x + cw / 2, y + ch * 0.68, str(v[t]), 16,
                            RED if late else MUTED if t in dead else INK,
                            "bold" if hot else "normal", "middle"))
        if now and right:
            out.append(text(note_x, y + ch * 0.44, right[0], 17, RED if result == "bad" else INK,
                            "bold", "start"))
            out.append(text(note_x, y + ch * 1.06, right[1], 17, INK if result else MUTED,
                            "bold" if result else "normal", "start"))
    return out


def copy(x, code, at, s_now, name, warp, waiting=False):
    """One box: its owner and warp, its s, the loop with its line washed."""
    out = [rect(x, CODE_Y - 40, COPY_W, COPY_LINES * LH + 52, WHITE, LINE, rx=6),
           text(x + 12, CODE_Y - 18, name, 14, INK, "bold", "start"),
           text(x + 12 + 14 * (len(name) - 1) + 20, CODE_Y - 18, warp, 13, MUTED,
                anchor="start")]
    if waiting:
        out.append(text(x + 138, CODE_Y - 18, "等待", 13, RED, "bold", "start"))
    out.append(mono(x + COPY_W - 12, CODE_Y - 18, f"s = {s_now}", 14, ORANGE, "bold", "end"))
    return out + listing(x + 12, CODE_Y - 4, code, size=CODE_SIZE, lh=LH,
                         marks={at: FILL_RED if waiting else FILL_ORANGE}, width=COPY_W - 14)


def copies(code, standing):
    """One box per thread. ``standing`` is (line, s, waiting) for threads 0 to 3."""
    out = []
    for t, (at, s_now, waiting) in enumerate(standing):
        out += copy(17 + t * (COPY_W + COPY_GAP), code, at, s_now, f"线程 {t}", f"Warp {t // 2}",
                    waiting)
    return out
