#!/usr/bin/env python3
"""When dense concurrent requests exhaust GPU memory before any KV cache expires (frame 1),
the naive idea swaps an inactive session's KV cache out to main memory (frame 2) and swaps
it back in when that session resumes (frame 3).
Run it to refresh ../assets/kv-swap-1..3.svg.
"""

from kv_kit import *
from svgkit import box

W, H = 960, 296
CAPTION_Y, LEGEND_Y = 242, 276


def slot(x, y, w, h, title, detail, kind):
    fill, stroke = KINDS[kind]
    dash = "5 4" if kind in ("empty", "gone") else None
    tone = MUTED if kind in ("empty", "gone") else INK
    return [rect(x, y, w, h, fill, stroke, rx=5, width=1.6, dash=dash),
            text(x + 16, y + h / 2 + 6, title, 16, tone, "bold", "start"),
            text(x + w - 16, y + h / 2 + 6, detail, 14, RED if kind == "waste" else MUTED,
                 "bold" if kind == "waste" else "normal", "end")]


def two_boxes():
    return [rect(48, 18, 376, 184, WHITE, INK, rx=8, width=2.0),
            text(66, 44, "显存（容量小、速度快）", 17, INK, "bold", "start"),
            rect(576, 18, 336, 184, WHITE, GREEN, rx=8, width=2.0),
            text(594, 44, "内存（容量大、速度慢）", 17, GREEN, "bold", "start")]


def frame1():
    out = two_boxes()
    out += slot(66, 56, 340, 38, "会话 1", "距上次访问 3 分钟（未过期）", "cache")
    out += slot(66, 102, 340, 38, "会话 2", "距上次访问 1 分钟（未过期）", "cache")
    out += slot(66, 148, 340, 38, "会话 3", "正在生成（未过期）", "cache")
    out += box(438, 66, 124, 96, "会话 4 到达", sub="显存已满",
               fill=FILL_RED, stroke=RED, size=15, sub_size=13)
    out += slot(594, 56, 300, 38, "空闲内存空间", "", "empty")
    out += slot(594, 102, 300, 38, "空闲内存空间", "", "empty")
    out.append(text(W / 2, CAPTION_Y,
                    "请求密集到达时，显存中的 KV Cache 均未到期，新到达的会话 4 无法放入显存",
                    19, INK, "bold"))
    items = [("cache", "显存中未过期的 KV Cache"), ("waste", "密集到达的新请求"), ("empty", "空闲内存")]
    out += legend((W - legend_width(items)) / 2, LEGEND_Y, items)
    return out


def frame2():
    out = two_boxes()
    out += slot(66, 56, 340, 38, "会话 4（新到达）", "占用腾出的显存", "new")
    out += slot(66, 102, 340, 38, "会话 2", "保留在显存", "cache")
    out += slot(66, 148, 340, 38, "会话 3", "保留在显存", "cache")
    out.append(arrow(430, 75, 568, 75, ORANGE, 2.4))
    out.append(text(500, 60, "① 换出到内存", 16, ORANGE, "bold"))
    out.append(text(500, 98, "腾出显存给会话 4", 14, MUTED))
    out += slot(594, 56, 300, 38, "会话 1 的 KV Cache", "暂存于内存", "reuse")
    out += slot(594, 102, 300, 38, "空闲内存空间", "", "empty")
    out.append(text(W / 2, CAPTION_Y,
                    "朴素设想（换出）：把暂不使用的会话 1 的 KV Cache 复制到内存，腾出显存",
                    19, INK, "bold"))
    items = [("cache", "显存中的 KV Cache"), ("new", "新到达的会话"), ("reuse", "换出到内存的 KV Cache")]
    out += legend((W - legend_width(items)) / 2, LEGEND_Y, items)
    return out


def frame3():
    out = two_boxes()
    out += slot(66, 56, 340, 38, "会话 1（继续请求）", "等待从内存换入", "waste")
    out += slot(66, 102, 340, 38, "会话 2", "保留在显存", "cache")
    out += slot(66, 148, 340, 38, "会话 3", "保留在显存", "cache")
    out.append(arrow(568, 75, 430, 75, RED, 2.4))
    out.append(text(500, 60, "② 从内存换入显存", 16, RED, "bold"))
    out.append(text(500, 98, "跨总线传输大量数据", 14, RED, "bold"))
    out += slot(594, 56, 300, 38, "会话 1 的 KV Cache", "从内存读出", "waste")
    out += slot(594, 102, 300, 38, "会话 4 的 KV Cache", "换出到内存", "reuse")
    out.append(text(W / 2, CAPTION_Y,
                    "朴素设想（换入）：会话 1 继续请求时，把 KV Cache 从内存复制回显存",
                    19, INK, "bold"))
    items = [("cache", "显存中的 KV Cache"), ("reuse", "内存中的 KV Cache"), ("waste", "跨总线换入（耗时）")]
    out += legend((W - legend_width(items)) / 2, LEGEND_Y, items)
    return out


if __name__ == "__main__":
    save("kv-swap-1", W, H, frame1())
    save("kv-swap-2", W, H, frame2())
    save("kv-swap-3", W, H, frame3())
