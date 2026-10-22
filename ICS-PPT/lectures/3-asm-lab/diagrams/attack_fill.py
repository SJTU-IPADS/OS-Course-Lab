#!/usr/bin/env python3
"""Six frames of the attack1 payload arriving in chat_turn's frame, for
p.frames on the lecture page: the stack frame before input, the 27-byte code
landing at the low end of the buffer, the buffer filling, the saved %rbp and
the return address being overwritten, ret landing in the code on the stack,
and the code calling report_pass.

The stack column on the left keeps the same shape in every frame; only the
cell colours and the overwrite arrow change, so one frame morphs into the
next. The panel on the right keeps the same zones in every frame — a step
tracker, a byte-progress bar, a short description, and the standing payload
listing — so nothing jumps position between frames.

The numbers are the demo build in ../examples/attack1, run with ASLR off,
where the buffer sits at 0x00007fffffffd550. Run it to refresh
../assets/attack_fill-1.svg ... attack_fill-6.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN, INK,
                    LINE, MUTED, ORANGE, WHITE, arrow, box, circle, elbow, line, mono,
                    rect, save, text)

W, H = 1120, 470

# the stack column on the left
SX0, SX1 = 88, 430
CX0, CW, CH = 99, 20, 22            # 16 cells centred in the box: 99 .. 419
RET_Y, RET_H = 56, 46               # return address, input offset 312
RBP_Y, RBP_H = 106, 44              # saved %rbp, input offset 304
BUF_Y, BUF_H = 154, 232             # the 304-byte buffer
TOP_Y = 188                         # offsets 288..303; grid centred in the box
ELI_Y0, ELI_Y1 = 216, 292          # the 256 elided bytes
R16_Y, R0_Y = 306, 330             # offsets 16..31 and 0..15
ARR_X = 36                          # the "input" direction arrow, in the left margin
HIJ_X = 464                         # the overwrite arrow's vertical leg

# the right panel
PX0, PX1 = 505, 1085

RET_MAIN = "0x00000000004025bc"
CODE_ADDR = "0x00007fffffffd550"
RBP_ORIG = "0x00007fffffff…"
# 27 bytes: mov $1,%edi; movabs $key,%rsi; movabs $&report_pass,%rax; call *%rax
CODE = ["bf", "01", "00", "00", "00", "48", "be", "0b", "23", "7d", "5c", "be",
        "fb", "d6", "27", "48", "b8", "00", "80", "52", "01", "00", "00", "00",
        "00", "ff", "d0"]

BYTES = {1: 0, 2: 27, 3: 304, 4: 320, 5: 320, 6: 320}

STEP_TAGS = ["写入前", "植入代码", "填满", "覆盖", "跳转", "执行"]
STEP_TITLES = {
    1: "① 写入前：正常的栈帧",
    2: "② 植入代码：27 字节落在低端",
    3: "③ 填满缓冲区：304 字节",
    4: "④ 覆盖返回地址",
    5: "⑤ ret：跳向栈上的代码",
    6: "⑥ 执行代码：调用 report_pass",
}
DESC = {
    1: ["缓冲区为空。返回地址是 main 中调用 chat_turn 的下一条指令 0x4025bc，chat_turn",
        "执行 ret 时就回到那里。"],
    2: ["输入开头 27 字节是 4 条指令的机器码，落在缓冲区偏移 0（最低地址）。返回地址",
        "在偏移 312，二者距离固定，与输入内容无关。"],
    3: ["再写 277 字节 0x41（字符 'A'）把缓冲区填到偏移 303。填充只为越过缓冲区，自身",
        "不执行；返回地址仍未改动。"],
    4: ["继续写 16 字节：偏移 304 的保存 %rbp 被 0x41 覆盖，偏移 312 的返回地址被改成",
        "缓冲区首地址 0x…d550。此刻它只是数据，等 ret 取用。"],
    5: ["chat_turn 执行 ret：从 %rbp+8 弹出返回地址送入 %rip，%rsp 指向 %rbp+16。%rip",
        "落到栈上的代码——栈可执行（-Wl,-z,execstack）使这一跳可行。"],
    6: ["栈上的代码把 1 与口令放入 %rdi、%rsi，call *%rax 进入 0x01528000 的",
        "report_pass，比较两参数后打印 phase 1 passed。"],
}
PAYLOAD = [
    ("bf 01 00 00 00", "mov    $1,%edi", "设 %edi = 1"),
    ("48 be 0b 23 7d 5c be fb d6 27", "movabs $0x27d6…230b,%rsi", "设 %rsi = 口令"),
    ("48 b8 00 80 52 01 00 00 00 00", "movabs $0x01528000,%rax", "设 %rax = report_pass"),
    ("ff d0", "call   *%rax", "调用"),
]


# ---------------------------------------------------------------- left column

def cell(cx, cy, value, fill, stroke):
    out = [rect(cx, cy, CW - 1, CH, fill, stroke, rx=1.5, width=1.0)]
    if value:
        out.append(mono(cx + CW / 2, cy + CH / 2 + 4, value, 10.5, INK, anchor="middle"))
    return out


def info_row(y, h, label, value, note, fill, stroke, note_col):
    out = [rect(SX0, y, SX1 - SX0, h, fill, stroke, rx=4, width=1.6),
           text(SX0 + 14, y + h / 2 + 5, label, 14, INK, "bold", anchor="start"),
           mono(SX0 + 128, y + 22, value, 13, INK, anchor="start")]
    if note:
        out.append(text(SX0 + 128, y + h - 9, note, 11.5, note_col, anchor="start"))
    return out


def stack(n):
    full = n >= 3                       # the filler bytes have arrived
    hijack = n >= 4                     # %rbp and the return address overwritten
    out = []

    # the return address and the saved %rbp
    if hijack:
        out += info_row(RET_Y, RET_H, "返回地址", CODE_ADDR, "指向缓冲区里的代码",
                        FILL_ORANGE, ORANGE, ORANGE)
        out += info_row(RBP_Y, RBP_H, "保存的 %rbp", "41 41 41 41 41 41 41 41",
                        "被 0x41 覆盖", FILL_ORANGE, ORANGE, ORANGE)
    else:
        out += info_row(RET_Y, RET_H, "返回地址", RET_MAIN, "main+0xde（正常返回）",
                        WHITE, LINE, MUTED)
        out += info_row(RBP_Y, RBP_H, "保存的 %rbp", RBP_ORIG, "调用者的帧基址",
                        FILL_GREY, MUTED, MUTED)

    # the buffer container
    out.append(rect(SX0, BUF_Y, SX1 - SX0, BUF_H, FILL_ORANGE if full else WHITE,
                    ORANGE, rx=4, width=1.6))
    out.append(text(SX0 + 14, BUF_Y + 20, "缓冲区", 14, INK, "bold", anchor="start"))
    out.append(text(SX0 + 64, BUF_Y + 20, "304 字节", 12, MUTED, anchor="start"))

    # top row: offsets 288..303
    for j in range(16):
        out += cell(CX0 + j * CW, TOP_Y,
                    "41" if full else "", FILL_ORANGE if full else FILL_GREY,
                    ORANGE if full else LINE)

    # the elided 256 bytes, drawn as a broken-out band
    eli_fill = FILL_ORANGE if full else WHITE
    out.append(rect(CX0, ELI_Y0, 16 * CW - 1, ELI_Y1 - ELI_Y0, eli_fill,
                    ORANGE if full else LINE, rx=3, width=1.0, dash="2 4"))
    ey = (ELI_Y0 + ELI_Y1) / 2
    for k in (-1, 1):                    # two break ticks, top and bottom
        out.append(line(CX0 + 6, ey + k * 10 - 3, CX0 + 24, ey + k * 10 + 3,
                        ORANGE if full else LINE, 1.4))
        out.append(line(16 * CW + CX0 - 24, ey + k * 10 - 3, 16 * CW + CX0 - 6,
                        ey + k * 10 + 3, ORANGE if full else LINE, 1.4))
    out.append(text((CX0 + 16 * CW + CX0) / 2, ey + 5,
                    "256 字节 · 全是 0x41" if full else "256 字节（未写入）",
                    12.5, INK if full else MUTED))

    # the two low rows where the code lands: offsets 16..31 and 0..15
    for base, ry in ((16, R16_Y), (0, R0_Y)):
        for j in range(16):
            i = base + j
            is_code = n >= 2 and i < len(CODE)
            written = is_code or full
            fill = FILL_BLUE if is_code else (FILL_ORANGE if written else FILL_GREY)
            stroke = BLUE if is_code else (ORANGE if written else LINE)
            value = CODE[i] if is_code else ("41" if written else "")
            out += cell(CX0 + j * CW, ry, value, fill, stroke)
    # the left-margin offset scale, all outside the box
    out.append(mono(SX0 - 6, RET_Y + RET_H - 4, "312", 11, MUTED, anchor="end"))
    out.append(mono(SX0 - 6, RBP_Y + RBP_H - 4, "304", 11, MUTED, anchor="end"))
    out.append(mono(SX0 - 6, R16_Y + CH / 2 + 4, "16", 11, MUTED, anchor="end"))
    out.append(mono(SX0 - 6, R0_Y + CH / 2 + 4, "0", 11, MUTED, anchor="end"))

    # the input direction: written from the low address upward (left margin)
    out.append(arrow(ARR_X, R0_Y + CH, ARR_X, RET_Y + 4, ORANGE, 2.0))
    for i, ch in enumerate("输入"):
        out.append(text(ARR_X - 18, 212 + i * 17, ch, 13, ORANGE, "bold", anchor="middle"))
    out.append(text(ARR_X, RET_Y - 6, "高", 11, MUTED, anchor="middle"))
    out.append(text(ARR_X, R0_Y + CH + 18, "低", 11, MUTED, anchor="middle"))

    # the overwritten return address pointing back into the buffer
    if hijack:
        dash = "6 4" if n == 4 else None
        out.append(elbow([(SX1, RET_Y + RET_H - 12), (HIJ_X, RET_Y + RET_H - 12),
                          (HIJ_X, R0_Y + CH / 2), (CX0 + 16 * CW, R0_Y + CH / 2)],
                         ORANGE, 2.2, dash=dash))
        if n >= 5:
            out.append(text(HIJ_X + 6, (RET_Y + R0_Y) / 2, "%rip", 12, ORANGE, "bold",
                            anchor="start"))
    return out


# ---------------------------------------------------------------- right panel

def stepper(n):
    out = []
    xs = [540 + i * 104 for i in range(6)]
    cy = 60
    out.append(line(xs[0], cy, xs[-1], cy, LINE, 2.0))
    if n > 1:
        out.append(line(xs[0], cy, xs[n - 1], cy, ORANGE, 2.4))
    for i in range(6):
        step = i + 1
        if step < n:
            fill, stroke, tcol = FILL_BLUE, BLUE, BLUE
        elif step == n:
            fill, stroke, tcol = ORANGE, ORANGE, WHITE
        else:
            fill, stroke, tcol = WHITE, LINE, MUTED
        out.append(circle(xs[i], cy, 13, fill, stroke, 1.8))
        out.append(text(xs[i], cy + 5, str(step), 13, tcol, "bold"))
        out.append(text(xs[i], cy + 28, STEP_TAGS[i], 11,
                        INK if step == n else MUTED, "bold" if step == n else "normal"))
    return out


def progress(n):
    b = BYTES[n]
    out = [text(PX0, 122, STEP_TITLES[n], 19, INK, "bold", anchor="start")]
    out.append(text(PX0, 150, "已写入", 13, MUTED, anchor="start"))
    out.append(text(PX1, 150, "%d / 320 字节" % b, 14, ORANGE if b else MUTED, "bold",
                    anchor="end"))
    tx0, ty, tw, th = PX0, 158, PX1 - PX0, 14
    out.append(rect(tx0, ty, tw, th, FILL_GREY, LINE, rx=7, width=1.0))
    if b:
        out.append(rect(tx0, ty, max(tw * b / 320.0, 14), th, ORANGE, ORANGE, rx=7,
                        width=1.0))
    return out


def description(n):
    out = []
    for i, ln in enumerate(DESC[n]):
        out.append(text(PX0, 196 + i * 23, ln, 14, INK, anchor="start"))
    return out


def payload(n):
    written = n >= 2
    run = n == 6
    out = [text(PX0, 245, "载荷代码", 14, ORANGE if written else MUTED, "bold",
                anchor="start"),
           text(PX0 + 70, 245, "27 字节 = 4 条指令" if written else "即将写入", 12,
                MUTED, anchor="start")]
    bx, by, bw, bh = PX0, 254, PX1 - PX0, 80       # box bottom 334, report bottom 386
    out.append(rect(bx, by, bw, bh, WHITE, ORANGE if written else LINE, rx=6, width=1.6))
    tcol = INK if written else MUTED
    for i, (hexs, asm, cmt) in enumerate(PAYLOAD):
        ly = by + 18 + i * 19
        if run and i == 3:
            out.append(rect(bx + 6, ly - 14, bw - 12, 18, FILL_GREEN, "none", rx=3,
                            width=0))
        out.append(mono(bx + 14, ly, "%-30s%s" % (hexs, asm), 12, tcol))
        out.append(mono(bx + 432, ly, cmt, 11, MUTED, anchor="start"))

    if run:
        out.append(arrow(bx + 60, by + bh, bx + 60, by + bh + 12, GREEN, 2.0))
        ry = by + bh + 12
        out.append(rect(bx, ry, bw, 40, FILL_GREEN, GREEN, rx=6, width=1.6))
        out.append(text(bx + 16, ry + 25, "report_pass(1, 口令)", 14, GREEN, "bold",
                        anchor="start"))
        out.append(mono(bx + 210, ry + 25, "0x01528000", 12, MUTED, anchor="start"))
        out.append(text(bx + 330, ry + 25, "→ phase 1 passed", 14, GREEN, "bold",
                        anchor="start"))
    return out


def frame(n):
    return stack(n) + stepper(n) + progress(n) + description(n) + payload(n)


if __name__ == "__main__":
    for n in range(1, 7):
        save("attack_fill-%d" % n, W, H, frame(n))
