#!/usr/bin/env python3
"""Finding the block length B of a cache whose B is unknown, in five frames.

A 256-byte node starts at a block boundary and is not in the cache. Load 1 reads the
node's first word and misses, which brings in the block that holds it. Load 2 reads the
word `offset` bytes further on. It hits when it lies in the same block, so the node costs
one miss, and it misses when it does not, so the node costs two. The strip is the node in
8-byte words. Frames 2 to 4 try offset = 8, 32, 56: the time stays at one miss, so the
first block reaches at least offset + 8 bytes. Frame 5 tries offset = 64: the time
doubles, so the block ends at or before byte 64, and B = 64.
Drawn at the width it is shown on the slide (1000 px).
Run it to refresh ../assets/linesize-method-1.svg ... linesize-method-5.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_BLUE, FILL_GREEN, FILL_RED, GREEN, INK, LINE, MUTED, RED, WHITE,
                    brace, elbow, line, rect, save, text)

W, H = 1000, 382
X0, CW, CY, CH = 52, 28, 54, 40            # strip origin, cell width (8 bytes), top, height
NODE = 256
OFFSETS = [8, 32, 56, 64]
SEG, HIT = 110, 14                           # bar: width of a miss and of a hit
BX, ROW0, ROW = 150, 204, 34
CAPTIONS = ["① 节点首地址与块对齐，读 1 miss，取入它所在的块，块长 B 未知",
            "② offset = 8：节点耗时只含 1 次 miss，读 2 在第一个块内，B ≥ 16",
            "③ offset = 32：耗时不变，读 2 仍在第一个块内，B ≥ 40",
            "④ offset = 56：耗时不变，读 2 仍在第一个块内，B ≥ 64",
            "⑤ offset = 64：耗时翻倍，读 2 不在第一个块内，B ≤ 64，所以 B = 64 字节"]


def cell_x(offset):
    return X0 + offset // 8 * CW


def frame(n):
    out = []
    tried = OFFSETS[:max(n - 1, 0)]          # the offsets measured so far
    off = tried[-1] if tried else None
    miss2 = n == 5
    covered = (off + 8) // 8 if off is not None and not miss2 else (8 if miss2 else 1)
    for c in range(NODE // 8):
        fill, stroke = (FILL_BLUE, LINE) if c < covered else (WHITE, LINE)
        out.append(rect(X0 + c * CW, CY, CW, CH, fill, stroke, rx=0, width=1.2))
    out.append(rect(cell_x(0), CY, CW, CH, FILL_RED, RED, rx=0, width=2))
    out.append(text(cell_x(0) + CW / 2, CY + CH / 2 + 5, "读1", 14, RED, "bold"))
    if off is not None:
        fill, stroke = (FILL_RED, RED) if miss2 else (FILL_GREEN, GREEN)
        out.append(rect(cell_x(off), CY, CW, CH, fill, stroke, rx=0, width=2))
        out.append(text(cell_x(off) + CW / 2, CY + CH / 2 + 5, "读2", 14, stroke, "bold"))
        x1, x2 = cell_x(0) + CW / 2, cell_x(off) + CW / 2
        out.append(elbow([(x1, CY - 3), (x1, CY - 18), (x2, CY - 18), (x2, CY - 3)], MUTED, 1.8))
        out.append(text((x1 + x2) / 2, CY - 26, f"offset = {off} 字节", 16, MUTED, "bold"))
    out.append(rect(X0, CY, NODE // 8 * CW, CH, "none", INK, rx=0, width=2))
    for byte in range(0, NODE + 1, 32):
        out.append(text(X0 + byte // 8 * CW, CY + CH + 20, str(byte), 14, MUTED))
    out.append(text(X0 - 8, CY + CH + 20, "字节", 14, MUTED, anchor="end"))
    if miss2:
        out.append(line(cell_x(64), CY - 6, cell_x(64), CY + CH + 6, INK, 3.4))
        out += brace(X0 + 2, cell_x(64) - 2, CY + CH + 32, INK, "B = 64 字节", 18, 8)
    elif off is not None:
        out += brace(X0 + 2, cell_x(off + 8) - 2, CY + CH + 32, MUTED, f"B ≥ {off + 8}", 17, 8)
    else:
        out.append(text(X0, CY + CH + 56, "第一个块从节点首地址开始，它的末端在哪里？", 17, MUTED,
                        anchor="start"))

    lx, ly = 500, CY + CH + 24                                           # legend
    for fill, stroke, label, width in ((FILL_RED, RED, "miss", 100), (FILL_GREEN, GREEN, "hit", 80),
                                       (FILL_BLUE, LINE, "已确定在第一个块内", 0)):
        out.append(rect(lx, ly, 20, 20, fill, stroke, rx=0, width=1.4))
        out.append(text(lx + 28, ly + 16, label, 16, INK, anchor="start"))
        lx += width + 28 + 20 * (width == 0)
    if n > 1:
        out.append(text(X0, ROW0 - 12, "offset", 16, MUTED, "bold", "start"))
        out.append(text(BX, ROW0 - 12, "一个节点的耗时", 16, MUTED, "bold", "start"))
        out.append(text(560, ROW0 - 12, "推论", 16, MUTED, "bold", "start"))
    for r, offset in enumerate(tried):
        y = ROW0 + r * ROW
        second_hits = offset < 64
        out.append(text(X0, y + 18, str(offset), 20, INK, "bold", "start"))
        out.append(rect(BX, y, SEG, 24, FILL_RED, RED, rx=2, width=1.4))
        out.append(text(BX + SEG / 2, y + 17, "读1 miss", 14, RED, "bold"))
        if second_hits:
            out.append(rect(BX + SEG, y, HIT, 24, FILL_GREEN, GREEN, rx=2, width=1.4))
            note = f"读2 hit，B ≥ {offset + 8}"
        else:
            out.append(rect(BX + SEG, y, SEG, 24, FILL_RED, RED, rx=2, width=1.4))
            out.append(text(BX + SEG * 1.5, y + 17, "读2 miss", 14, RED, "bold"))
            note = "读2 miss，耗时翻倍，B ≤ 64"
        out.append(text(560, y + 18, note, 18, INK, "bold" if not second_hits else "normal",
                        "start"))
    out.append(text(W / 2, H - 10, CAPTIONS[n - 1], 20, INK, "bold"))
    return out


if __name__ == "__main__":
    for n in range(1, 6):
        save(f"linesize-method-{n}", W, H, frame(n))
