#!/usr/bin/env python3
"""How the parts of one machine are wired together.

The slide names CPU / DRAM / GPU / 总线与 I/O in four bullets; this figure shows
the topology those four names describe — two buses, the memory bus between CPU
and DRAM and the I/O bus everything else hangs off — and puts an order-of-
magnitude bandwidth on each link, because the next slide argues that decoding is
bandwidth-bound. Run it to refresh ../assets/hardware-bus.svg.
"""

import pathlib

W, H = 940, 300

BLUE, ORANGE, INK = "#156082", "#e97132", "#0e2841"
LINE, MUTED = "#9fb5c3", "#5a6b78"
FILL_BLUE, FILL_ORANGE, FILL_GREY = "#eaf3f7", "#fce9df", "#f2f5f7"
FONT = "PingFang SC, Noto Sans CJK SC, Source Han Sans SC, sans-serif"

BOX_W, BOX_H = 200, 68
TOP_Y = 34
BUS_Y = 176               # centre line of the I/O bus bar
BOT_Y = 214
BOT_W, BOT_H = 190, 66

CPU_X, DRAM_X = 92, 512   # the memory bus runs between these two
BUS_X0, BUS_X1 = 60, 880

DEVICES = [               # (x, title, sub, faded)
    (60, "GPU 加速器", "并行执行单元 + HBM 显存", False),
    (272, "存储设备", "NVMe SSD · 权重文件在此", False),
    (484, "网络接口", "NIC · 接收推理请求", False),
    (696, "其他外设", "USB · 显示输出", True),
]


def text(x, y, s, size=13, fill=INK, weight="normal", anchor="middle"):
    return (f'<text x="{x}" y="{y}" font-family="{FONT}" font-size="{size}" '
            f'font-weight="{weight}" fill="{fill}" text-anchor="{anchor}">{s}</text>')


def box(x, y, w, h, title, sub, fill, stroke, faded=False):
    dash = ' stroke-dasharray="5 4"' if faded else ""
    color = MUTED if faded else INK
    return [
        f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="7" fill="{fill}" '
        f'stroke="{stroke}" stroke-width="1.5"{dash}/>',
        text(x + w / 2, y + 27, title, 15.5, color, "bold"),
        text(x + w / 2, y + 48, sub, 11.5, MUTED),
    ]


def build():
    out = [f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" '
           f'width="{W}" height="{H}">']

    # --- CPU and DRAM, joined by the memory bus ------------------------------
    out += box(CPU_X, TOP_Y, BOX_W, BOX_H, "CPU",
               "控制单元 · ALU · 寄存器 · 缓存", FILL_BLUE, BLUE)
    out += box(DRAM_X, TOP_Y, BOX_W, BOX_H, "DRAM 主存",
               "当前使用的代码与数据", FILL_BLUE, BLUE)

    mid = TOP_Y + BOX_H / 2
    x0, x1 = CPU_X + BOX_W, DRAM_X
    out.append(f'<rect x="{x0}" y="{mid - 5}" width="{x1 - x0}" height="10" rx="5" '
               f'fill="{FILL_ORANGE}" stroke="{ORANGE}" stroke-width="1.4"/>')
    out.append(text((x0 + x1) / 2, mid - 14, "内存总线", 14, ORANGE, "bold"))
    out.append(text((x0 + x1) / 2, mid + 27, "数量级 100 GB/s", 11.5, MUTED))

    # --- the drop from the CPU down to the I/O bus ---------------------------
    cx = CPU_X + BOX_W / 2
    out.append(f'<line x1="{cx}" y1="{TOP_Y + BOX_H}" x2="{cx}" y2="{BUS_Y}" '
               f'stroke="{LINE}" stroke-width="1.6"/>')
    dx = DRAM_X + BOX_W / 2
    out.append(f'<line x1="{dx}" y1="{TOP_Y + BOX_H}" x2="{dx}" y2="{BUS_Y}" '
               f'stroke="{LINE}" stroke-width="1.6" stroke-dasharray="4 4"/>')
    out.append(text(dx + 8, BUS_Y - 28, "DMA：设备直接读写主存", 11.5, MUTED, anchor="start"))

    # --- the I/O bus bar ------------------------------------------------------
    out.append(f'<rect x="{BUS_X0}" y="{BUS_Y - 6}" width="{BUS_X1 - BUS_X0}" height="12" '
               f'rx="6" fill="{FILL_ORANGE}" stroke="{ORANGE}" stroke-width="1.4"/>')
    out.append(text((BUS_X0 + BUS_X1) / 2, BUS_Y - 14,
                    "I/O 总线（PCIe）　数量级 64 GB/s", 14, ORANGE, "bold"))

    # --- the devices hanging off it ------------------------------------------
    for x, title, sub, faded in DEVICES:
        fill = FILL_GREY if faded else "#ffffff"
        stroke = LINE
        out.append(f'<line x1="{x + BOT_W / 2}" y1="{BUS_Y}" x2="{x + BOT_W / 2}" '
                   f'y2="{BOT_Y}" stroke="{LINE}" stroke-width="1.6"/>')
        out += box(x, BOT_Y, BOT_W, BOT_H, title, sub, fill, stroke, faded)

    out.append("</svg>")
    return "\n".join(out)


if __name__ == "__main__":
    path = pathlib.Path(__file__).resolve().parent.parent / "assets" / "hardware-bus.svg"
    path.write_text(build(), encoding="utf-8")
    print(path)
