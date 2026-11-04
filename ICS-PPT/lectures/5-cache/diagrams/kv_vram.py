#!/usr/bin/env python3
"""GPU memory layout during LLM inference (frame 1) and why GPU memory inevitably fills up
if every session's KV cache stays resident forever (frame 2).
Run it to refresh ../assets/kv-vram-1..2.svg.
"""

from kv_kit import *
from svgkit import box

W, H = 960, 336
GX, GY, GW, GH = 36, 48, 676, 172        # the GPU memory box
IY, IH = 94, 108                         # inner blocks: top, height
CAPTION_Y, LEGEND_Y = 274, 312


def base_vram(full=False):
    out = [rect(GX, GY, GW, GH, WHITE, INK, rx=8, width=2.2),
           text(GX + 16, GY + 28, "显存（GPU 内存）", 18, INK, "bold", "start")]
    out += box(GX + 16, IY, 168, IH, "模型权重", sub="固定大小，各请求共用",
               fill=FILL_PURPLE, stroke=PURPLE, size=17, sub_size=13)
    out += box(GX + 194, IY, 84, IH, "中间结果", sub="当前步",
               fill=FILL_GREY, stroke=MUTED, size=16, sub_size=13)
    kx, kw = GX + 290, 370
    out.append(rect(kx, GY + 14, kw, GH - 26, "none", BLUE, rx=6, width=1.8, dash="6 4"))
    out.append(text(kx + kw / 2, GY + 34, "KV Cache 区（按会话存放）", 15, BLUE, "bold"))
    if not full:
        sessions = [("会话 1", 98, "cache", "k、v"),
                    ("会话 2", 82, "cache", "k、v"),
                    ("会话 3", 90, "cache", "k、v"),
                    ("剩余空间", 68, "empty", "空闲")]
    else:
        sessions = [("会话 1", 98, "cache", "已暂停"),
                    ("会话 2", 82, "cache", "已暂停"),
                    ("会话 3", 90, "cache", "进行中"),
                    ("会话 4", 68, "new", "新加入")]
    x = kx + 10
    for name, w, kind, sub in sessions:
        fill, stroke = KINDS[kind]
        dash = "5 4" if kind == "empty" else None
        out.append(rect(x, IY, w, IH, fill, stroke, rx=5, width=1.6, dash=dash))
        tone = MUTED if kind == "empty" else INK
        out.append(text(x + w / 2, IY + IH / 2 - 4, name, 15, tone, "bold"))
        out.append(text(x + w / 2, IY + IH / 2 + 18, sub, 13, MUTED))
        x += w + 6
    return out


def frame1():
    out = base_vram(full=False)
    ox = GX + GW + 38
    out.append(arrow(ox - 6, IY + IH / 2, GX + GW + 4, IY + IH / 2, ORANGE, 2.2))
    out.append(rect(ox, IY, 96, IH, FILL_ORANGE, ORANGE, rx=5, width=1.8))
    out.append(text(ox + 48, IY + IH / 2 - 4, "会话 4", 15, INK, "bold"))
    out.append(text(ox + 48, IY + IH / 2 + 18, "放入剩余空间", 13, ORANGE, "bold"))
    out.append(text(ox + 48, GY + 34, "新到达的会话", 16, ORANGE, "bold"))
    out.append(text(W / 2, CAPTION_Y,
                    "显存同时存放模型权重、当前步中间结果与各会话的 KV Cache", 19, INK, "bold"))
    items = [("param", "模型权重"), ("mid", "当前步中间结果"),
             ("cache", "各会话的 KV Cache"), ("empty", "剩余显存")]
    out += legend((W - legend_width(items)) / 2, LEGEND_Y, items)
    return out


def frame2():
    out = base_vram(full=True)
    ox = GX + GW + 34
    out.append(arrow(ox - 4, IY + IH / 2, GX + GW + 4, IY + IH / 2, RED, 2.2))
    for i, name in enumerate(("会话 5", "会话 6")):
        bx = ox + i * 98
        out.append(rect(bx, IY, 90, IH, FILL_RED, RED, rx=5, width=1.8))
        out.append(text(bx + 45, IY + IH / 2 - 4, name, 15, INK, "bold"))
        out.append(text(bx + 45, IY + IH / 2 + 18, "放不下", 13, RED, "bold"))
    out.append(text(ox + 94, GY + 34, "后续不断到达的会话", 16, RED, "bold"))
    out.append(text(W / 2, CAPTION_Y,
                    "若旧会话的 KV Cache 始终驻留在显存中，随新会话不断积累，显存迟早被占满",
                    19, INK, "bold"))
    items = [("param", "模型权重"), ("cache", "长期驻留的 KV Cache"),
             ("new", "刚放入的会话"), ("waste", "显存已满，放不下")]
    out += legend((W - legend_width(items)) / 2, LEGEND_Y, items)
    return out


if __name__ == "__main__":
    save("kv-vram-1", W, H, frame1())
    save("kv-vram-2", W, H, frame2())
