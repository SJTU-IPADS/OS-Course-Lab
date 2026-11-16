#!/usr/bin/env python3
"""What the host code of vecadd.cu does with the two memories, one step per frame.

Left, the host: the CPU and the main memory that holds the arrays a, b, c.
Right, the device: the GPU and the device memory that will hold da, db, dc.
Frame 1 is the state before main touches the device. Each later frame adds the
arrows of one step of the code, in the order of the code:
(1) cudaMalloc allocates da, db, dc, (2) cudaMemcpy copies a, b to da, db,
(3) the kernel launch makes the GPU read da, db and write dc, (4) cudaMemcpy
copies dc back to c, (5) cudaFree releases da, db, dc. The arrows of the step
just taken are drawn in full colour, those of the earlier steps stay faded.
The calls that manage memory (1, 5) are dashed orange, the copies (2, 4) solid
blue, the launch (3) dark. An array that holds nothing yet is a dashed outline;
inputs a, b, da, db are blue and the result c, dc orange, as in the other
figures of this lecture.Run it to refresh ../assets/host-device-1.svg .. host-device-6.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_ORANGE, GREEN, INK, MUTED,
                    ORANGE, WHITE, arrow, elbow, mono, rect, save, text)

W = 1040
HOST_X, DEV_X, PANEL_W = 10, 740, 290      # the two panels
BOX_X = (40, 770)                          # left edge of the boxes in each panel
BOX_W = 230
PROC_Y, PROC_H = 34, 68                    # CPU and GPU
MEM_Y, MEM_H = 132, 154                    # the two memories
ROW_Y, ROW_H, ROW_GAP = 166, 30, 6         # the arrays inside a memory
PANEL_H = 302                              # the panels end below the memories
FADED = 0.38

H = PANEL_H + 8


def row_center(i):
    return ROW_Y + i * (ROW_H + ROW_GAP) + ROW_H / 2


def tag(x, y, s, size=15, color=INK, weight="bold"):
    """A label sitting on a line: white behind it, so the line seems to pause."""
    w = sum(1.0 if ord(c) > 0x2e80 else 0.58 for c in s) * size + 12
    return [rect(x - w / 2, y - size, w, size + 6, WHITE, "none", rx=2, width=0),
            text(x, y, s, size, color, weight)]


def array(bx, i, name, state):
    """One array in a memory: 'blue' / 'orange' hold data, 'empty' is a dashed outline."""
    y = ROW_Y + i * (ROW_H + ROW_GAP)
    if state == "empty":
        shape = rect(bx + 40, y, BOX_W - 80, ROW_H, WHITE, MUTED, rx=4, width=1.4, dash="5 4")
        ink = MUTED
    else:
        fill, stroke = (FILL_BLUE, BLUE) if state == "blue" else (FILL_ORANGE, ORANGE)
        shape = rect(bx + 40, y, BOX_W - 80, ROW_H, fill, stroke, rx=4, width=1.4)
        ink = INK
    return [shape, mono(bx + BOX_W / 2, y + ROW_H / 2 + 5, name, 16, ink, "bold", "middle")]


def side(x, title, proc, mem, rows):
    bx = BOX_X[0] if x == HOST_X else BOX_X[1]
    out = [rect(x, 8, PANEL_W, PANEL_H - 8, "none", MUTED, rx=8, width=1.2, dash="5 4"),
           text(x + 12, 28, title, 15, MUTED, "bold", anchor="start"),
           rect(bx, PROC_Y, BOX_W, PROC_H, WHITE, INK, rx=6, width=1.6),
           text(bx + BOX_W / 2, PROC_Y + PROC_H / 2 + 6, proc, 20, INK, "bold"),
           rect(bx, MEM_Y, BOX_W, MEM_H, FILL_GREEN, GREEN, rx=6, width=1.6),
           text(bx + 12, MEM_Y + 22, mem, 16, INK, "bold", anchor="start")]
    for i, (name, state) in enumerate(rows):
        out += array(bx, i, name, state)
    return out


def faded(shapes):
    return [f'<g opacity="{FADED}">'] + shapes + ["</g>"]


def steps(left, right):
    """The arrows of each step, keyed by the frame that adds them."""
    mid = (left + right) / 2
    ys = [row_center(i) for i in range(3)]
    return {
        2: [elbow([(left, 68), (690, 68), (690, MEM_Y + 12), (right, MEM_Y + 12)],
                  ORANGE, 2, dash="7 5"),
            *tag((left + 690) / 2, 73, "① cudaMalloc  分配 da、db、dc", 15, ORANGE)],
        3: [arrow(left, ys[0], right, ys[0], BLUE, 2), *tag(mid, ys[0] + 5, "② cudaMemcpy  a → da", 15, BLUE),
            arrow(left, ys[1], right, ys[1], BLUE, 2), *tag(mid, ys[1] + 5, "② cudaMemcpy  b → db", 15, BLUE)],
        4: [arrow(left, 44, right, 44, INK, 2.2),
            *tag(mid, 49, "③ 启动核函数  add_kernel<<<blocks, threads>>>", 15, INK)],
        5: [arrow(right, ys[2], left, ys[2], BLUE, 2), *tag(mid, ys[2] + 5, "④ cudaMemcpy  dc → c", 15, BLUE)],
        6: [elbow([(left, 92), (650, 92), (650, MEM_Y + 28), (right, MEM_Y + 28)],
                  ORANGE, 2, dash="7 5"),
            *tag((left + 650) / 2, 97, "⑤ cudaFree  释放 da、db、dc", 15, ORANGE)],
    }


def build(frame):
    host = [("a", "blue"), ("b", "blue"), ("c", "empty" if frame < 5 else "orange")]
    if frame == 1 or frame == 6:
        dev = []
    else:
        dev = [("da", "empty" if frame < 3 else "blue"), ("db", "empty" if frame < 3 else "blue"),
               ("dc", "empty" if frame < 4 else "orange")]
    out = side(HOST_X, "主机", "CPU", "主机内存", host)
    out += side(DEV_X, "设备", "GPU", "设备显存", dev)

    cx_host, cx_dev = BOX_X[0] + BOX_W / 2, BOX_X[1] + BOX_W / 2
    out.append(arrow(cx_host, PROC_Y + PROC_H, cx_host, MEM_Y, MUTED, 1.8, both=True))
    out.append(text(cx_host + 12, 123, "读写 a、b、c", 14, MUTED, anchor="start"))
    if frame >= 4:
        out.append(arrow(cx_dev, PROC_Y + PROC_H, cx_dev, MEM_Y, MUTED, 1.8, both=True))
        out.append(text(cx_dev + 12, 123, "读 da、db，写 dc", 14, MUTED, anchor="start"))

    arrows = steps(BOX_X[0] + BOX_W, BOX_X[1])
    for step in range(2, frame + 1):
        out += arrows[step] if step == frame else faded(arrows[step])
    return out


if __name__ == "__main__":
    for frame in range(1, 7):
        save(f"host-device-{frame}", W, H, build(frame))
