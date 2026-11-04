#!/usr/bin/env python3
"""Invalidating KV cache and recomputing on demand in three frames (modified from kv_expire.py):
(1) Dense requests arrive while all existing sessions in GPU memory are still unexpired;
(2) Directly invalidate one session's KV cache to free GPU memory for the incoming session;
(3) When the invalidated session resumes later, recompute the k, v of its earlier tokens.
Run it to refresh ../assets/kv-recompute-1..3.svg.
"""

from kv_kit import *
from svgkit import box

W, H = 960, 296
CAPTION_Y, LEGEND_Y = 242, 276


def slot(x, y, w, h, title, detail, kind, right_color=None):
    fill, stroke = KINDS[kind]
    dash = "5 4" if kind in ("empty", "gone") else None
    tone = MUTED if kind in ("empty", "gone") else INK
    rc = right_color or (RED if kind == "waste" else MUTED)
    rw = "bold" if right_color or kind == "waste" else "normal"
    return [rect(x, y, w, h, fill, stroke, rx=5, width=1.6, dash=dash),
            text(x + 16, y + h / 2 + 6, title, 16, tone, "bold", "start"),
            text(x + w - 16, y + h / 2 + 6, detail, 14, rc, rw, "end")]


def vram_box():
    return [rect(48, 18, 434, 184, WHITE, INK, rx=8, width=2.0),
            text(66, 44, "显存中的 KV Cache（容量已满）", 17, INK, "bold", "start")]


def frame1():
    out = vram_box()
    out += slot(66, 56, 398, 38, "会话 1", "距上次访问：5 分钟", "cache")
    out += slot(66, 102, 398, 38, "会话 2", "距上次访问：2 分钟", "cache")
    out += slot(66, 148, 398, 38, "会话 3", "距上次访问：1 分钟", "cache")
    out += box(548, 66, 356, 96, "新请求（会话 4）到达",
               sub="各会话均未过期，显存已无剩余空间",
               fill=FILL_ORANGE, stroke=ORANGE, size=17, sub_size=14)
    out.append(text(W / 2, CAPTION_Y,
                    "① 请求密集到达，显存中各会话的 KV Cache 均未过期，无空间容纳会话 4",
                    19, INK, "bold"))
    items = [("cache", "未过期的 KV Cache"), ("new", "新到达的会话请求")]
    out += legend((W - legend_width(items)) / 2, LEGEND_Y, items)
    return out


def frame2():
    out = vram_box()
    out += slot(66, 56, 398, 38, "会话 1（使缓存失效）", "释放显存 → 放入会话 4", "gone", RED)
    out += slot(66, 102, 398, 38, "会话 2", "距上次访问：2 分钟", "cache")
    out += slot(66, 148, 398, 38, "会话 3", "距上次访问：1 分钟", "cache")
    out.append(arrow(486, 75, 542, 75, GREEN, 2.2))
    out += box(548, 66, 356, 96, "直接使会话 1 的缓存失效",
               sub="立即释放显存空间，供会话 4 使用",
               fill=FILL_GREEN, stroke=GREEN, size=17, sub_size=14)
    out.append(text(W / 2, CAPTION_Y,
                    "② 显存不足时直接使部分 KV Cache 失效（invalidate），腾出显存空间",
                    19, INK, "bold"))
    items = [("cache", "保留的 KV Cache"), ("gone", "直接使失效并释放"), ("reuse", "回收显存空间")]
    out += legend((W - legend_width(items)) / 2, LEGEND_Y, items)
    return out


def frame3():
    out = vram_box()
    out += slot(66, 56, 398, 38, "会话 1（再次继续）", "写入重新计算的 k、v", "new", ORANGE)
    out += slot(66, 102, 398, 38, "会话 2", "保留在显存", "cache")
    out += slot(66, 148, 398, 38, "会话 3", "保留在显存", "cache")
    out.append(arrow(542, 75, 486, 75, ORANGE, 2.2))
    out += box(548, 66, 356, 96, "会话 1 再次发来请求",
               sub="重新计算前序 token 的 k、v，生成新缓存",
               fill=FILL_ORANGE, stroke=ORANGE, size=17, sub_size=14)
    out.append(text(W / 2, CAPTION_Y,
                    "③ 被使失效的会话再次继续时，服务端重新计算前序 token 的 k、v",
                    19, INK, "bold"))
    items = [("cache", "保留的 KV Cache"), ("new", "重新计算前序 k、v 并写入显存")]
    out += legend((W - legend_width(items)) / 2, LEGEND_Y, items)
    return out


if __name__ == "__main__":
    save("kv-recompute-1", W, H, frame1())
    save("kv-recompute-2", W, H, frame2())
    save("kv-recompute-3", W, H, frame3())
