#!/usr/bin/env python3
"""Two ways to choose the set index of a 4-set cache over 16 blocks of main memory.
Middle bits: the index is the low two bits of the block number, so consecutive
blocks fall into different sets. High bits: the index is the top two bits, so
runs of 4 consecutive blocks fall into one set. An array of 4 consecutive blocks
(blocks 4 to 7, framed in orange) uses all four sets in the first layout and only
set 1 in the second. Run it to refresh ../assets/dm-index-bits.svg.
"""

from cachekit import *

W, H = 1000, 420
X0, CW, CH = 40, 58, 44
ARRAY = range(4, 8)


def bar(x, y, fields, cw=52, ch=30):
    """fields: [(kind, nbits)] from the high end to the low end of the address."""
    out = []
    cx = x
    for kind, n in fields:
        out.append(rect(cx, y, n * cw, ch, FIELD_FILL[kind], FIELD_COLOR[kind], rx=3,
                        width=2))
        name = {"tag": "标记", "idx": "组索引", "off": "块偏移"}[kind]
        out.append(text(cx + n * cw / 2, y + ch / 2 + 6, name, 15, FIELD_COLOR[kind], "bold"))
        cx += n * cw
    return out


def row(y, title, fields, idx_of, note):
    out = [text(X0, y - 40, title, 19, INK, "bold", "start")]
    out += bar(X0 + 260, y - 62, fields)
    for blk in range(16):
        st = idx_of(blk)
        x = X0 + blk * CW
        out.append(rect(x, y, CW - 4, CH, SET_FILLS[st], SET_STROKES[st], rx=2, width=1.4))
        out.append(text(x + (CW - 4) / 2, y + 29, f"组 {st}", 15, INK, "bold"))
        out.append(text(x + (CW - 4) / 2, y - 8, str(blk), 14, MUTED))
    x0 = X0 + ARRAY[0] * CW - 5
    out.append(rect(x0, y - 4, len(ARRAY) * CW + 2, CH + 8, "none", ORANGE, rx=5, width=3.2))
    out.append(text(X0 + 8 * CW - 2, y + CH + 32, note, 18, ORANGE, "bold"))
    return out


def build():
    out = [text(X0 - 6, 22, "块号", 15, MUTED, "start")]
    out += row(110, "用中间位作组索引",
               [("tag", 2), ("idx", 2), ("off", 1)], lambda blk: blk & 3,
               "数组的 4 个块落在 4 个不同的组，同时留在缓存中")
    out += row(290, "用高位作组索引",
               [("idx", 2), ("tag", 2), ("off", 1)], lambda blk: blk >> 2,
               "数组的 4 个块全部落在组 1，互相替换")
    return out


if __name__ == "__main__":
    save("dm-index-bits", W, H, build())
