#!/usr/bin/env python3
"""Print the blank margin on each side of every figure.

A figure is placed on the slide by its canvas, so a drawing that sits off the
middle of its canvas sits off the middle of the page, and a canvas much larger
than the drawing makes the figure small. Reads every SVG named on the command
line (default: all of ../assets), rasterises it with rsvg-convert and writes
one line per figure: name, then the left, right, top and bottom margin in
canvas pixels, tab-separated.

    python3 margins.py | awk -F'\t' '$2 - $3 > 8 || $3 - $2 > 8'   # off-centre
"""

import io
import pathlib
import subprocess
import sys

from PIL import Image, ImageChops


def margins(path):
    png = subprocess.run(["rsvg-convert", "-b", "white", str(path)], check=True,
                         capture_output=True).stdout
    im = Image.open(io.BytesIO(png)).convert("RGB")
    ink = ImageChops.difference(im, Image.new("RGB", im.size, "white"))
    left, top, right, bottom = ink.point(lambda v: 255 if v > 12 else 0).getbbox()
    return left, im.width - right, top, im.height - bottom


def main(argv):
    paths = [pathlib.Path(a) for a in argv[1:]]
    if not paths:
        paths = sorted((pathlib.Path(__file__).resolve().parent.parent
                        / "assets").glob("*.svg"))
    for p in paths:
        print(p.name, *margins(p), sep="\t")


if __name__ == "__main__":
    main(sys.argv)
