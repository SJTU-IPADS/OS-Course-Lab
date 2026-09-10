# The book target

Many lectures, one book. `lecturekit book` renders a course into a LaTeX tree
that compiles to a PDF: **each lecture becomes a chapter**. For authoring a
single lecture see [dsl.md](dsl.md); for the deck pipeline see
[usage.md](usage.md).

## Two bodies, one tree

A slide is projected bullets; a textbook wants paragraphs. So a page carries
**two parallel bodies**, and each renderer sees only its own:

| block | deck | book |
| --- | --- | --- |
| `p.slide(...)` | ✅ | ❌ |
| `p.prose(...)` | ❌ | ✅ |
| everything else (`image`, `table`, `sidenote`, `code`, `news`, …) | ✅ | ✅ |

Figures, tables, sidenotes and code are authored **once** and shared. Only the
running text is written twice, because it is genuinely different writing.

```python
def why_now(p):
    p.title("量变引起质变：指数 Scale 的魔力")
    p.slide("AI 超过人类 可能是瞬间的")            # deck only
    p.prose("""
指数增长的直觉很差。每天提升 1%，一年后是 37 倍 —— 这不是修辞，
而是 $1.01^{365} \\approx 37.8$ 的算术结果。
""")                                              # book only
    p.image("assets/intelligence-explosion.png")  # both
```

To reuse one slide's text as that page's prose, force it in:

```python
p.slide("这段话讲义和书里一字不差", only=["latex"])
```

That forced-in slide is also the only way an inline
[`<mark>`](dsl.md#inline-highlight-mark) reaches the book — the tag is legal in
slide text alone, and a slide is otherwise deck-only. It renders as the same
marker stroke the deck draws.

(`p.handout(...)` is a deprecated alias for `p.prose(...)`.)

## `book.py`

The book is an *ordering*, not a document. It carries no content of its own
beyond front matter:

```python
# book.py, at the root of a course
from lecturekit.book import Book

book = Book(title="从算法到系统：一个关于 Scaling 的故事", author="Xingda Wei")
book.preface("这本书讲的是 …")

book.lecture("01-introduction")
book.lecture("02-a-story-of-scaling")
```

- `Book(*, title, author=None, subtitle=None)`.
- `book.preface(text)` — optional; renders as an unnumbered `\chapter*`.
- `book.lecture(dir)` — append a lecture directory, resolved relative to
  `book.py`. Order is call order, and a lecture that is not listed is not in the
  book — that is how an unfinished one is excluded.

Each directory is loaded through the normal lecture pipeline, so a lecture is
authored once and read by both targets. Lecture ids must be unique across the
book.

## Running it

```bash
lecturekit book <book-dir>                     # -> build/book/
lecturekit book <book-dir> --out build/mybook
lecturekit book <book-dir> --stats             # coverage only, renders nothing
lecturekit book <book-dir> --lectures lec02    # one chapter, by lecture id
lecturekit book <book-dir> --compile           # also run latexmk (needs XeLaTeX)
```

From this repo's root, remember the `PYTHONPATH` prefix as with the other
commands:

```bash
PYTHONPATH=lecturekit python3 -m lecturekit.cli book "$BOOK"
```

The output tree:

```
build/book/
  book.tex                                 # preamble + \include of each chapter
  chapters/02-a-story-of-scaling.tex
  assets/lec02/intelligence-explosion.png
  Makefile                                 # latexmk -xelatex book.tex
```

Images are **copied** into `assets/<lecture-id>/`, namespaced by lecture id so
two lectures may ship the same basename.

### One chapter at a time

Only `book.tex` carries a preamble, so a chapter can be built alone two ways:

- `--lectures lec02` renders **only** that chapter into the output tree, so just
  its images are copied and the build is small and fast. It stands alone, but is
  renumbered as chapter 1 from page 1, and references to other chapters dangle.
  Use it while drafting.
- `\includeonly{chapters/lec02}` (before `\begin{document}`) in a fully rendered
  `book.tex` typesets only that chapter while reading the rest from `book.aux`,
  so it stays chapter 2 starting on page 34, with the table of contents and every
  cross-reference intact. Use it to check how a chapter lands in the finished
  book; delete the line and rerun `make` to get the whole book back.

Both name the **lecture id** — the `id=` passed to `Lecture(...)` — not the
directory name. For `\includeonly` the argument is the include path,
`chapters/<lecture-id>`; copy it straight out of the `\include{...}` line in
`book.tex`. A name that matches no `\include` is not an error: LaTeX just builds
a book with no chapters in it.

Rendering needs no TeX at all. Compiling needs **XeLaTeX** and `ctex` (the body
is Chinese) — on macOS, MacTeX provides both: `cd build/book && make`.

## Another language

`--lang en` renders the book in English:

```bash
lecturekit i18n extract <book-dir> --lang en    # -> <book-dir>/i18n/en.toml
lecturekit book <book-dir> --lang en            # -> build/book-en/
```

A book is an ordering, so its own overlay holds three keys — `book.title`,
`book.subtitle`, `book.preface`. Every chapter is translated through **its own
lecture's** `i18n/en.toml`, so a lecture translated for the deck is already
translated for the book. The chapter-end headings lecturekit prints itself
(参考文献, 延伸阅读, 动手试试, the 图 prefix) follow the language too. `--strict`
refuses to build a book with anything untranslated. See [i18n.md](i18n.md).

Every listed chapter needs at least a skeleton `i18n/<lang>.toml` (run
`i18n extract` on it) — a chapter with no overlay file at all is an error, not a
silent chapter of Chinese in an English book. `--lectures` is applied first, so
a one-chapter draft build only needs that chapter's.

## Unwritten pages, and `--stats`

A page with no `prose` keeps its heading and its figures, and prints a red
`TODO` box in place of the missing text. The book is therefore complete in
skeleton from day one and fills in over time.

A page that *had* prose and then held it back with `p.prose(...).disable()` (see
[dsl.md](dsl.md#block-visibility)) is a different case: it opted out of book
text, so it prints its figures with **no** `TODO` box and drops out of the
`--stats` denominator entirely — like a `book="skip"` page, but still visible. A
page never gets a `TODO` for text it deliberately disabled, only for text it
never wrote.

`--stats` reports how much is written, and renders nothing:

```
lec01  0/1 pages    0%
lec02  12/46 pages  26%
---------------------------
total  12/67 pages  18%
```

## Tree mapping

| lecture AST | LaTeX |
| --- | --- |
| `Lecture` | `\chapter` |
| `Section` | `\section` (nested: `\subsection`, `\subsubsection`, `\paragraph`) |
| `Page` | one level below its parent section |
| a top-level `Page` (e.g. `lec.close(...)`) | `\section` |

Nesting deeper than `\paragraph` is a `ValidationError`. Page titles are
markdown, exactly as in the deck.

### When pages and book sections don't line up

The deck often spends several pages on one textbook topic, and a slide title is
often the wrong book heading. Two page-level knobs (inert everywhere but the
book) bend the mapping:

- `book_title="…"` — the book prints this heading instead of `p.title(...)`.
- `book="merge"` — no heading of its own: the page's book-visible blocks and
  news append to the previous page's section. A run of merged pages is **one**
  book section (one TODO check, one unit in `--stats`).
- `book="skip"` — the page is not in the book at all. Use it on the second
  page of an annotation-reveal pair, which would otherwise repeat its figures.

An [animation](dsl.md#frames-an-animation) needs neither knob: a page carrying
`p.frames(...)` is one book section printing its **last** frame — the finished
picture — and the earlier frames skip themselves. It is one TODO check and one
unit in `--stats`, and the page's `book`/`book_title` apply to that section as
usual.

A [bridge page](dsl.md#the-tree) (`lec.bridge(...)`) is likewise forced to
`book="skip"` with no say in it: its line of text is projector rhetoric, and the
book's transitions live in the prose itself. It is out of the `--stats`
denominator like any skipped page.

A [review page](dsl.md#review-replaying-another-lectures-pages) needs neither
knob either, and takes no say in it: `review_section(...)` forces `book="skip"`.
The deck replays those slides because a class benefits from the reminder; a
reader just turns back a chapter, so the book never reprints them, and they are
out of the `--stats` denominator.

```python
s.page(id="duel-chess-ai", body=duel_chess_ai,
       book_title="Case #1：Deep Blue 与国际象棋")
s.page(id="duel-chess-scale", body=duel_chess_scale, book="merge")
```

### Referencing figures from prose

Name a captioned figure with `ref="…"` (`image` / `row` / `architecture`,
keyword or chained `.ref(name)`), then cite it in `prose` / `sidenote` /
caption / footnote text as `[@name]` — the book renders `图~\ref{…}`, so the
reference survives floats and page reshuffles. `[@lecid:name]` reaches across
chapters. See [dsl.md](dsl.md#figure-refs-book-only) for the rules.

## Block mapping

| block | LaTeX |
| --- | --- |
| `prose` | body paragraphs |
| `image` | `figure` + `\includegraphics`; a caption adds `\caption` + `\label` |
| `frames` | one `figure`, showing the animation's last frame |
| `row` | one `figure` of `subfigure`s |
| `table` | `booktabs` `tabular`; `align` maps to the column spec |
| `code` | `lstlisting` |
| `link` | `\href` |
| `sidenote` | a `tcolorbox` with a linked title |
| `aside` | a small `quote` |
| `highlight` | `\lkhighlight`: a centered `\tcbox` framing a one-column `tabular` — rule and text in the tone, with the deck's drop shadow |
| `demo` | a `tcolorbox` titled 动手试试 |
| `architecture` | stacked `tabular` boxes; `flow` becomes an arrow |
| footnote | `\footnote` (on a figure: `\footnotemark` + `\footnotetext`) |
| `slide`, `notes`, `side_image`, `image_right`, annotations | skipped |

`p.news(...)` is page metadata, not a block: a chapter's news collects into an
unnumbered **延伸阅读** section at its end.

`p.cite(...)` is likewise page metadata: a chapter's citations are deduplicated
and collected into an unnumbered **参考文献** section at its end (the deck's
per-slide page backrefs are dropped — they mean nothing in print).

Two deliberate approximations, in the spirit of the pptx target:

- **`architecture` is a box drawing, not a diagram.** Layers stack as tabulars.
- **A figure LaTeX cannot embed** (an animated `.gif`, say — graphicx reads
  png/jpg/pdf/eps) renders as a labelled placeholder, and the CLI warns. Print
  has no place for an animation, and one such file should not fail the book.

## Markdown → LaTeX

The converter is deliberately **not pandoc**: no external binary, and it handles
exactly the inline vocabulary the DSL documents — `**bold**`, `*italic*`,
`` `code` ``, `[label](url)`, `$math$` — plus lists, paragraphs, and fenced code.

Math passes through untouched (it is already TeX), code spans escape only their
body, and a link's URL is never escaped. Everything else is escaped, so a literal
`100% & rising` or `a_b` is safe to write.

`p.prose(...)` is **not** autobolded — a paragraph has no headline. That rule
applies to `slide` alone.
