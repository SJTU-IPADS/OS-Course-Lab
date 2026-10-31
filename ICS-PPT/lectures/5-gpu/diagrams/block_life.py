#!/usr/bin/env python3
"""The life of thread blocks on two SMs: waiting, resident, executing, finished.

Sixteen blocks wait. Each SM takes 6 of them whole (256 threads each, 8 warps
each) until its 1536 thread slots are full; the other 4 keep waiting. Resident
warps do not all issue at once: in one cycle the 4 warp schedulers of an SM
pick 4 warps (orange), in the next cycle 4 others. Then block 2 of SM 0 ends,
frees its slot and its share of the resources, and block 12 takes the slot.
Frame k: ../assets/block-life-1.svg .. block-life-7.svg. Run it to refresh them.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN, INK, LINE,
                    MUTED, ORANGE, WHITE, arrow, elbow, rect, save, text)

W, H = 1120, 368
QUEUE = (14, 8, 280, 266)                 # x, y, w, h of the waiting area
SM_X, SM_Y, SM_W, SM_H = (340, 740), 8, 360, 266
SLOT_W, SLOT_H, SLOT_GAP, SLOT_TOP = 104, 76, 12, 44
WARP_W, WARP_H, WARP_GAP = 20, 18, 4
THREADS = 1536
BLOCKS = 16

# Warps that issue in the two cycles: (sm, slot, warp).
ISSUE_T = {(0, 0, 3), (0, 1, 5), (0, 3, 0), (0, 4, 6), (1, 0, 1), (1, 2, 4), (1, 3, 7), (1, 5, 2)}
ISSUE_T1 = {(0, 0, 6), (0, 2, 2), (0, 3, 5), (0, 5, 1), (1, 1, 0), (1, 2, 7), (1, 4, 3), (1, 5, 5)}

FULL = [[(n, "run") for n in range(0, 6)], [(n, "run") for n in range(6, 12)]]


def with_slot(sms, sm, slot, value):
    out = [list(s) for s in sms]
    out[sm][slot] = value
    return out


# (blocks waiting, blocks in SM slots, warps issuing, threads in use, arrows, note)
FRAMES = [
    (range(16), [[None] * 6, [None] * 6], set(), (0, 0), (),
     "16 个线程块等待分配，两个 SM 都是空的"),
    (range(12, 16), FULL, set(), (1536, 1536), ("both",),
     "每个线程块整块分配给一个 SM，各占 256 个线程。两个 SM 各驻留 6 个线程块，线程数已满，其余线程块继续等待"),
    (range(12, 16), FULL, ISSUE_T, (1536, 1536), (),
     "周期 t：每个 SM 的 4 个 Warp 调度器各发射一个 Warp 的一条指令，其余 44 个驻留的 Warp 本周期不发射"),
    (range(12, 16), FULL, ISSUE_T1, (1536, 1536), (),
     "周期 t + 1：调度器重新选择，选中的 Warp 可以与上一周期相同，也可以不同，驻留的 48 个 Warp 都保留着自己的寄存器"),
    (range(12, 16), with_slot(FULL, 0, 2, (2, "done")), set(), (1536, 1536), (),
     "B2 的全部线程执行完毕，线程块完成"),
    (range(12, 16), with_slot(FULL, 0, 2, None), set(), (1280, 1536), (),
     "B2 释放它占用的线程数、寄存器与共享内存，SM 0 空出一个线程块的位置"),
    (range(13, 16), with_slot(FULL, 0, 2, (12, "new")), set(), (1536, 1536), ("fill",),
     "硬件调度器把等待的 B12 分配到 SM 0 的空位，B13 至 B15 继续等待"),
]


def slot_xy(sm, slot):
    col, row = slot % 3, slot // 3
    return (SM_X[sm] + 12 + col * (SLOT_W + SLOT_GAP),
            SM_Y + SLOT_TOP + row * (SLOT_H + SLOT_GAP))


def waiting_xy(n):
    col, row = n % 4, n // 4
    return QUEUE[0] + 14 + col * 66, QUEUE[1] + 46 + row * 50


def queue(waiting):
    x, y, w, h = QUEUE
    out = [rect(x, y, w, h, WHITE, LINE, rx=6, width=1.4, dash="6 4"),
           text(x + w / 2, y + 26, "等待的线程块", 17, INK, "bold")]
    for n in range(BLOCKS):
        bx, by = waiting_xy(n)
        if n in waiting:
            out.append(rect(bx, by, 56, 36, FILL_GREY, MUTED, rx=3, width=1.4))
            out.append(text(bx + 28, by + 24, f"B{n}", 16, INK))
        else:
            out.append(rect(bx, by, 56, 36, "none", LINE, rx=3, width=1, dash="3 3"))
    return out


def block(sm, slot, n, state, issuing):
    x, y = slot_xy(sm, slot)
    fill, stroke = {"run": (FILL_BLUE, BLUE), "done": (FILL_GREEN, GREEN),
                    "new": (FILL_BLUE, ORANGE)}[state]
    out = [rect(x, y, SLOT_W, SLOT_H, fill, stroke, rx=4, width=2.6 if state == "new" else 1.6),
           text(x + 8, y + 18, f"B{n}", 15, INK, "bold", "start")]
    if state == "done":
        out.append(text(x + SLOT_W - 8, y + 18, "完成", 14, GREEN, "bold", "end"))
    for k in range(8):
        wx = x + 6 + (k % 4) * (WARP_W + WARP_GAP)
        wy = y + 28 + (k // 4) * (WARP_H + WARP_GAP)
        if state == "done":
            f, s = GREEN, GREEN
        elif (sm, slot, k) in issuing:
            f, s = ORANGE, ORANGE
        else:
            f, s = FILL_GREY, MUTED
        out.append(rect(wx, wy, WARP_W, WARP_H, f, s, rx=2, width=1.2))
    return out


def sm_box(sm, blocks, issuing, threads):
    x = SM_X[sm]
    out = [rect(x, SM_Y, SM_W, SM_H, WHITE, INK, rx=6, width=1.6),
           text(x + 14, SM_Y + 28, f"SM {sm}", 17, INK, "bold", "start"),
           text(x + SM_W - 14, SM_Y + 28, "4 个 Warp 调度器", 14, MUTED, anchor="end")]
    for slot in range(6):
        entry = blocks[slot]
        if entry is None:
            sx, sy = slot_xy(sm, slot)
            out.append(rect(sx, sy, SLOT_W, SLOT_H, "none", LINE, rx=4, width=1.2, dash="4 3"))
        else:
            out += block(sm, slot, entry[0], entry[1], issuing)
    by = SM_Y + SLOT_TOP + 2 * SLOT_H + SLOT_GAP + 14
    out += [text(x + 12, by + 14, "占用的线程数", 14, MUTED, anchor="start"),
            text(x + SM_W - 12, by + 14, f"{threads} / {THREADS}", 14, INK, "bold", "end"),
            rect(x + 12, by + 22, SM_W - 24, 12, FILL_GREY, LINE, rx=3, width=1),
            rect(x + 12, by + 22, (SM_W - 24) * threads / THREADS, 12, BLUE, BLUE, rx=3, width=1)
            if threads else ""]
    return out


def arrows(kinds, waiting):
    if "both" in kinds:
        mid = QUEUE[1] + QUEUE[3] / 2
        return [arrow(QUEUE[0] + QUEUE[2], mid, SM_X[0] - 2, mid, ORANGE, 2.4),
                elbow([(QUEUE[0] + QUEUE[2] - 4, mid + 40), (QUEUE[0] + QUEUE[2] + 22, mid + 40),
                       (QUEUE[0] + QUEUE[2] + 22, SM_Y + SM_H + 22), (SM_X[1] + 180, SM_Y + SM_H + 22),
                       (SM_X[1] + 180, SM_Y + SM_H + 2)], ORANGE, 2.4)]
    if "fill" in kinds:
        mid = QUEUE[1] + QUEUE[3] / 2
        return [arrow(QUEUE[0] + QUEUE[2], mid, SM_X[0] - 2, mid, ORANGE, 2.4)]
    return []


def legend(y):
    out, x = [], 300
    for fill, stroke, dash, label in ((FILL_GREY, MUTED, None, "驻留的 Warp"),
                                      (ORANGE, ORANGE, None, "本周期发射指令的 Warp"),
                                      ("none", LINE, "4 3", "空位")):
        out.append(rect(x, y - 13, 20, 16, fill, stroke, rx=2, width=1.2, dash=dash))
        out.append(text(x + 28, y, label, 15, MUTED, anchor="start"))
        x += 28 + len(label) * 15 + 36
    return out


def frame(k):
    waiting, blocks, issuing, threads, kinds, note = FRAMES[k - 1]
    out = queue(set(waiting))
    for sm in range(2):
        used = threads[sm] if isinstance(threads, tuple) else threads
        out += sm_box(sm, blocks[sm], issuing, used)
    out += arrows(kinds, waiting)
    out.append(text(W / 2, 324, note, 17, INK, "bold"))
    out += legend(356)
    return out


if __name__ == "__main__":
    for k in range(1, len(FRAMES) + 1):
        save(f"block-life-{k}", W, H, frame(k), left=-3)
