#!/usr/bin/env python3
"""The 18 bytes that hold 32 quantized weights.

The slide gives the arithmetic (4.5 bits per weight); this figure gives the
layout it comes from — llama.cpp's block_q4_0 — and zooms one byte out into
the two 4-bit fields the shift and the mask pull back apart.
Run it to refresh ../assets/q4-block.svg.
"""

import pathlib

W, H = 900, 330
BLUE, ORANGE, INK = "#156082", "#e97132", "#0e2841"
LINE, MUTED = "#9fb5c3", "#5a6b78"
FILL_BLUE, FILL_ORANGE = "#eaf3f7", "#fce9df"
FONT = "PingFang SC, Noto Sans CJK SC, Source Han Sans SC, sans-serif"
MONO = "SFMono-Regular, Menlo, Consolas, monospace"

STRIP_X, STRIP_Y, CELL, CELL_H = 40, 62, 45, 46      # 18 cells of 45px = 810
ZOOM_Y, BIT_W, BIT_H = 214, 52, 50
ZOOM_X = 250
FOCUS = 4                                            # the byte drawn twice


def esc(s):
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def text(x, y, s, size=13, fill=INK, weight="normal", anchor="middle", font=FONT):
    s = esc(s)
    return (f'<text x="{x}" y="{y}" font-family="{font}" font-size="{size}" '
            f'font-weight="{weight}" fill="{fill}" text-anchor="{anchor}">{s}</text>')


def build():
    out = [f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" '
           f'width="{W}" height="{H}">']

    for i in range(18):
        x = STRIP_X + i * CELL
        scale = i < 2
        fill = FILL_ORANGE if scale else FILL_BLUE
        stroke = ORANGE if scale else BLUE
        width = 2.2 if i == FOCUS else 1.4
        out.append(f'<rect x="{x}" y="{STRIP_Y}" width="{CELL}" height="{CELL_H}" '
                   f'rx="4" fill="{fill}" stroke="{stroke}" stroke-width="{width}"/>')
        out.append(text(x + CELL / 2, STRIP_Y + CELL_H / 2 + 5,
                        "d" if scale else "qs", 13.5, MUTED, font=MONO))

    out.append(text(STRIP_X + CELL, STRIP_Y - 14, "2 字节", 12.5, ORANGE, "bold"))
    out.append(text(STRIP_X + 2 * CELL + 8 * CELL, STRIP_Y - 14, "16 字节", 12.5,
                    BLUE, "bold"))
    out.append(text(STRIP_X + CELL, STRIP_Y + CELL_H + 20, "缩放系数 · fp16", 12.5, MUTED))
    out.append(text(STRIP_X + 2 * CELL + 8 * CELL, STRIP_Y + CELL_H + 20,
                    "32 个权重的 4 位编码", 12.5, MUTED))
    out.append(text(W / 2, 30, "block_q4_0：一组 32 个权重占 18 字节", 15.5, INK, "bold"))

    # zoom lines from the marked byte down to the enlarged one
    fx = STRIP_X + FOCUS * CELL
    for x_from, x_to in ((fx, ZOOM_X), (fx + CELL, ZOOM_X + 8 * BIT_W)):
        out.append(f'<line x1="{x_from}" y1="{STRIP_Y + CELL_H}" x2="{x_to}" '
                   f'y2="{ZOOM_Y}" stroke="{LINE}" stroke-width="1.2" '
                   f'stroke-dasharray="4 4"/>')

    for i in range(8):
        high = i < 4
        x = ZOOM_X + i * BIT_W
        out.append(f'<rect x="{x}" y="{ZOOM_Y}" width="{BIT_W}" height="{BIT_H}" '
                   f'rx="3" fill="{FILL_BLUE if high else FILL_ORANGE}" '
                   f'stroke="{BLUE if high else ORANGE}" stroke-width="1.5"/>')
        out.append(text(x + BIT_W / 2, ZOOM_Y + BIT_H / 2 + 6, "1010"[i % 4], 17,
                        INK, "bold", font=MONO))
        out.append(text(x + BIT_W / 2, ZOOM_Y - 8, f"b{7 - i}", 11.5, MUTED))

    out.append(text(ZOOM_X + 2 * BIT_W, ZOOM_Y + BIT_H + 24, "高 4 位 · 第 j+16 个权重",
                    13.5, BLUE, "bold"))
    out.append(text(ZOOM_X + 6 * BIT_W, ZOOM_Y + BIT_H + 24, "低 4 位 · 第 j 个权重",
                    13.5, ORANGE, "bold"))
    out.append(text(ZOOM_X + 2 * BIT_W, ZOOM_Y + BIT_H + 46, "qs[i] >> 4", 13.5,
                    INK, font=MONO))
    out.append(text(ZOOM_X + 6 * BIT_W, ZOOM_Y + BIT_H + 46, "qs[i] & 0x0f", 13.5,
                    INK, font=MONO))

    out.append("</svg>")
    return "\n".join(out)


if __name__ == "__main__":
    path = pathlib.Path(__file__).resolve().parent.parent / "assets" / "q4-block.svg"
    path.write_text(build(), encoding="utf-8")
    print(path)
