#!/usr/bin/env python3
"""prompt-order: where the current time is written decides what is reused.

An agent harness that lets the model know the current time writes it into
every request. Two groups of two consecutive requests, each request a
prompt of prompt_bar.py from X0, the group's name above it, the request's
on the left. The time is a stamp of prompt_bar.py inside a segment: 10:01
in request 1, 10:02 in request 2.
Upper, the time in the last message: system prompt, tool descriptions, then
the question with the time at its end. In request 2 the system prompt and
the tools are reused (the band of K, V, 「复用」), the last message is the
first difference (red outline, 「不同」).
Lower, the time in the system prompt, its centre a third of the way in, as
in "You are .... Current time: 10:01. ...", then the tools and the question.
In request 2 the part of the system prompt before the time is reused, the
time is the first difference, the rest of the system prompt and the tools
are computed again (hatched) although request 1 computed the same text,
and question 2 is new (grey).
The rows end left of the canvas's right edge; save's left centres the
drawing.
"""
from prompt_bar import *

H = 356
UPPER, LOWER = 20, 204             # baseline of each group's name
ROW_UP, PITCH = 14, 52             # name baseline to the first row, row to row
TIMES = ("10:01", "10:02")
TIME_AT = 80                       # centre of the time in the system prompt
STAMP_GAP, STAMP_END = 10, 10      # question label to time, time to segment end
SYS = "系统提示词"


def stamp_width(s):
    return width(s, STAMP_SIZE) + 2 * STAMP_PAD


def system_prompt(x, y, time, second=False):
    """The system prompt with the time written into it. second: the part
    before the time is reused, the time differs, the part after it is
    computed again. Returns (elements, before, time, after) with each part
    as (left, right)."""
    w = WIDTH[SYS]
    t0 = x + TIME_AT - stamp_width(time) / 2
    t1 = t0 + stamp_width(time)
    cx = (t1 + x + w) / 2              # the name stands in the part after the time
    out = [rect(x, y, w, SEG_H, FILL_DATA, DATA, rx=4, width=1.6)]
    if second:
        out += hatch(t1, y + 1, x + w - 1 - t1, SEG_H - 2)
        pw = width(SYS, SIZE) + 14     # the grey plate of segment()
        out.append(rect(cx - pw / 2, y + SEG_H / 2 - 13, pw, 26, FILL_DATA, "none",
                        rx=3, width=0))
        out += band(x, y + SEG_H, t0 - x)
    out.append(text(cx, y + SEG_H / 2 + SIZE * 0.36, SYS, SIZE, INK))
    out += stamp(t0, y, time, diff=second)[0]
    return out, (x, t0), (t0, t1), (t1, x + w)


def question(x, y, name, time, diff=False):
    """The last message: the question, the time at its end. Returns
    (elements, (left, right))."""
    lw = width(name, SIZE)
    w = PAD + lw + STAMP_GAP + stamp_width(time) + STAMP_END
    stroke, sw = (RED, 2.4) if diff else (DATA, 1.6)
    out = [rect(x, y, w, SEG_H, FILL_DATA, stroke, rx=4, width=sw),
           text(x + PAD + lw / 2, y + SEG_H / 2 + SIZE * 0.36, name, SIZE, INK)]
    out += stamp(x + PAD + lw + STAMP_GAP, y, time)[0]
    return out, (x, x + w)


def rows(hy, title):
    """The group's name and its two request names. Returns (elements, y1, y2)."""
    y1 = hy + ROW_UP
    y2 = y1 + PITCH
    out = [label(NAME_X, hy, title, 19, INK, "bold", "start")]
    for y, s in ((y1, "请求 1"), (y2, "请求 2")):
        out.append(label(NAME_X, y + SEG_H / 2 + 6.5, s, 18, INK, anchor="start"))
    return out, y1, y2


def upper():
    """The time in the last message. Returns (elements, right end)."""
    b, y1, y2 = rows(UPPER, "时间写在最后一条消息中")
    els, spans = bar(X0, y1, [SYS, "工具说明"])
    b += els
    qx = spans[-1][1] + GAP
    b += question(qx, y1, "问题 1", TIMES[0])[0]
    els, spans = bar(X0, y2, [SYS, "工具说明"], reused={0, 1})
    b += els
    els, last = question(qx, y2, "问题 2", TIMES[1], diff=True)
    b += els
    lab_y = y2 + SEG_H + BAND_H + 22
    b += under(spans[0][0], spans[1][1], lab_y, "复用")
    b += under(*last, lab_y, "不同", RED)
    return b, last[1]


def lower():
    """The time in the system prompt. Returns (elements, right end)."""
    b, y1, y2 = rows(LOWER, "时间写在系统提示词中")
    tx = X0 + WIDTH[SYS] + GAP
    b += system_prompt(X0, y1, TIMES[0])[0]
    b += bar(tx, y1, ["工具说明", "问题 1"])[0]
    els, before, time, after = system_prompt(X0, y2, TIMES[1], second=True)
    b += els
    els, spans = bar(tx, y2, ["工具说明", "问题 2"], hatched={0})
    b += els
    lab_y = y2 + SEG_H + BAND_H + 22
    b += under(*before, lab_y, "复用")
    b += under(*time, lab_y, "不同", RED)
    b += under(after[0], spans[0][1], lab_y, "缓存未命中，重新计算")
    return b, spans[-1][1]


def main():
    top, top_right = upper()
    bottom, bottom_right = lower()
    right = max(top_right, bottom_right)
    save("prompt-order", top + bottom, H, left=round((NAME_X + right) / 2 - W / 2))


main()
