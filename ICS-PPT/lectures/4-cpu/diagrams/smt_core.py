#!/usr/bin/env python3
"""A core with hyper-threading: two copies of the thread state, one set of the rest.

Three bands. Top and bottom: thread A (blue) and thread B (orange), each a PC
and a register file holding the same register names, the copied hardware.
Middle: the hardware both threads share, the stages of the two-wide
out-of-order core of the superscalar and out-of-order pages, left to right:
fetch, decode, the decoded instructions waiting to execute, two execution
units, memory with the results waiting to write back under it, write-back.
Fetch takes its address from PC A or PC B; decode reads, and write-back
writes, the register file of the instruction's own thread.

smt-hw.svg is the structure, every box empty and a short line in the large
ones saying what it does for the two threads. smt-run.svg is the same canvas
in one cycle of the two threads' run, each instruction a chip tagged with its
thread. The run, under the rules of those pages (2 instructions fetched per
cycle, A and B taking turns from cycle 1; decode the next cycle; two
execution units, the oldest ready instruction first, a thread ignored; a
memory read three cycles; write-back in each thread's program order, two per
cycle at most), both threads starting from movl (%rdi,%r8,4),%r10d:

  cycle 8  fetch      B movslq, B movl (%rsi,%r8,4),%ecx
           decode     A movslq, A movl (%rsi,%r8,4),%ecx
           waiting    B addl %ecx,%r9d (its product), A jl (ready: units full),
                      B cmpl (ready: units full), B jl (the flags)
           execute    B imull (w[i] read in cycles 5-7), A addl %ecx,%r9d
                      (A's product, executed in cycle 7)
           memory     A imull, A cmpl
           waiting to write back   A addl $1,%eax, B addl $1,%eax
           write-back B movl (%rdi,%r8,4),%r10d into B's %r10

The cycle is named under the shared band's label. The PC B arrow and B's
write arrow are drawn in B's colour, with B's %r10 cell; the other arrows
stay grey.
Run it to refresh ../assets/smt-hw.svg and smt-run.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_ORANGE, INK, LINE, MONO, MUTED, ORANGE, WHITE,
                    arrow, line, rect, save, text)

W, H = 1120, 356
LEFT = -8                           # centres the ink (x 16..1072) on the canvas
COLS = {                            # box -> (x, width)
    "取指": (150, 125), "译码": (294, 125), "等待": (438, 210),
    "执行": (667, 115), "访存": (801, 115), "写回": (935, 125),
}
SY, SH = 98, 160                    # the shared band's boxes
SPLIT = 14                          # gap between two stacked boxes
HALF = (SH - SPLIT) / 2
BANDS = {"A": (6, BLUE, FILL_BLUE), "B": (SY + SH + 34, ORANGE, FILL_ORANGE)}
BAND_H = 58
REGS = ["%rax", "%rcx", "%rdx", "%rsi", "%rdi", "%r8", "%r9", "%r10"]
CHIP_H, CHIP_STEP = 20, 23

RUN = {                             # box -> chips (thread, name), cycle 8
    "取指": [("B", "movslq"), ("B", "movl (%rsi…)")],
    "译码": [("A", "movslq"), ("A", "movl (%rsi…)")],
    "等待": [("B", "addl %ecx"), ("A", "jl"), ("B", "cmpl"), ("B", "jl")],
    "执行1": [("B", "imull")],
    "执行2": [("A", "addl %ecx")],
    "访存": [("A", "imull"), ("A", "cmpl")],
    "等待写回": [("A", "addl $1"), ("B", "addl $1")],
    "写回": [("B", "movl (%rdi…)")],
}
NOTES = {                           # box -> what it does for the two threads (smt-hw)
    "取指": ["按 PC A、PC B", "轮流取指"],
    "译码": ["读本线程的", "寄存器堆"],
    "等待": ["两个线程的指令", "带着线程号等待"],
    "写回": ["写进本线程的", "寄存器堆"],
}


def mid(name):
    x, w = COLS[name]
    return x + w / 2


def boxes():
    """The shared boxes: name -> (x, y, w, h, title)."""
    out = {}
    for name in ("取指", "译码", "写回"):
        x, w = COLS[name]
        out[name] = (x, SY, w, SH, {"取指": "取指", "译码": "译码", "写回": "写回"}[name])
    x, w = COLS["等待"]
    out["等待"] = (x, SY, w, SH, "等待执行的指令")
    x, w = COLS["执行"]
    out["执行1"] = (x, SY, w, HALF, "执行部件 1")
    out["执行2"] = (x, SY + HALF + SPLIT, w, HALF, "执行部件 2")
    x, w = COLS["访存"]
    out["访存"] = (x, SY, w, HALF, "访存")
    out["等待写回"] = (x, SY + HALF + SPLIT, w, HALF, "等待写回")
    return out


def chip(x, y, w, who, name):
    color, fill = BANDS[who][1], BANDS[who][2]
    return [rect(x, y, w, CHIP_H, fill, color, rx=3, width=1.4),
            text(x + 6, y + 15, who, 12, color, "bold", anchor="start", font=MONO),
            text(x + 19, y + 15, name, 12, INK, anchor="start", font=MONO)]


def band(who, run):
    top, color, fill = BANDS[who]
    out = [rect(16, top, 1056, BAND_H, fill, color, rx=8, width=1.4, dash="6 4"),
           text(24, top + 26, f"线程 {who}", 17, color, "bold", anchor="start"),
           text(24, top + 46, "每个线程一份", 13, MUTED, anchor="start")]
    x, w = COLS["取指"]
    out += [rect(x, top + 8, w, BAND_H - 16, WHITE, color, rx=5, width=1.6),
            text(x + w / 2, top + BAND_H / 2 + 6, "PC", 17, color, "bold")]
    x0 = COLS["译码"][0]
    x1 = sum(COLS["写回"])
    out += [rect(x0, top + 6, x1 - x0, BAND_H - 12, WHITE, color, rx=5, width=1.6),
            text(x0 + 14, top + BAND_H / 2 + 6, "寄存器堆", 16, color, "bold", anchor="start")]
    for k, reg in enumerate(REGS):
        rx = x0 + 104 + k * 64
        lit = run and who == "B" and reg == "%r10"
        out += [rect(rx, top + 14, 58, BAND_H - 28, FILL_ORANGE if lit else WHITE,
                     color if lit else LINE, rx=3, width=2.2 if lit else 1.2),
                text(rx + 29, top + BAND_H / 2 + 5, reg, 13, INK, "bold" if lit else "normal",
                     font=MONO)]
    out.append(text(x0 + 104 + len(REGS) * 64 + 14, top + BAND_H / 2 + 5, "…", 15, MUTED))
    return out


def links(run):
    """PC -> fetch, register file -> decode (读), write-back -> register file (写)."""
    out = []
    a_bot = BANDS["A"][0] + BAND_H - 6
    b_top = BANDS["B"][0] + 6
    fx, dx, wx = mid("取指"), mid("译码"), mid("写回")
    pa, pb = BANDS["A"][0] + BAND_H - 8, BANDS["B"][0] + 8
    out.append(arrow(fx, pa, fx, SY, MUTED if run else BLUE, 2))
    out.append(arrow(fx, pb, fx, SY + SH, ORANGE if run else ORANGE, 2.8 if run else 2))
    if run:
        out.append(text(fx + 10, SY + SH + 26, "按 PC B 取指", 13, ORANGE, "bold",
                        anchor="start"))
    out.append(arrow(dx, a_bot, dx, SY, MUTED, 1.8))
    out.append(text(dx + 8, SY - 16, "读", 13, MUTED, anchor="start"))
    out.append(arrow(dx, b_top, dx, SY + SH, MUTED, 1.8))
    out.append(text(dx + 8, SY + SH + 26, "读", 13, MUTED, anchor="start"))
    out.append(arrow(wx, SY, wx, a_bot, MUTED, 1.8))
    out.append(text(wx + 8, SY - 16, "写", 13, MUTED, anchor="start"))
    out.append(arrow(wx, SY + SH, wx, b_top, ORANGE if run else MUTED, 2.8 if run else 1.8))
    out.append(text(wx + 8, SY + SH + 26, "写 %r10d" if run else "写", 13,
                    ORANGE if run else MUTED, "bold" if run else "normal", anchor="start"))
    return out


def flow():
    """The arrows between the shared boxes."""
    b = boxes()
    out = []
    y = SY + SH / 2
    for a, c in (("取指", "译码"), ("译码", "等待")):
        out.append(arrow(b[a][0] + b[a][2], y, b[c][0], y, MUTED, 1.8))
    wx = b["等待"][0] + b["等待"][2]
    for unit in ("执行1", "执行2"):
        uy = b[unit][1] + HALF / 2
        out.append(arrow(wx, uy, b[unit][0], uy, MUTED, 1.8))
    ex = b["执行1"][0] + b["执行1"][2]
    jx = (ex + b["访存"][0]) / 2
    y1, y2 = SY + HALF / 2, b["执行2"][1] + HALF / 2
    out.append(arrow(ex, y1, b["访存"][0], y1, MUTED, 1.8))
    out.append(line(ex, y2, jx, y2, MUTED, 1.8))
    out.append(line(jx, y2, jx, y1, MUTED, 1.8))
    mx = b["访存"][0] + b["访存"][2] / 2
    out.append(arrow(mx, SY + HALF, mx, SY + HALF + SPLIT, MUTED, 1.6))
    rx = b["访存"][0] + b["访存"][2]
    out.append(arrow(rx, y1, b["写回"][0], y1, MUTED, 1.8))
    out.append(arrow(rx, y2, b["写回"][0], y2, MUTED, 1.8))
    return out


def shared(run):
    out = [rect(16, SY - 10, 1056, SH + 20, "none", MUTED, rx=8, width=1.2, dash="6 4"),
           text(24, SY + SH / 2 - 4, "两个线程", 16, INK, "bold", anchor="start"),
           text(24, SY + SH / 2 + 18, "共用", 16, INK, "bold", anchor="start")]
    if run:
        out.append(text(24, SY + SH / 2 + 50, "周期 8", 16, ORANGE, "bold", anchor="start"))
    for name, (x, y, w, h, title) in boxes().items():
        dash = "5 4" if name == "等待写回" else None
        out += [rect(x, y, w, h, WHITE, INK, rx=5, width=1.4, dash=dash),
                text(x + w / 2, y + 20, title, 15, INK, "bold")]
        if run:
            chips = RUN.get(name, [])
            cols = 2 if name == "等待" else 1
            cw = (w - 12 - (cols - 1) * 6) / cols
            for k, (who, s) in enumerate(chips):
                cx = x + 6 + (k % cols) * (cw + 6)
                cy = y + 28 + (k // cols) * CHIP_STEP
                out += chip(cx, cy, cw, who, s)
        elif name in NOTES:
            for k, s in enumerate(NOTES[name]):
                out.append(text(x + w / 2, y + h / 2 + 4 + k * 20, s, 13, MUTED))
    return out


def build(run):
    return band("A", run) + band("B", run) + shared(run) + flow() + links(run)


if __name__ == "__main__":
    save("smt-hw", W, H, build(False), left=LEFT)
    save("smt-run", W, H, build(True), left=LEFT)
