#!/usr/bin/env python3
"""The KV cache: each token's K and V computed once, read by every later token.

Left: the cache in GPU memory holding K and V of the five tokens already
processed, plus the pair the current token has just computed.  Right: the
attention of the current token reads all six pairs.  Run it to refresh
../assets/llm-kv-cache.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, INK, LINE, MUTED, ORANGE,
                    WHITE, arrow, box, brace, rect, save, text)

W, H = 1120, 232
N_OLD = 5
CX0, CY0 = 20, 36             # the cache box
COL, CW, CH = 92, 68, 30      # column pitch, cell width, cell height


def build():
    out = []
    ncol = N_OLD + 1
    box_w = 40 + ncol * COL
    out.append(rect(CX0, CY0, box_w, 178, FILL_GREY, MUTED, rx=8, width=1.6))
    out.append(text(CX0 + box_w / 2, CY0 + 22, "显存中的 KV 缓存（每一层一份）", 16, INK, "bold"))
    for i in range(ncol):
        x = CX0 + 32 + i * COL
        new = i == N_OLD
        fill, stroke = (FILL_ORANGE, ORANGE) if new else (FILL_BLUE, BLUE)
        out.append(text(x + CW / 2, CY0 + 50, f"t{i + 1}", 15, ORANGE if new else INK, "bold"))
        out += box(x, CY0 + 60, CW, CH, f"k{i + 1}", fill, stroke, 14, "normal")
        out += box(x, CY0 + 98, CW, CH, f"v{i + 1}", fill, stroke, 14, "normal")
    x_old0 = CX0 + 32
    x_old1 = x_old0 + N_OLD * COL - (COL - CW)
    out += brace(x_old0, x_old1, CY0 + 134, BLUE, "已经处理的 token：各自那一步算出，之后只读取", 14)
    x_new = CX0 + 32 + N_OLD * COL
    out += brace(x_new, x_new + CW, CY0 + 134, ORANGE, "本次算出", 14)
    # attention of the current token
    ax = CX0 + box_w + 70
    out += box(ax, CY0 + 40, W - ax - 20, 70, "当前 token t6 的注意力",
               WHITE, INK, 16, "bold", sub="读取 k1 ~ k6、v1 ~ v6，算出一个输出", sub_size=14)
    out.append(arrow(CX0 + box_w, CY0 + 75, ax - 2, CY0 + 75, INK, 2))
    out.append(text((CX0 + box_w + ax) / 2, CY0 + 66, "读", 14, MUTED))
    out.append(text(ax + (W - ax - 20) / 2, CY0 + 140,
                    "没有缓存时，生成每个 token 都要重算前面全部 token 的 k、v", 14, MUTED))
    return out


if __name__ == "__main__":
    save("llm-kv-cache", W, H, build())
