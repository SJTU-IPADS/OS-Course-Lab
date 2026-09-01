# One theme, every target

Style is defined once and read everywhere. This page is for someone changing
the look of the deck, or adding a renderer; authoring a lecture never needs it.

## Tokens live in the theme's `:root`

`themes/basic-office.css` is the deck's Marp theme, and its `:root` block is the
single source of every visual constant: the palette, the font stacks, the single
faces PowerPoint should pick out of them (`--pptx-font-cjk`,
`--pptx-font-mono`), and the geometry of a `<mark>` stroke.

Every renderer reads those custom properties through `lecturekit/tokens.py`.
No renderer spells a colour, a font, or a stroke height out — a test
(`tests/test_tokens.py`) refuses a hex literal anywhere under `lecturekit/`.

`themes/` lives beside the package, not inside it, so lecturekit runs from a
checkout (`pip install -e .` or `PYTHONPATH`), never from a wheel.

## Two translations on the way out

**Alpha.** The deck paints a marker stroke translucent so the word shows
through. The book, PowerPoint, and the printed transcript sheet paint on white
with no alpha, so `tokens.opaque(...)` composites the same colour onto white for
them.

**Coordinates.** The deck's stroke is `--mark-top` / `--mark-bottom`, fractions
of an inline box. LaTeX has no inline box, so the theme also states the stroke
as `--mark-tex-raise` / `--mark-tex-height` in `ex` — two numbers, one stroke,
both in the theme. PowerPoint's text highlight is full-height and takes no
arguments, so there the stroke is the ink alone.

## Targets that cannot `var()`

The LaTeX preamble and the transcript sheet (`themes/transcript.css`) cannot
read a CSS custom property. They write `@--token-name@` instead, and
`tokens.substitute(...)` fills it in, in the spelling that target wants.

## Renderers

One `Lecture → Section → Page` tree, read by several renderers. Each renders
only the blocks it knows, so a page is authored once.

| Target | Command | Output |
| --- | --- | --- |
| viewer | `render` / `view` | Marp deck in an outline shell (+ `--pdf`, `--png`) |
| pptx | `render --to pptx` | editable PowerPoint, styled from the viewer theme |
| book | `book` | one `ctexbook` document, one chapter per lecture |

`RENDERERS` in `lecturekit/renderers/` is the `--to` registry the first two
live in, and the extension point for a new deck target. `book` is its own
command, and the notebook path (`Lecture(assets=".")`) reuses the viewer's Marp
pipeline to display a single slide as a cell output.

Two renderers are deliberately approximate, because neither has a browser to
measure text with: PPTX skips the geometry-heavy blocks (architecture diagrams,
annotation bubbles, float-image wrap), and the book draws architecture as
stacked boxes.

## Screenshots

`docs/images/` is rendered from `examples/showcase` by the ordinary pipeline;
`scripts/screenshots.sh` re-shoots all of them. The README shows what the
renderers do now, not what they did the day someone took a screenshot. The
PowerPoint shot is macOS Quick Look's rendering, which draws the slide but not
PowerPoint's native text highlight.
