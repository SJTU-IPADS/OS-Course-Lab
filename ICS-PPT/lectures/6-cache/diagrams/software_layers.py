#!/usr/bin/env python3
"""The hardware as a stack ordered by access speed, with the software that reaches it beside it.

The three figures share one layout, so a device stays in its place from one page to
the next. software-layers and resource-cache also share one canvas, so the page draws
them at one scale; a legend goes inside that canvas. The devices stand from top to bottom in the order of storage_levels.py,
fastest first, with a dashed box around the two rows inside the CPU. Applications and
the operating system are two columns on the left, and what they reach is a horizontal
arrow at the height of the device:
  software-layers  what each column can reach: interfaces, drivers, and the caches
                   inside the hardware that no software can change;
  resource-cache   a game downloads its resource files from the cloud to the SSD and
                   later reads them from there;
  file-cache       a program reads a whole file into memory once and later only loads
                   from memory, so its arrow passes over the operating system.
Orange is the data being fetched, green the read that finds it nearby. The device fills
follow storage_levels.py. The icons are read from icons/ and embedded, since an SVG shown
in <img> loads no other file.
Run it to refresh ../assets/software-layers.svg, resource-cache.svg and file-cache.svg.
"""

import base64
import pathlib
import sys

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

from svgkit import (FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN, INK, MONO, MUTED, ORANGE,
                    RED, WHITE, arrow, line, rect, save, text)

W = 620
APP_X, APP_W = 2, 78
OS_X, OS_W = 172, 110
GAP_MID = (APP_X + APP_W + OS_X) / 2      # the gap between the two columns, for its labels
FRAME_X, FRAME_W = 352, 240
CX = 472                                  # centre line of the hardware stack
OS_GAP_MID = (OS_X + OS_W + FRAME_X) / 2  # the gap between the operating system and the frame
SPEED_X = 606
TOP = 6
CHIP_DY, CHIP_H = 34, 34                  # a chip inside a tall row, from the top of the row
# Each row of the stack: top, height, width. The widths grow downwards, as in storage_levels.py.
# An arrow head is 7 stroke widths long, and the frames leave room for it in front of a row.
REG = (44, 40, 120)
SRAM = (92, 40, 176)
DRAM = (150, 78, 184)
SSD = (238, 78, 192)
NIC = (326, 44, 192)
REMOTE = (408, 78, 206)
CPU_X, CPU_TOP, CPU_W, CPU_BOTTOM = 364, 36, 216, 140
FRAME_BOTTOM = NIC[0] + NIC[1] + 24
BOTTOM = REMOTE[0] + REMOTE[1]
ICON = 52
APP_CHIP_H = 46
ICON_CLIP = ('<clipPath id="icon-clip" clipPathUnits="objectBoundingBox">'
             '<rect width="1" height="1" rx="0.2"/></clipPath>')


def left(row):
    return CX - row[2] / 2


def level(row):
    """The height of the arrows that reach a row: the middle of its chip, or of the row."""
    y, height, _ = row
    return y + CHIP_DY + CHIP_H / 2 if height > 50 else y + height / 2


def column(x, width, top, bottom, name):
    return [rect(x, top, width, bottom - top, WHITE, MUTED, rx=6, width=1.6),
            text(x + width / 2, top + 28, name, 17, INK, "bold")]


def device(row, label, fill, stroke, dash=None, title_x=None, empty=False):
    """A row of the stack. A title at `title_x` leaves room for an arrow through the middle."""
    y, height, width = row
    baseline = y + 24 if height > 50 and not empty else y + height / 2 + 6
    out = [rect(CX - width / 2, y, width, height, fill, stroke, rx=6, width=1.6, dash=dash)]
    if title_x is None:
        out.append(text(CX, baseline, label, 16, INK, "bold"))
    else:
        out.append(text(title_x, baseline, label, 16, INK, "bold", "start"))
    return out


def chip(row, label, fill, stroke, dash=None, cx=CX, width=92, font=None):
    y = row[0] + CHIP_DY
    style = {"font": font} if font else {}
    return [rect(cx - width / 2, y, width, CHIP_H, fill, stroke, rx=4, width=1.6, dash=dash),
            text(cx, y + CHIP_H / 2 + 5, label, 14, INK, **style)]


def driver(row, label):
    """The driver of a device, in the operating system at the height of that device."""
    y = level(row) - CHIP_H / 2
    return [rect(OS_X + 6, y, OS_W - 12, CHIP_H, FILL_GREY, MUTED, rx=4, width=1.6),
            text(OS_X + OS_W / 2, y + CHIP_H / 2 + 5, label, 14, INK)]


def centred(count, height, gap):
    """Items of one height, stacked and centred in the columns: the first top and the step."""
    return (TOP + FRAME_BOTTOM - count * height - (count - 1) * gap) / 2, height + gap


def application(y, *lines):
    """An application in its column, in the style of the drivers in the next column."""
    out = [rect(APP_X + 6, y, APP_W - 12, APP_CHIP_H, FILL_GREY, MUTED, rx=4, width=1.6)]
    first = y + APP_CHIP_H / 2 + 5 - (len(lines) - 1) * 9
    for index, label in enumerate(lines):
        out.append(text(APP_X + APP_W / 2, first + index * 18, label, 14, INK))
    return out


def speed(bottom):
    """The stack reads from fast to slow."""
    return [text(SPEED_X, REG[0] - 10, "快", 15, MUTED, "bold"),
            arrow(SPEED_X, REG[0] - 4, SPEED_X, bottom - 22, MUTED, 2),
            text(SPEED_X, bottom - 4, "慢", 15, MUTED, "bold")]


def stack(frame_bottom=FRAME_BOTTOM):
    """The frame of the machine, the CPU inside it and the registers, which no figure draws on."""
    return ([rect(FRAME_X, TOP, FRAME_W, frame_bottom - TOP, WHITE, MUTED, rx=6, width=1.6),
             text(FRAME_X + 12, TOP + 22, "硬件", 16, INK, "bold", "start"),
             rect(CPU_X, CPU_TOP, CPU_W, CPU_BOTTOM - CPU_TOP, WHITE, MUTED, rx=6, width=1.6, dash="7 5"),
             text(CPU_X + 8, REG[0] + REG[1] / 2 + 5, "CPU", 15, INK, "bold", "start")]
            + device(REG, "寄存器", FILL_BLUE, INK))


def icon(x, y, name):
    data = base64.b64encode((HERE / "icons" / f"{name}.png").read_bytes()).decode()
    return (f'<image x="{x}" y="{y}" width="{ICON}" height="{ICON}" clip-path="url(#icon-clip)" '
            f'href="data:image/png;base64,{data}"/>')


def software_layers():
    out = column(APP_X, APP_W, TOP, FRAME_BOTTOM, "应用") + column(OS_X, OS_W, TOP, FRAME_BOTTOM, "操作系统")
    # The applications stand together in the middle of the column.
    top, step = centred(3, APP_CHIP_H, 16)
    out += (application(top, "浏览器") + application(top + step, "游戏")
            + application(top + 2 * step, "自己编写", "的程序"))
    out += stack()
    out += device(SRAM, "SRAM：L1 至 L3 缓存", FILL_BLUE, RED, "5 4")
    out += device(DRAM, "DRAM：内存", FILL_GREEN, GREEN) + chip(DRAM, "行缓冲", WHITE, RED, "5 4")
    out += device(SSD, "SSD", FILL_GREY, MUTED) + chip(SSD, "内部缓存", WHITE, RED, "5 4")
    out += device(NIC, "网卡", FILL_GREY, MUTED)
    out += device(REMOTE, "远端存储", FILL_GREY, MUTED, "7 5")
    out.append(text(CX, REMOTE[0] + CHIP_DY + CHIP_H / 2 + 5, "其他机器", 14, MUTED))
    out.append(line(CX, NIC[0] + NIC[1], CX, REMOTE[0], MUTED, 2))
    out += driver(SSD, "SSD 驱动程序") + driver(NIC, "网卡驱动程序")
    # Applications reach the operating system through its interfaces.
    out.append(text(GAP_MID, level(DRAM) - 61, "操作系统", 14, MUTED, "bold"))
    out.append(text(GAP_MID, level(DRAM) - 43, "提供的接口", 14, MUTED, "bold"))
    for row, label in ((DRAM, "申请内存"), (SSD, "读写文件"), (NIC, "收发网络数据")):
        y = level(row)
        out.append(arrow(APP_X + APP_W + 2, y, OS_X - 2, y, INK, 2))
        out.append(text(GAP_MID, y - 8, label, 14, INK))
    # The operating system hands out memory and reaches the devices through their drivers.
    y = level(DRAM)
    out.append(arrow(OS_X + OS_W + 2, y, left(DRAM) - 2, y, INK, 2))
    out.append(text(OS_GAP_MID, y - 8, "分配内存", 14, INK))
    for row in (SSD, NIC):
        out.append(arrow(OS_X + OS_W - 6, level(row), left(row) - 2, level(row), INK, 2))
    # It cannot change what the caches inside the hardware keep.
    y = level(SRAM)
    out.append(arrow(OS_X + OS_W + 2, y, left(SRAM) - 2, y, RED, 2, dash="5 4"))
    x, arm = OS_GAP_MID, 7
    out.append(line(x - arm, y - arm, x + arm, y + arm, RED, 2.6))
    out.append(line(x - arm, y + arm, x + arm, y - arm, RED, 2.6))
    out.append(text(x, y - 12, "无法修改", 15, RED, "bold"))
    out += speed(BOTTOM)
    # Legend under the columns, beside the remote storage.
    y = REMOTE[0] + REMOTE[1] / 2 - 18
    out.append(rect(APP_X, y, 28, 18, WHITE, RED, rx=3, width=1.6, dash="5 4"))
    out.append(text(APP_X + 38, y + 15, "硬件内部未暴露的缓存", 15, INK, anchor="start"))
    out.append(text(APP_X + 38, y + 37, "保留哪些数据由硬件决定", 15, INK, anchor="start"))
    return out, BOTTOM + 6


def resource_cache():
    out = column(APP_X, APP_W, TOP, FRAME_BOTTOM, "应用") + column(OS_X, OS_W, TOP, FRAME_BOTTOM, "操作系统")
    # The two games stand together in the middle of the column, each an icon and its label.
    x = APP_X + (APP_W - ICON) / 2
    top, step = centred(2, ICON + 22, 20)
    for y, name, label in ((top, "honor-of-kings", "王者荣耀"), (top + step, "genshin-impact", "原神")):
        out.append(icon(x, y, name))
        out.append(text(APP_X + APP_W / 2, y + ICON + 18, label, 14, INK, "bold"))
    out += stack()
    out += device(SRAM, "SRAM：L1 至 L3 缓存", FILL_BLUE, INK)
    out += device(DRAM, "DRAM：内存", FILL_GREEN, GREEN, empty=True)
    # The resource files, in the cloud and on the SSD.
    out += device(SSD, "SSD", FILL_GREY, MUTED) + chip(SSD, "资源文件", FILL_ORANGE, ORANGE)
    out += device(NIC, "网卡", FILL_GREY, MUTED)
    out += device(REMOTE, "云端服务器", FILL_GREY, MUTED, "7 5")
    out += chip(REMOTE, "资源文件", FILL_ORANGE, ORANGE)
    out += driver(SSD, "SSD 驱动程序") + driver(NIC, "网卡驱动程序")
    app_right, os_right, chip_left = APP_X + APP_W, OS_X + OS_W, CX - 46
    # 1: from the cloud through the network card to the game.
    out.append(arrow(CX, REMOTE[0] - 2, CX, NIC[0] + NIC[1] + 2, ORANGE, 2.6))
    y = level(NIC)
    out.append(arrow(left(NIC) - 2, y, os_right + 2, y, ORANGE, 2.6))
    out.append(arrow(OS_X - 2, y, app_right + 2, y, ORANGE, 2.6))
    out.append(text(GAP_MID, y + 21, "① 下载", 15, ORANGE, "bold"))
    # 2: from the game to the SSD.
    y = level(SSD) - 9
    out.append(arrow(app_right + 2, y, OS_X - 2, y, ORANGE, 2.6))
    out.append(arrow(os_right - 6, y, chip_left - 2, y, ORANGE, 2.6))
    out.append(text(GAP_MID, y - 9, "② 写入", 15, ORANGE, "bold"))
    # 3: on later runs, from the SSD to the game.
    y = level(SSD) + 9
    out.append(arrow(chip_left - 2, y, os_right + 2, y, GREEN, 2.6))
    out.append(arrow(OS_X - 2, y, app_right + 2, y, GREEN, 2.6))
    out.append(text(GAP_MID, y + 21, "③ 之后读取", 15, GREEN, "bold"))
    out += speed(BOTTOM)
    return out, BOTTOM + 6


def file_cache():
    """Without the network: the figure stands beside the listing, so it is as short as it can be."""
    frame_bottom = SSD[0] + SSD[1] + 10
    bottom = frame_bottom
    os_top = level(DRAM) + 24
    out = column(APP_X, APP_W, TOP, bottom, "应用") + column(OS_X, OS_W, os_top, bottom, "操作系统")
    # The program beside the figure is written in C.
    out.append(icon(APP_X + (APP_W - ICON) / 2, (TOP + bottom - ICON) / 2, "c-language"))
    out += stack(frame_bottom)
    out += device(SRAM, "SRAM：L1 至 L3 缓存", FILL_BLUE, INK)
    title_x = left(DRAM) + 12
    chip_x = CX + 46
    out += device(DRAM, "DRAM：内存", FILL_GREEN, GREEN, title_x=title_x)
    out += chip(DRAM, "buf", FILL_ORANGE, ORANGE, cx=chip_x, width=80, font=MONO)
    out += device(SSD, "SSD", FILL_GREY, MUTED, title_x=left(SSD) + 12)
    out += chip(SSD, "文件", WHITE, MUTED, cx=chip_x, width=80)
    out += driver(SSD, "SSD 驱动程序")
    app_right, os_right = APP_X + APP_W, OS_X + OS_W
    # 1: read asks the operating system, and the whole file goes from the SSD to buf.
    y = level(SSD)
    out.append(arrow(app_right + 2, y, OS_X - 2, y, ORANGE, 2.6))
    out.append(arrow(os_right - 6, y, chip_x - 42, y, ORANGE, 2.6))
    out.append(text(GAP_MID, y - 9, "① read", 15, ORANGE, "bold", font=MONO))
    out.append(arrow(chip_x, SSD[0] + CHIP_DY - 2, chip_x, DRAM[0] + CHIP_DY + CHIP_H + 2, ORANGE, 2.6))
    out.append(text(chip_x - 10, SSD[0] + 24, "整个文件", 14, ORANGE, "bold", "end"))
    # 2: later reads are loads, without the operating system.
    y = level(DRAM)
    out.append(arrow(chip_x - 42, y, app_right + 2, y, GREEN, 2.6))
    mid = (app_right + left(DRAM)) / 2
    out.append(text(mid, y - 31, "② 之后的读取：一次内存读取", 15, GREEN, "bold"))
    out.append(text(mid, y - 10, "不经过操作系统", 15, GREEN, "bold"))
    out += speed(bottom)
    return out, bottom + 6


if __name__ == "__main__":
    body, height = software_layers()
    save("software-layers", W, height, body)
    body, height = file_cache()
    save("file-cache", W, height, body, defs=(ICON_CLIP,))
    body, height = resource_cache()
    save("resource-cache", W, height, body, defs=(ICON_CLIP,))
