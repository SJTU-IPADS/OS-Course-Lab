#!/usr/bin/env bash
# Re-shoot docs/images/ from examples/showcase.
#
#   scripts/screenshots.sh
#
# The README's screenshots are output, not artwork: every one of them is
# rendered from examples/showcase by the same pipeline a lecture goes through,
# so a change to the theme or a renderer is one command away from being visible
# in the README. Nothing here is hand-edited or hand-cropped.
#
# Each shot needs the tool its target needs, and is skipped (with a note) when
# that tool is missing: Node for the deck, a local Chrome for the PNG and
# outline shots, XeLaTeX + poppler for the book page, macOS Quick Look for the
# PowerPoint preview. ImageMagick, if present, trims the outline shot.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

LEC=examples/showcase
OUT=docs/images
BUILD=build/screenshots
PY="${PYTHON:-python3}"
CHROME="${CHROME_PATH:-/Applications/Google Chrome.app/Contents/MacOS/Google Chrome}"

lecturekit() { PYTHONPATH="$ROOT" "$PY" -m lecturekit.cli "$@"; }
skip() { echo "screenshots: skipping $1 ($2)" >&2; }

mkdir -p "$OUT"
rm -rf "$BUILD"

# ---------------------------------------------------------------- the deck --
# One render of the whole lecture, then the slides worth showing are copied out
# by deck index (`inspect` prints the order; animations count one PNG a frame).
echo "screenshots: rendering the deck …"
lecturekit render "$LEC" --out "$BUILD/viewer" --png >/dev/null

cp "$BUILD/viewer/slides.002.png" "$OUT/slide-marks.png"     # marks, chip, marked code
cp "$BUILD/viewer/slides.008.png" "$OUT/slide-logging.png"   # the page shown in all 3 targets
cp "$BUILD/viewer/slides.009.png" "$OUT/slide-architecture.png"

# The animation (slides 4-6) is three slides carrying one text, so showing one
# of them shows nothing. The strip stacks the figure band of each frame: the
# band is a fixed window on the slide, cut by the same numbers every time, so
# the three read as one figure advancing rather than as three screenshots.
if command -v magick >/dev/null 2>&1; then
  magick "$BUILD/viewer/slides.004.png" "$BUILD/viewer/slides.005.png" \
         "$BUILD/viewer/slides.006.png" -crop 1280x300+0+180 +repage -append \
         "$OUT/frames-strip.png"
else
  skip frames-strip.png "no ImageMagick"
fi

# ------------------------------------------------------------- the outline --
# The viewer shell is the outline, so it is a page in its own right: shot at 2x
# and trimmed to its content, because the deck it lists is only nine slides.
if [ -x "$CHROME" ]; then
  "$CHROME" --headless=new --disable-gpu --hide-scrollbars \
    --force-device-scale-factor=2 --window-size=1280,860 \
    --virtual-time-budget=3000 \
    --screenshot="$BUILD/outline.png" "file://$ROOT/$BUILD/viewer/index.html" 2>/dev/null
  if command -v magick >/dev/null 2>&1; then
    magick "$BUILD/outline.png" -trim +repage \
      -bordercolor white -border 48 -resize 1280 "$OUT/outline.png"
  else
    cp "$BUILD/outline.png" "$OUT/outline.png"
  fi
else
  skip outline.png "no Chrome at $CHROME — set CHROME_PATH"
fi

# ---------------------------------------------------------------- the book --
# The same lecture as a chapter. The page carrying the logging section is found
# by a sentence of its prose rather than by number, so adding a paragraph
# earlier in the chapter does not silently re-shoot the wrong page — and a
# sentence of prose, unlike the heading, cannot also match the table of
# contents.
if command -v xelatex >/dev/null 2>&1 && command -v pdftoppm >/dev/null 2>&1; then
  echo "screenshots: compiling the book …"
  lecturekit book examples --out "$BUILD/book" --compile >/dev/null
  page=""
  if command -v pdftotext >/dev/null 2>&1; then
    for n in $(seq 1 30); do
      if pdftotext -f "$n" -l "$n" "$BUILD/book/book.pdf" - 2>/dev/null |
         grep -q "Logging inverts the shadow copy"; then page="$n"; break; fi
    done
  fi
  if [ -z "$page" ]; then
    skip book-page.png "could not find the logging section in book.pdf"
  else
    pdftoppm -png -r 110 -f "$page" -l "$page" -singlefile \
      "$BUILD/book/book.pdf" "${OUT%/}/book-page"
  fi
else
  skip book-page.png "needs xelatex and pdftoppm (MacTeX, poppler)"
fi

# ---------------------------------------------------------------- the pptx --
# Exported with `--pages logging` so the slide of interest is slide 1, which is
# all Quick Look renders. Quick Look is not PowerPoint: it drops the text
# highlight `<mark>` becomes, which the file does carry.
if command -v qlmanage >/dev/null 2>&1; then
  echo "screenshots: exporting PowerPoint …"
  lecturekit render "$LEC" --to pptx --out "$BUILD/pptx" --pages logging >/dev/null
  qlmanage -t -s 1600 -o "$BUILD" "$BUILD"/pptx/*.pptx >/dev/null 2>&1
  shot="$(ls "$BUILD"/*.pptx.png 2>/dev/null | head -1)"
  if [ -n "$shot" ]; then cp "$shot" "$OUT/pptx-slide.png"; else
    skip pptx-slide.png "Quick Look produced no thumbnail"
  fi
else
  skip pptx-slide.png "no qlmanage (macOS only)"
fi

echo "screenshots: wrote $OUT"
ls -1 "$OUT"
