#!/usr/bin/env python3
"""mmap, drawn once.

The first lecture only needs the qualitative shape of the mechanism — a mapping
is recorded, an access faults, one page is brought in — so the whole thing is a
single picture with three numbered relations rather than a frame-by-frame walk;
the details belong to the virtual-memory chapter.
Run it to refresh ../assets/mmap-overview.svg.
"""

import pathlib

W, H = 900, 358
COLS = [
    (20, "进程虚拟地址空间", "推理进程看到的地址"),
    (355, "物理内存 DRAM", "操作系统页缓存"),
    (690, "存储设备", "model.gguf · 4.7 GB"),
]
COL_W = 190
BODY_Y, BODY_H = 52, 216
SLOT_X = 14
SLOT_H, SLOT_GAP, SLOT_TOP, SLOTS = 23, 3, 60, 8
MID_Y = 168

BLUE, ORANGE, INK = "#156082", "#e97132", "#0e2841"
LINE, MUTED, FAINT = "#9fb5c3", "#6b7c8d", "#d5dfe5"
FILL_BLUE, FILL_ORANGE, FILL_MAP = "#eaf3f7", "#fce9df", "#eef6fa"
FONT = "PingFang SC, Noto Sans CJK SC, Source Han Sans SC, sans-serif"

MAPPED = (1, 6)            # the slot range the mapping covers, inclusive
RESIDENT = {2, 3, 5}       # the pages an access has actually faulted in


def slot_y(i):
    return SLOT_TOP + i * (SLOT_H + SLOT_GAP)


def text(x, y, s, size=14, fill=INK, weight="normal"):
    return (f'<text x="{x}" y="{y}" font-family="{FONT}" font-size="{size}" '
            f'font-weight="{weight}" fill="{fill}" text-anchor="middle">{s}</text>')


def arrow(x1, x2, color, label):
    """Label sits on a white plate so it never reads into the column beside it."""
    head = "endBlue" if color == BLUE else "endOrange"
    mid, w = (x1 + x2) / 2, 13 * len(label)
    return (f'<line x1="{x1}" y1="{MID_Y}" x2="{x2}" y2="{MID_Y}" stroke="{color}" '
            f'stroke-width="1.8" marker-end="url(#{head})"/>'
            f'<rect x="{mid - w / 2}" y="{MID_Y - 24}" width="{w}" height="18" fill="#ffffff"/>'
            + text(mid, MID_Y - 11, label, 12.5, color))


def build():
    out = [
        f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" '
        f'width="{W}" height="{H}">',
        '<defs>',
        f'<marker id="endBlue" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" '
        f'markerHeight="7" orient="auto"><path d="M0,0 L10,5 L0,10 z" fill="{BLUE}"/></marker>',
        f'<marker id="endOrange" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" '
        f'markerHeight="7" orient="auto"><path d="M0,0 L10,5 L0,10 z" fill="{ORANGE}"/></marker>',
        '</defs>',
    ]

    for i, (x, title, sub) in enumerate(COLS):
        out.append(f'<rect x="{x}" y="{BODY_Y}" width="{COL_W}" height="{BODY_H}" rx="8" '
                   f'fill="none" stroke="{LINE}" stroke-width="1.3"/>')
        out.append(text(x + COL_W / 2, 26, title, 15, INK, "bold"))
        out.append(text(x + COL_W / 2, 43, sub, 12.5, "#5a6b78"))
        for k in range(SLOTS):
            fill, stroke, dash = "#ffffff", FAINT, ' stroke-dasharray="3 3"'
            if i == 2:
                fill, stroke, dash = FILL_BLUE, LINE, ""
            elif i == 1 and k in RESIDENT:
                fill, stroke, dash = FILL_ORANGE, ORANGE, ""
            out.append(f'<rect x="{x + SLOT_X}" y="{slot_y(k)}" width="{COL_W - 2 * SLOT_X}" '
                       f'height="{SLOT_H}" rx="3" fill="{fill}" stroke="{stroke}" '
                       f'stroke-width="1.2"{dash}/>')

    lo, hi = MAPPED
    y0, y1 = slot_y(lo) - 4, slot_y(hi) + SLOT_H + 4
    out.append(f'<rect x="{COLS[0][0] + 8}" y="{y0}" width="{COL_W - 16}" '
               f'height="{y1 - y0}" rx="5" fill="{FILL_MAP}" fill-opacity="0.75" '
               f'stroke="{BLUE}" stroke-width="1.8" stroke-dasharray="5 4"/>')
    out.append(text(COLS[0][0] + COL_W / 2, (y0 + y1) / 2 + 5, "映射区 · 4.7 GB", 14, BLUE, "bold"))

    # The mapping relates address space to file, so it arcs *under* the DRAM
    # column rather than through it — nothing is copied along the way.
    bot = BODY_Y + BODY_H
    out.append(f'<path d="M 115 {bot} C 115 {bot + 44}, 785 {bot + 44}, 785 {bot}" '
               f'fill="none" stroke="{BLUE}" stroke-width="1.8" stroke-dasharray="5 4"/>')
    out.append(f'<rect x="{W / 2 - 200}" y="{bot + 24}" width="400" height="22" fill="#ffffff"/>')
    out.append(text(W / 2, bot + 40, "① mmap：登记地址区间 ↔ 文件区间的对应，不拷贝数据", 13, BLUE))

    out.append(arrow(215, 350, BLUE, "② 访问映射区内的地址"))
    out.append(arrow(685, 550, ORANGE, "③ 缺页 → 装入这一页"))

    out.append(text(W / 2, H - 10,
                    "只有被访问到的页进入 DRAM，其余部分保持在存储设备上。", 13.5, MUTED))
    out.append("</svg>")
    return "\n".join(out)


if __name__ == "__main__":
    path = pathlib.Path(__file__).resolve().parent.parent / "assets" / "mmap-overview.svg"
    path.write_text(build(), encoding="utf-8")
    print(path)
