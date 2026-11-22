# One theme, every target

Style is defined once and read everywhere. This page is for someone changing
the look of the deck, or adding a renderer; authoring a lecture never needs it.

## Tokens live in two places

Every visual constant is a custom property, with one home:

- a colour is a colour scheme's: `themes/schemes/<name>.css`, one file each;
- what every scheme shares is in the `:root` of the deck's Marp theme,
  `themes/basic-office.css`: the font stacks, the single faces PowerPoint should
  pick out of them (`--pptx-font-cjk`, `--pptx-font-mono`), the geometry of a
  `<mark>` stroke, and the geometry of a page: the slide's padding
  (`--slide-padding-top`, `--slide-padding-x`, `--slide-padding-bottom`), the
  footnote band's least height (`--footnote-band-min`), the gap between the
  image column and the main column (`--side-gap`) and the gutter of a flex row
  (`--layout-gutter`).

Every renderer reads those custom properties through `lecturekit/tokens.py`,
which gives the theme's and the default scheme's, `light`, as one palette;
`tokens.px(name)` reads a length in pixels.
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

## Colour schemes

The viewer's web pages take a [colour scheme](usage.md#colour-scheme), and
every export is `light`. A scheme is data: one `:root` block with the value of
each colour token, and `color-scheme: light` or `dark` for the ground it is
built on. It has no rules. The rules are written once, over the tokens, in
`basic-office.css` and the viewer's assets, so the slides, the outline, the
scrollbar and the live preview's panels all follow the one file.

`lecturekit/renderers/viewer/colorscheme.py` decides what a page gets of the
scheme chosen, a fragment for the top of its `<head>`. The renderer writes it
to `head.html` in the bundle, and the deck build and the dev server both put
it in from there.

### Adding a colour scheme

1. Copy `themes/schemes/light.css`, or `dark.css` for a dark ground, to
   `themes/schemes/<name>.css`.
2. Change the values. Keep every token and add none: a file that lacks one or
   has another is refused, by name.
3. Render with `--color-scheme <name>` and look at the pages. `pytest` already
   covers the file.

A value may name another token (`var(--color-bg)`). The `--adapt-*` tokens
say what the ground needs, with `initial` or `transparent` for nothing. A
figure is usually black ink on transparency, so a dark ground puts a sheet
behind each picture, and gives what can lie over one (a footnote marker, a
chip, the page number, text that a tall figure runs under) the page's own
ground. A picture [drawn again for the scheme](dsl.md#pictures-for-a-colour-scheme)
gets no sheet. The page number's ground is a colour alone, and there is none
on a page that keeps `light` over a background picture: its ink is dark.

To add a colour to the theme, make it a token in every scheme and write
`var(--token)` in the rule. A colour written in a lecture's own HTML is
outside the palette and shows as written in every scheme.

## Renderers

One `Lecture → Section → Page` tree, read by several renderers. Each renders
only the blocks it knows, so a page is authored once.

| Target | Command | Output |
| --- | --- | --- |
| viewer | `render --to html` (also `pdf`, `png`), `view` | Marp deck in an outline shell, exported to PDF or PNG |
| pptx | `render --to pptx` | editable PowerPoint, styled from the viewer theme |
| transcript | `render --to transcript` (`transcript-pdf` for the PDF) | one printable A4 sheet, styled by `themes/transcript.css` from the same palette |
| book | `book` | one `ctexbook` document, one chapter per lecture |

Text is parsed once. PowerPoint and the transcript sheet read it as the deck
does, through `lecturekit/markup/`, and then only draw:

```
field text ─ markup/fields ─> the deck's markdown or HTML ──────────────> Marp (the deck)
                                  │
                                  └─ markup/filter.cjs ─> HTML + MathML
                                     (the deck's engine)        │
                                                         markup/document
                                                    (a tree that knows no target)
                                                      ┌─────────┴─────────┐
                                                    pptx              transcript
                                             DrawingML + OMML       HTML + MathML
```

| Module | Its one job |
| --- | --- |
| `markup/fields.py` | which markdown or HTML each field becomes, for the deck and for the parse |
| `markup/filter.cjs` | a filter: markdown in, the deck's HTML out, formulas as MathML |
| `markup/engine.py` | that filter as a function, one process for a render |
| `markup/document.py` | HTML into a small tree of blocks and inlines |
| `markup/__init__.py` | `Reader`: a field in, its tree out |
| `renderers/pptx/text.py` | the tree as paragraphs and runs |
| `renderers/pptx/omml.py` | MathML as OMML, one function for each element |
| `renderers/pptx/draw.py` | the drawers that elements and slide text share: text, table, listing, pictures, columns |
| `renderers/pptx/page.py` | one slide: the bands, the columns, footnotes and bubbles |
| `renderers/transcript/text.py` | the tree as the sheet's HTML |
| `renderers/transcript/mathml.py` | MathML as browsers' MathML Core reads it |

A change of syntax is made in `markup/` and reaches both. A renderer holds no
markdown, HTML or TeX grammar. The book is the exception: TeX sets its
formulas, and `renderers/latex/text.py` reads the markdown subset
[book.md](book.md#markdown--latex) lists.

`RENDERERS` in `lecturekit/renderers/` is the `--to` registry the first three
live in, and the extension point for a new deck target. The renderers hold no
table of block kinds: each one walks the page and asks every block to draw itself
(see [Elements](#elements)). `book` is its own
command, and the notebook path (`Lecture(assets=".")`) reuses the viewer's Marp
pipeline to display a single slide as a cell output.

Two renderers are deliberately approximate in layout, because neither has a
browser to measure text with: PPTX estimates every height from the text, draws
an architecture diagram from computed widths rather than flexbox, and floats
an `image_right` figure beside a narrowed text box instead of wrapping text
around it; the book draws architecture as stacked boxes. The content of a PPTX
slide is exact, and `scripts/check-pptx.py` checks it
([usage.md](usage.md#checking-an-export-against-the-deck)).

## Elements

Every block on a page is an instance of a class under `lecturekit/elements/`,
one module per kind (`image.py`, `table.py`, …), all derived from
`model.Block`. `Block(kind="table", content=...)` builds a `Table`, and a kind
no class registered is refused by validation. The base class carries what every
element shares: `halign`, `valign`, `footnotes` and `annotations`, so any element
takes a footnote and an alignment without a method of its own.

A class says everything its kind knows about itself:

| What | Where |
| --- | --- |
| the targets that draw it, what it may carry | class constants `TARGETS`, `REF`, `MARKS`, `FILE_LINKS`, `VERBATIM` |
| where it may use a deck variable, whether it is a control | `VARIABLE_KEYS` (content keys that may hold `$name`), `CONTROL` |
| how the viewer lays it out | `REVEAL_STEP`, `IN_FLOW`, `CENTERED`, `HOLDABLE`, `IN_CONTAINER` |
| how it validates its content | `check_content(page_id)` |
| how it is drawn | `to_marp` (deck: HTML, PDF, PNG), `to_pptx`, `to_latex`, `to_transcript` |
| where a footnote's superscript goes | `footnote_marker` |

A deck variable is spelled once per target before any block draws itself:
`variables.for_deck(lecture, figures)` for the Marp deck,
`variables.for_static(lecture)` for PowerPoint, the book and the transcript
sheet (see `lecturekit/variables.py`). A new target calls the one that fits it
at its entry, and its blocks see plain values. A bound figure needs nothing
from a static target: its file is drawn at the defaults. The controls' colours
are the `--control-*` tokens of the theme, and a control's default label, the
variable's symbol, is set in `--font-math`, the face the figures' math uses.
A character of a formula that MathJax has no glyph for, such as Chinese, is
set in `--font-base`, the body's.

A renderer calls the method of its target on each block of the page. Its
context carries the reader: `to_pptx` draws what `ctx.read` returns, and
`to_transcript(ctx)` writes it with the sheet it is given (`ctx.blocks`,
`ctx.inline`, `ctx.table`). A kind whose deck markdown is all content gets its
tree from that markdown with `self.read_deck(ctx.read)`. A method is
needed only for a target the class lists in `TARGETS`. The `renderers/*/blocks.py`
modules (and `renderers/pptx/draw.py`) hold the helpers several elements draw
with and nothing per kind. The page's layout is the renderer's, once
per page: its three bands (the title, the body, the footnote band) and the
body's columns (the main column, and the image column beside it). In the deck
each block, a side image included, draws itself into the column it belongs to.

A new kind of block is a new class: a module in `lecturekit/elements/` with
`@element`, imported in `lecturekit/elements/__init__.py`, plus the DSL method
that creates it (`docs/dsl.md`). Its renderers' imports stay inside the methods,
so python-pptx is not loaded unless a PowerPoint is rendered.

## Screenshots

`docs/images/` is rendered from `examples/showcase` by the ordinary pipeline;
`scripts/screenshots.sh` re-shoots all of them. The README shows what the
renderers do now, not what they did the day someone took a screenshot. The
PowerPoint shot is macOS Quick Look's rendering, which draws the slide but not
PowerPoint's native text highlight.
