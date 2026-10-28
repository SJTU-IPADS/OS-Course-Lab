"""Geometry and layer partitions for the six-H200 teaching pipeline.

The target is the dense Llama 3.1 405B: 126 layers, 21 per stage.
Reuse the lecture's colours without changing the CPU figures' grid.
"""
from svgkit import BLUE, INK, MUTED, ORANGE, circle, mono, path, rect, text
from timegrid import TONE

GPU_COUNT, LAYER_COUNT, HIDDEN_SIZE = 6, 126, 16384
LAYERS_PER_GPU = LAYER_COUNT // GPU_COUNT
W, CH, CW, LX, TOP, PITCH = 1120, 34, 66, 204, 52, 40
BOTTOM = TOP + (GPU_COUNT - 1) * PITCH + CH
RETURN_GAP = 56
GPUS = [f"GPU {index + 1}（{index * LAYERS_PER_GPU + 1}–{(index + 1) * LAYERS_PER_GPU} 层）"
        for index in range(GPU_COUNT)]


def cell_x(column, return_gap=0, return_after=(GPU_COUNT,)):
    gaps = sum(column >= boundary for boundary in return_after)
    return LX + column * CW + return_gap * gaps


def cx(column, return_gap=0, return_after=(GPU_COUNT,)):
    return cell_x(column, return_gap, return_after) + CW / 2


def canvas_left(columns, return_gap=0, return_after=(GPU_COUNT,)):
    gaps = sum(0 < boundary < columns for boundary in return_after)
    return (24 + LX + columns * CW + return_gap * gaps) / 2 - W / 2


def cycle_numbers(count, emph=None, return_gap=0, return_after=(GPU_COUNT,)):
    out = [text(LX - 12, TOP - 10, "周期", 14, MUTED, anchor="end")]
    for column in range(count):
        emphasis = column + 1 == emph
        out.append(text(cx(column, return_gap, return_after), TOP - 10, str(column + 1),
                        16 if emphasis else 14, ORANGE if emphasis else MUTED,
                        "bold" if emphasis else "normal"))
    return out


def row(label, y, names, upto=None, return_gap=0, return_after=(GPU_COUNT,)):
    out = [mono(24, y + CH / 2 + 5, label, 14, INK)]
    for column, name in enumerate(names):
        if upto is not None and column >= upto:
            continue
        tone_name = name
        if name.startswith("请求 "):
            tone_name = f"请求 {(int(name.split()[-1]) - 1) % 4 + 1}"
        fill, stroke, tone = TONE[tone_name]
        x = cell_x(column, return_gap, return_after)
        out.append(rect(x + 2, y + 2, CW - 4, CH - 4, fill, stroke, rx=3,
                        width=1.2, dash="4 3" if name == "bubble" else None))
        out.append(text(x + CW / 2, y + CH / 2 + 5, name, 13, tone, "bold"))
    return out


def return_arrow(source_cycle=GPU_COUNT, color=BLUE, label="词 1 回传",
                 return_after=(GPU_COUNT,)):
    """Connect a completed step to its successor through a dedicated gutter."""
    source_column = source_cycle - 1
    target_column = source_column + 1
    source_x = cell_x(source_column, RETURN_GAP, return_after) + CW - 2
    target_x = cell_x(target_column, RETURN_GAP, return_after) + 2
    source_y = TOP + (GPU_COUNT - 1) * PITCH + CH / 2
    target_y = TOP + CH / 2
    gutter_x = source_x + 2 + RETURN_GAP / 2
    radius = 8
    route = (f"M {source_x + 3} {source_y} H {gutter_x - radius} "
             f"Q {gutter_x} {source_y} {gutter_x} {source_y - radius} "
             f"V {target_y + radius} "
             f"Q {gutter_x} {target_y} {gutter_x + radius} {target_y} "
             f"H {target_x - 4}")
    out = [path(route, color, width=1.8),
           circle(source_x + 3, source_y, 2.5, color, color, width=0),
           text(gutter_x, 20, label, 12, color, "bold")]
    for column, stage in [(source_column, GPU_COUNT - 1), (target_column, 0)]:
        out.append(rect(cell_x(column, RETURN_GAP, return_after) + 2, TOP + stage * PITCH + 2,
                        CW - 4, CH - 4, "none", color, rx=3, width=2.4))
    return out


def col_frame(column, y0, y1, color):
    return rect(LX + column * CW + 1, y0, CW - 2, y1 - y0,
                "none", color, rx=5, width=2, dash="6 4")
