#!/usr/bin/env python3
"""context-overflow: three ways to handle an input longer than the context.

One frame, three rows, one method each, its name on the left. Tokens are the
small chips of context-size, those outside the window faded. The window has
the same width in every row and ends at the newest token, so its left edge is
one orange dashed line through all rows, as on context-size.
1: the earliest tokens fall outside the window and are dropped.
2: the request is longer than the window and is rejected as a whole: its
   chips framed in red, the error below.
3: the earlier part is replaced by a shorter summary (green) at the start of
   the window, followed by the most recent tokens.
"""
from tk import *

H = 324
P, CW, CH = 20, 15, 22      # chip pitch, width, height
NWIN = 24                   # tokens in the window
NOUT = 12                   # tokens before the window, in every row
NSUM = 4                    # pitches the summary takes
WR = 1000                   # right end of the window
WL = WR - (10 + (NWIN - 1) * P + CW)    # left edge of the window
NX = 40                     # left edge of the method names
ROWS = (72, 160, 256)       # top of the chips in each row


def inside(j):
    """Left edge of chip j of the window, 0 the oldest."""
    return WL + 5 + j * P


def outside(k):
    """Left edge of chip k before the window, 1 the nearest."""
    return WL - 5 - CW - (k - 1) * P


def chips(xs, y, faded=False):
    if faded:
        return [rect(x, y, CW, CH, WHITE, LINE, rx=3, width=1.2, dash="2 2") for x in xs]
    return [rect(x, y, CW, CH, FILL_DATA, DATA, rx=3, width=1.2) for x in xs]


def name(y, title, sub=None):
    """The method's name level with its chips, an optional note below."""
    mid = y + CH / 2
    out = [label(NX, mid + 7, title, 19, INK, "bold", "start")]
    if sub:
        out.append(note(NX, mid + 30, sub, 16, "start"))
    return out


def error_box(cx, y, msg, size=17):
    """A red box centred on cx: a cross in a disc, then the message."""
    w = 14 + 18 + 10 + width(msg, size) + 14
    x = cx - w / 2
    ix, iy = x + 14 + 9, y + 15
    return [rect(x, y, w, 30, FILL_RED, RED, rx=6, width=1.6),
            circle(ix, iy, 9, RED, RED),
            line(ix - 4, iy - 4, ix + 4, iy + 4, WHITE, 2),
            line(ix - 4, iy + 4, ix + 4, iy - 4, WHITE, 2),
            label(ix + 19, iy + size * 0.36, msg, size, RED, "bold", "start")]


def main():
    b = []
    left = outside(NOUT)
    before = [outside(k) for k in range(1, NOUT + 1)]
    window = [inside(j) for j in range(NWIN)]
    # time runs left to right, the window covers the newest tokens
    b.append(note(left, 21, "较早", 15, "start"))
    b.append(note(WR, 21, "较新", 15, "end"))
    b.append(arrow(left + 42, 16, WR - 42, 16, MUTED, 1.4))
    b += brace(WL, WR, 64, INK, "上下文长度", size=17, below=False)
    b.append(line(WL, 56, WL, ROWS[2] + CH + 6, ORANGE, 1.6, dash="5 4"))

    # 1: drop the earliest tokens
    y = ROWS[0]
    b += name(y, "舍弃最早的上下文")
    b += chips(before, y, faded=True) + chips(window, y)
    b += brace(left, WL - 5, y + CH + 6, MUTED, "舍弃", size=16, depth=7)

    # 2: reject the whole request
    y = ROWS[1]
    b += name(y, "直接报错", "AI 云服务的 API 常用")
    b += chips(before, y) + chips(window, y)
    b.append(rect(left - 6, y - 6, WR + 1 - (left - 6), CH + 12, "none", RED, rx=6,
                  width=1.6))
    b += error_box((WL + WR) / 2, y + CH + 14, "请求被拒绝：超出上下文长度")

    # 3: replace the earlier part by a summary at the start of the window
    y = ROWS[2]
    b += name(y, "会话总结", "Agent 常用")
    b += chips(before, y, faded=True)
    sw = (NSUM - 1) * P + CW
    sx = inside(0) + sw / 2
    b.append(rect(inside(0), y, sw, CH, FILL_UPD, UPD, rx=3, width=1.4))
    b.append(label(sx, y + CH / 2 + 15 * 0.36, "摘要", 15, UPD, "bold"))
    b += chips(window[NSUM:], y)
    by = y + CH + 6
    b += brace(left, WL - 5, by, UPD, "总结", size=16, depth=7)
    mid = (left + WL - 5) / 2
    ay = by + 7 + 20 - 6
    b.append(elbow([(mid + 26, ay), (sx, ay), (sx, y + CH + 4)], UPD, 1.8))
    save("context-overflow", b, H)


main()
