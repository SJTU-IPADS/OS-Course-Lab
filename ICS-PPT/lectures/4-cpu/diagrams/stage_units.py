#!/usr/bin/env python3
"""The hardware each of the five stages uses, followed by one movslq.

Drawn after the SEQ hardware figure of CS:APP (ICS1 lecture 14, "指令执行阶段"):
stages stacked bottom-up from fetch to write-back, one band each; the line on
the left carries every stage's output upward, and write-back runs from the top
down the right side into the register file's write port. The register file
sits in the decode band with its read and write ports drawn apart; the flags
(CF, ZF, SF, OF) sit beside the ALU, which sets them. Fetch holds
PC, the instruction memory and the adder for PC + instruction length (lecture 3:
%rip <- %rip + 指令长度); the jump target is left out, part 2 covers it.
The instruction is the first one of lecture 3's loop body, movslq %eax, %r8
at 0x20 (3 bytes, 4c 63 c0, objdump -d dot.o of gcc -Og), with %eax = 5.
Frame k lights stage k in the time-space colours (timegrid.TONE) and frames
its band together with what that stage does to movslq on the right, the text
centred on the band; the other hardware is grey; movslq sets no flag, so the
flags stay grey. A lit stage also colours its stretch of the line on the left.
movslq does not touch memory: in the memory frame the data memory stays grey
and only the line through the band turns green.
clock.py draws the next page on the same hardware.
Run it to refresh ../assets/stage-units-1.svg .. stage-units-5.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FONT, INK, LINE, MONO, MUTED, WHITE, arrow, elbow, esc, line,
                    rect, save, text)
from timegrid import TONE

W, H = 1120, 498
BUS = 140                     # the upward line on the left
UX = 236                      # left edge of the hardware column
WX = 720                      # the write-back path comes down here
PX = 800                      # the text on the right
BANDS = {                     # stage -> (top, bottom), write-back on top
    "写回": (8, 76),
    "访存": (76, 164),
    "执行": (164, 252),
    "译码": (252, 354),
    "取指": (354, 490),
}
ORDER = ["取指", "译码", "执行", "访存", "写回"]
PANEL = {                     # stage -> (heading, lines); `code` is monospace
    "取指": ("取指：PC、指令内存", [
        "指令内存按 PC = `0x20` 读出 3 字节",
        "`4c 63 c0`，即 `movslq %eax, %r8`；",
        "PC + 3 = `0x23`，下一条指令的地址"]),
    "译码": ("译码：寄存器堆的读端口", [
        "识别操作：32 位符号扩展为 64 位；",
        "读端口读出 `%eax` = `0x00000005`"]),
    "执行": ("执行：ALU", [
        "ALU 把 `0x00000005` 符号扩展为",
        "`0x0000000000000005`"]),
    "访存": ("访存：数据内存", [
        "`movslq` 不访问内存，数据内存不工作；",
        "执行的结果直接通过"]),
    "写回": ("写回：寄存器堆的写端口", [
        "`%r8` ← `0x0000000000000005`"]),
}


def rich(x, y, s, size=16, fill=INK, weight="normal"):
    """One left-aligned line in which the `code` runs are monospace tspans,
    so the browser places each run right after the previous one."""
    body = "".join(f'<tspan font-family="{MONO}">{esc(piece)}</tspan>' if k % 2 else esc(piece)
                   for k, piece in enumerate(s.split("`")) if piece)
    return [f'<text x="{x:.1f}" y="{y:.1f}" font-family="{FONT}" font-size="{size}" '
            f'font-weight="{weight}" fill="{fill}" text-anchor="start" '
            f'xml:space="preserve">{body}</text>']


def tone(stage, lit):
    """(fill, stroke, text) of a box and the colour of its paths."""
    if stage not in lit:
        return WHITE, LINE, MUTED, LINE
    fill, stroke, _ = TONE[stage]
    return fill, stroke, WHITE if stage == "写回" else INK, stroke


def unit(x, y, w, h, label, stage, lit, size=17):
    fill, stroke, ink, _ = tone(stage, lit)
    return [rect(x, y, w, h, fill, stroke, rx=6, width=1.6),
            text(x + w / 2, y + h / 2 + size * 0.36, label, size, ink, "bold")]


def to_bus(x, y, label, stage, lit):
    """Output: from a unit's left side into the bus, labelled above."""
    c = tone(stage, lit)[3]
    on = stage in lit
    return [arrow(x, y, BUS + 3, y, c, 2),
            text((BUS + x) / 2, y - 7, label, 15, c if on else MUTED, "bold" if on else "normal")]


def from_bus(x, y, stage, lit):
    """Input: from the bus into a unit's left side."""
    return [arrow(BUS + 2, y, x, y, tone(stage, lit)[3], 2)]


def hardware(lit, pc):
    """The hardware with the stages in `lit` coloured; the PC box shows `pc`."""
    out = []
    for stage, (top, bottom) in BANDS.items():
        if bottom < BANDS["取指"][1]:
            out.append(line(24, bottom, 760, bottom, LINE, 1.2, "4 4"))
        mid = (top + bottom) / 2
        if stage in lit:
            out.append(text(28, mid + 7, stage, 20, TONE[stage][1], "bold", anchor="start"))
        else:
            out.append(text(28, mid + 6, stage, 17, MUTED, anchor="start"))
    out.append(line(BUS, 398, BUS, BANDS["写回"][1], LINE, 3))
    for stage in ORDER[:4]:                          # a lit stage's stretch of the line
        if stage in lit:
            top, bottom = BANDS[stage]
            out.append(line(BUS, min(bottom, 398), BUS, top, TONE[stage][1], 3))

    # fetch: PC, instruction memory, PC + instruction length
    out += unit(UX, 370, 180, 42, "指令内存", "取指", lit)
    out += unit(476, 370, 140, 42, "PC + 指令长度", "取指", lit, 16)
    out += unit(286, 436, 120, 36, f"PC = {pc}", "取指", lit, 16)
    c = tone("取指", lit)[3]
    out.append(arrow(346, 436, 346, 413, c, 2))
    out.append(elbow([(406, 454), (546, 454), (546, 413)], c, 2))
    out.append(elbow([(616, 391), (660, 391), (660, 482), (256, 482), (256, 454),
                      (284, 454)], c, 2))
    out += to_bus(UX, 391, "指令", "取指", lit)

    # decode and write-back: one register file, read and write ports apart
    rf = bool(lit & {"译码", "写回"})
    out.append(rect(UX, 260, 404, 82, WHITE, INK if rf else LINE, rx=6, width=1.6))
    out.append(text(UX + 202, 279, "寄存器堆", 16, INK if rf else MUTED, "bold"))
    out += unit(248, 288, 180, 42, "读端口", "译码", lit, 16)
    out += unit(448, 288, 180, 42, "写端口", "写回", lit, 16)
    out += to_bus(248, 298, "源操作数", "译码", lit)
    out += from_bus(248, 320, "译码", lit)

    # execute; movslq leaves the flags alone
    out += unit(UX, 186, 140, 44, "ALU", "执行", lit)
    out += to_bus(UX, 196, "结果或地址", "执行", lit)
    out += from_bus(UX, 220, "执行", lit)
    out += unit(436, 186, 120, 44, "标志位", "执行", set(), 16)
    out.append(arrow(UX + 140, 208, 434, 208, LINE, 2))

    # memory: movslq leaves the data memory idle
    out += unit(UX, 98, 180, 44, "数据内存", "访存", set())
    out += to_bus(UX, 108, "读出的数据", "访存", set())
    out += from_bus(UX, 132, "访存", set())

    # write-back: up the bus, along the top, down the right into the write port
    on = "写回" in lit
    c = tone("写回", lit)[3]
    out.append(elbow([(BUS, BANDS["写回"][1]), (BUS, 40), (WX, 40), (WX, 309), (630, 309)],
                     c, 2))
    out.append(text((BUS + WX) / 2, 32, "结果或数据", 15, c if on else MUTED,
                    "bold" if on else "normal"))
    return out


def build(stage):
    top, bottom = BANDS[stage]
    out = [rect(12, top + 3, W - 24, bottom - top - 6, "none", TONE[stage][1], rx=8,
                width=2, dash="6 4")]
    out += hardware({stage}, "0x20")
    heading, lines = PANEL[stage]
    span = 24 + 23 * (len(lines) - 1)               # heading baseline to last baseline
    # ink runs from 0.8 em above the heading to 0.2 em below the last line;
    # put its middle on the band's middle
    y = (top + bottom) / 2 - (span + 0.2 * 16 - 0.8 * 19) / 2
    out.append(text(PX, y, heading, 19, TONE[stage][1], "bold", anchor="start"))
    for k, s in enumerate(lines):
        out += rich(PX, y + 24 + 23 * k, s)
    return out


if __name__ == "__main__":
    for k, stage in enumerate(ORDER, 1):
        save(f"stage-units-{k}", W, H, build(stage))
