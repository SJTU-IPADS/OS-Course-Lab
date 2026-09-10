#!/usr/bin/env python3
"""The process virtual address space, drawn for this lecture.

The stock CS:APP figure is an English screenshot that reads as noise at slide
size. This one is in Chinese, carries only the five regions the lecture names,
and puts the mmap region in the accent colour so the next page can point at it.
Run it to refresh ../assets/address-space.svg.
"""

import pathlib

W, H = 460, 470
X, BW = 90, 250
TOP, BOT = 40, 430

BLUE, ORANGE, INK = "#156082", "#e97132", "#0e2841"
LINE, MUTED = "#9fb5c3", "#5a6b78"
FONT = "PingFang SC, Noto Sans CJK SC, Source Han Sans SC, sans-serif"

# (height weight, label, sublabel, fill, stroke, arrow)
BANDS = [
    (1.0, "内核", "用户代码看不见", "#eceff1", "#b9c4cb", None),
    (1.1, "栈", "局部变量 · 调用现场", "#eaf3f7", LINE, "down"),
    (2.4, "共享库 / mmap 区域", "libc · 映射进来的 4.7 GB 权重", "#fce9df", ORANGE, None),
    (1.1, "堆", "malloc / new", "#eaf3f7", LINE, "up"),
    (0.8, "Data / BSS", "全局与静态变量", "#eaf3f7", LINE, None),
    (0.8, "Text", "机器指令 · 只读", "#eaf3f7", LINE, None),
]


def build():
    total = sum(b[0] for b in BANDS)
    unit = (BOT - TOP) / total
    out = [f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" width="{W}" height="{H}">',
           '<defs><marker id="tip" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="6" '
           f'markerHeight="6" orient="auto"><path d="M0,0 L10,5 L0,10 z" fill="{MUTED}"/></marker></defs>',
           f'<text x="{W / 2}" y="24" font-family="{FONT}" font-size="15" font-weight="bold" '
           f'fill="{INK}" text-anchor="middle">进程虚拟地址空间</text>']
    y = TOP
    for weight, label, sub, fill, stroke, grow in BANDS:
        h = weight * unit
        out.append(f'<rect x="{X}" y="{y:.1f}" width="{BW}" height="{h:.1f}" fill="{fill}" '
                   f'stroke="{stroke}" stroke-width="1.4"/>')
        bold = "bold" if stroke == ORANGE else "600"
        color = ORANGE if stroke == ORANGE else INK
        out.append(f'<text x="{X + BW / 2}" y="{y + h / 2 - 3:.1f}" font-family="{FONT}" '
                   f'font-size="14.5" font-weight="{bold}" fill="{color}" '
                   f'text-anchor="middle">{label}</text>')
        out.append(f'<text x="{X + BW / 2}" y="{y + h / 2 + 15:.1f}" font-family="{FONT}" '
                   f'font-size="11.5" fill="{MUTED}" text-anchor="middle">{sub}</text>')
        if grow == "down":
            out.append(f'<line x1="{X + BW + 16}" y1="{y + 8:.1f}" x2="{X + BW + 16}" '
                       f'y2="{y + h - 4:.1f}" stroke="{MUTED}" stroke-width="1.3" '
                       f'marker-end="url(#tip)"/>')
            out.append(f'<text x="{X + BW + 24}" y="{y + h / 2:.1f}" font-family="{FONT}" '
                       f'font-size="11.5" fill="{MUTED}">向下长</text>')
        if grow == "up":
            out.append(f'<line x1="{X + BW + 16}" y1="{y + h - 8:.1f}" x2="{X + BW + 16}" '
                       f'y2="{y + 4:.1f}" stroke="{MUTED}" stroke-width="1.3" '
                       f'marker-end="url(#tip)"/>')
            out.append(f'<text x="{X + BW + 24}" y="{y + h / 2:.1f}" font-family="{FONT}" '
                       f'font-size="11.5" fill="{MUTED}">向上长</text>')
        y += h
    out.append(f'<text x="{X - 8}" y="{TOP + 6}" font-family="{FONT}" font-size="11.5" '
               f'fill="{MUTED}" text-anchor="end">高地址</text>')
    out.append(f'<text x="{X - 8}" y="{BOT}" font-family="{FONT}" font-size="11.5" '
               f'fill="{MUTED}" text-anchor="end">低地址</text>')
    out.append(f'<text x="{W / 2}" y="{H - 12}" font-family="{FONT}" font-size="11.5" '
               f'fill="{MUTED}" text-anchor="middle">实际排列还受 ABI、ASLR、动态链接与线程影响</text>')
    out.append("</svg>")
    return "\n".join(out)


if __name__ == "__main__":
    path = pathlib.Path(__file__).resolve().parent.parent / "assets" / "address-space.svg"
    path.write_text(build(), encoding="utf-8")
    print(path)
