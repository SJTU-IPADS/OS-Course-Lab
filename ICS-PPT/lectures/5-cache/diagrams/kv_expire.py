#!/usr/bin/env python3
"""KV cache expiration time (TTL) in three frames:
(1) The server tracks how long each session's KV cache has been idle;
(2) Once a session exceeds the expiration threshold (e.g. 1 hour), its KV cache is freed;
(3) If that session resumes later, the server recomputes the k, v of its earlier tokens.
Run it to refresh ../assets/kv-expire-1..3.svg.
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
            text(66, 44, "显存中的 KV Cache（过期时间：1 小时）", 17, INK, "bold", "start")]


def frame1():
    out = vram_box()
    out += slot(66, 56, 398, 38, "会话 1", "距上次访问：45 分钟", "cache")
    out += slot(66, 102, 398, 38, "会话 2", "距上次访问：10 分钟", "cache")
    out += slot(66, 148, 398, 38, "会话 3", "距上次访问：2 分钟", "cache")
    out += box(548, 66, 356, 96, "计时器监控各会话的闲置时间",
               sub="未超过 1 小时：KV Cache 保留在显存中",
               fill=FILL_GREY, stroke=MUTED, size=17, sub_size=14)
    out.append(text(W / 2, CAPTION_Y,
                    "① 服务端为每个会话的 KV Cache 设定过期时间（如 1 小时），记录闲置时长",
                    19, INK, "bold"))
    items = [("cache", "未过期的 KV Cache"), ("mid", "过期计时监控")]
    out += legend((W - legend_width(items)) / 2, LEGEND_Y, items)
    return out


def frame2():
    out = vram_box()
    out += slot(66, 56, 398, 38, "会话 1（闲置满 1 小时）", "到期失效 → 释放显存", "gone", RED)
    out += slot(66, 102, 398, 38, "会话 2", "距上次访问：25 分钟", "cache")
    out += slot(66, 148, 398, 38, "会话 3", "距上次访问：17 分钟", "cache")
    out.append(arrow(486, 75, 542, 75, GREEN, 2.2))
    out += box(548, 66, 356, 96, "释放的显存空间可供新会话使用",
               sub="避免已结束的会话永久占用显存",
               fill=FILL_GREEN, stroke=GREEN, size=17, sub_size=14)
    out.append(text(W / 2, CAPTION_Y,
                    "② 会话 1 超过 1 小时无新请求，其 KV Cache 自动失效并释放显存",
                    19, INK, "bold"))
    items = [("cache", "未过期的 KV Cache"), ("gone", "到期失效并释放"), ("reuse", "回收显存空间")]
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
                    "③ 过期后若同一会话再次继续，服务端重新计算前序 token 的 k、v",
                    19, INK, "bold"))
    items = [("cache", "未过期的 KV Cache"), ("new", "重新计算前序 k、v 并写入显存")]
    out += legend((W - legend_width(items)) / 2, LEGEND_Y, items)
    return out


if __name__ == "__main__":
    save("kv-expire-1", W, H, frame1())
    save("kv-expire-2", W, H, frame2())
    save("kv-expire-3", W, H, frame3())
