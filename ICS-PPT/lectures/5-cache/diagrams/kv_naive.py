#!/usr/bin/env python3
"""Decoding six tokens without a KV cache, one frame per step.

Row s of the grid is step s: the model takes the s tokens so far and needs the k and v of
every one of them. Each cell is one k, v projection. The cell of the new token is orange,
the cells of tokens whose k, v were already computed in an earlier step are red (computed
again). The panel on the right counts them; the numbers come from the loop below.
kv_cache.py imports the geometry and draws the same six steps with a cache.
Run it to refresh ../assets/kv-naive-1.svg ... kv-naive-6.svg.
"""

from kv_kit import *

W, H = 1040, 372
T = 6                                    # tokens generated
GX, CW, PITCH = 96, 74, 82               # grid: left edge, cell width, column pitch
RY, RH, RPITCH = 84, 32, 38              # first row, row height, row pitch
HEAD_Y = 24                              # token boxes above the columns
CAPTION_Y, LEGEND_Y = 334, 362


def col_x(i):                            # i = 1 .. T
    return GX + (i - 1) * PITCH


def row_y(s):
    return RY + (s - 1) * RPITCH


def header(s, out):
    """The tokens x1 .. xT; those not generated yet are dashed, the current one orange."""
    out.append(text(GX - 14, HEAD_Y + 22, "token", 15, MUTED, "bold", "end"))
    for i in range(1, T + 1):
        kind = "new" if i == s else ("cache" if i < s else "empty")
        out += cell(col_x(i), HEAD_Y, CW, 32, f"x{i}", kind, size=16)


def stats(s):
    total = s * (s + 1) // 2
    waste = s * (s - 1) // 2
    x0, x1 = 650, 1010
    out = [text(x0, RY + 14, f"第 {s} 步", 22, INK, "bold", "start")]
    rows = [("本步 k、v 投影", f"{s} 次", INK),
            ("其中重复", f"{s - 1} 次", RED),
            ("累计投影", f"{total} 次", INK),
            ("其中必要", f"{s} 次", ORANGE),
            ("累计重复", f"{waste} 次", RED)]
    for k, (label, value, color) in enumerate(rows):
        y = RY + 54 + k * 38
        out.append(text(x0, y, label, 17, MUTED, "normal", "start"))
        out.append(text(x1, y, value, 22, color, "bold", "end"))
        out.append(line(x0, y + 12, x1, y + 12, LINE, 1))
    return out


def frame(s):
    out = []
    header(s, out)
    for r in range(1, s + 1):
        y = row_y(r)
        out.append(text(GX - 14, y + RH / 2 + 5, f"第 {r} 步", 16, INK if r == s else MUTED,
                        "bold" if r == s else "normal", "end"))
        for i in range(1, r + 1):
            out += cell(col_x(i), y, CW, RH, f"k{i} v{i}", "new" if i == r else "waste",
                        size=15)
    out += stats(s)
    old = "、".join(f"k{i} v{i}" for i in range(1, s))
    cap = (f"第 {s} 步：输入 x{s}，重新计算 {old}，再算 k{s} v{s}" if s > 1
           else "第 1 步：输入 x1，计算 k1 v1")
    out.append(text(W / 2, CAPTION_Y, cap, 20, INK, "bold"))
    out += legend(250, LEGEND_Y, [("new", "新 token 的 k、v"), ("waste", "重复计算")])
    return out


if __name__ == "__main__":
    for s in range(1, T + 1):
        save(f"kv-naive-{s}", W, H, frame(s))
