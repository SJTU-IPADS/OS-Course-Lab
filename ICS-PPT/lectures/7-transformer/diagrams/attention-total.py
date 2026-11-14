#!/usr/bin/env python3
"""attention-total: the attention parameters of GPT-3 against the whole model.

Left, the running tally with embedding, key, query, value, output and
unembedding filled in (the MLP rows still "?"), the four attention rows
washed orange. Right, one bar of all 175,181,291,520 parameters: the
attention share 4 x 14,495,514,624 = 57,982,058,496 (about a third) in
orange, the rest, mostly the MLP blocks, in grey.
"""
from tk import *

H = 340
TOTAL = 175_181_291_520
ATT = 4 * 14_495_514_624
EMB = 2 * 617_558_016

b = []
els, th = tally(40, 26, {0, 1, 2, 3, 4, 7}, w=430, total=False, focus={1, 2, 3, 4})
b += els

bx, bw, by, bh = 540, 460, 150, 44
att_w = bw * ATT / TOTAL
emb_w = bw * EMB / TOTAL
# bracket over the whole bar
b.append(path(f"M {bx} {by - 10} L {bx} {by - 18} L {bx + bw} {by - 18} L {bx + bw} {by - 10}",
              INK, 1.6, head=False))
b.append(text(bx + bw / 2, by - 30, "GPT-3 参数总数 175,181,291,520", 20, INK, "bold"))
b.append(rect(bx, by, att_w, bh, FILL_ORANGE, ORANGE, rx=2, width=1.8))
b.append(rect(bx + att_w, by, emb_w, bh, DATA, DATA, rx=0, width=0.6))
b.append(rect(bx + att_w + emb_w, by, bw - att_w - emb_w, bh, FILL_GREY, LINE, rx=2, width=1.6))
b.append(text(bx + att_w / 2, by + bh / 2 + 7, "注意力", 19, ORANGE, "bold"))
b.append(text(bx + att_w + emb_w + (bw - att_w - emb_w) / 2, by + bh / 2 + 7,
              "其余主要在 MLP", 19, INK))

b.append(text(bx, by + bh + 36, "4 × 14,495,514,624", 19, ORANGE, anchor="start"))
b.append(text(bx, by + bh + 64, "= 57,982,058,496（≈ 1/3）", 19, ORANGE, "bold",
              anchor="start"))
save("attention-total", b, H)
