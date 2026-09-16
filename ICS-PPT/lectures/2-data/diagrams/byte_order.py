#!/usr/bin/env python3
"""Where the bytes of one 32-bit value land in memory.

The slide states the rule in two lines; this figure is what the rule looks
like at four consecutive addresses, with the least significant byte marked so
the two orders can be told apart at a glance. The bottom band reads the same
question off the file the lecture hexdumps.
Run it to refresh ../assets/byte-order.svg.
"""

import pathlib

W, H = 900, 320

BLUE, ORANGE, INK = "#156082", "#e97132", "#0e2841"
LINE, MUTED = "#9fb5c3", "#5a6b78"
FILL_BLUE, FILL_ORANGE = "#eaf3f7", "#fce9df"
FONT = "PingFang SC, Noto Sans CJK SC, Source Han Sans SC, sans-serif"
MONO = "SFMono-Regular, Menlo, Consolas, monospace"

CELL_W, CELL_H, GAP = 104, 54, 10
X0, LABEL_X = 214, 196
ADDRS = ["0x100", "0x101", "0x102", "0x103"]
BIG = ["01", "23", "45", "67"]
LITTLE = ["67", "45", "23", "01"]


def cell_x(i):
    return X0 + i * (CELL_W + GAP)


def text(x, y, s, size=14, fill=INK, weight="normal", anchor="middle", font=FONT):
    return (f'<text x="{x}" y="{y}" font-family="{font}" font-size="{size}" '
            f'font-weight="{weight}" fill="{fill}" text-anchor="{anchor}">{s}</text>')


def band(y, label, cells, lsb_index):
    """One memory row: a label on the left, four byte cells to its right."""
    out = [text(LABEL_X, y + CELL_H / 2 + 5, label, 15, INK, "bold", "end")]
    for i, byte in enumerate(cells):
        low = i == lsb_index
        fill = FILL_ORANGE if low else FILL_BLUE
        stroke = ORANGE if low else BLUE
        out.append(f'<rect x="{cell_x(i)}" y="{y}" width="{CELL_W}" height="{CELL_H}" '
                   f'rx="6" fill="{fill}" stroke="{stroke}" stroke-width="1.6"/>')
        out.append(text(cell_x(i) + CELL_W / 2, y + CELL_H / 2 + 7, byte, 21,
                        ORANGE if low else INK, "bold", font=MONO))
    return out


def build():
    out = [f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" '
           f'width="{W}" height="{H}">']

    out.append(text(LABEL_X, 26, "内存地址", 13, MUTED, "normal", "end"))
    for i, a in enumerate(ADDRS):
        out.append(text(cell_x(i) + CELL_W / 2, 26, a, 13.5, MUTED, font=MONO))

    out += band(38, "大端 Big Endian", BIG, 3)
    out += band(108, "小端 Little Endian", LITTLE, 0)

    out.append(text(LABEL_X, 186, "被存放的值", 13, MUTED, "normal", "end"))
    out.append(text(X0, 188, "int x = 0x01234567;　橙色是最低有效字节", 14.5, INK,
                    "normal", "start"))

    # The same question, asked of the file the lecture opens with.
    y = 216
    out.append(f'<rect x="{LABEL_X - 176}" y="{y}" width="{W - 40 - (LABEL_X - 176)}" '
               f'height="{H - y - 16}" rx="8" fill="none" stroke="{LINE}" '
               f'stroke-width="1.3" stroke-dasharray="5 4"/>')
    out.append(text(LABEL_X - 158, y + 28, "tiny.gguf 的第 5 到第 8 个字节", 14, INK,
                    "bold", "start"))
    out.append(text(LABEL_X - 158, y + 54,
                    "03 00 00 00　按小端读出 0x3，按大端读出 0x03000000", 14.5, INK,
                    "normal", "start", font=MONO))
    out.append(text(LABEL_X - 158, y + 76,
                    "GGUF 规定小端，因此这一字段的值是 3", 13, MUTED, "normal", "start"))

    out.append("</svg>")
    return "\n".join(out)


if __name__ == "__main__":
    path = pathlib.Path(__file__).resolve().parent.parent / "assets" / "byte-order.svg"
    path.write_text(build(), encoding="utf-8")
    print(path)
