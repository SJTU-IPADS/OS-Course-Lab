# The lecturekit DSL

The authoring surface: a small Python DSL that builds a `Lecture -> Section ->
Page` tree, where each page is a list of blocks drawn from a fixed vocabulary.
This is the reference for what you can write. For how to render and view what you
write, see [usage.md](usage.md).

A lecture source is a directory containing `lecture.py` (plus optional imported
modules like `pages.py` and an `assets/` folder). `lecture.py` constructs a
`Lecture`, fills it with sections and pages, and exposes it for the CLI.

```python
from lecturekit.dsl import Lecture

lec = Lecture(id="lec03", title="Processes", subtitle="CSAPP §8")

def intro(p):
    p.title("What is a process?")
    p.slide("An instance of a running program.")
    p.image("proc.svg", width_px=480).footnote("CSAPP 3e §8.2")

lec.page(id="intro", body=intro)
```

## The tree

### `Lecture(*, id, title, subtitle=None, ratio="16:9", lang=None)`

The root. `ratio` sets the deck aspect ratio for every page; allowed values are
`16:9` (default), `4:3`, `16:10`, `3:2`. `lang` names the language the
lecture is *written* in (`"en"`), which only picks lecturekit's own chrome —
the 参考文献 / References page, the outline button, the book's fixed headings;
it defaults to Chinese, and `--lang` (see [i18n.md](i18n.md)) overrides it. `assets=<dir>` names the asset root (the directory containing `assets/`) for
inline notebook rendering; it is inert for the CLI build. See
[notebook.md](notebook.md).

- `lec.section(title, *, id=None, collapsed=False)` — add a section, returns a
  `SectionBuilder`. `id` defaults to a slug of the title. `collapsed` hints the
  outline to start folded. Sections may nest (`section.section(...)`).
- `lec.cover(title, *, id="cover", author=None, time=None, logo=None, tags=())`
  — add an optional top-level cover page. Nothing is inserted unless you call
  this method. `title` is required; `author`, `time`, and `logo` are optional.
  `logo` may be one image path, a `(left, right)` tuple, or a
  `{"left": ..., "right": ...}` mapping for dual logos.

  ```python
  lec.cover(
      "Elastic model serving via efficient autoscaling",
      author="Xingda Wei",
      time="July 2026",
      logo=("assets/ipads.svg", "assets/sjtu.svg"),
  )
  ```

- `lec.page(id, *, body, tags=(), annotation=True, book="page", book_title=None)`
  — add a page. `body` is a
  function `f(p)` that receives a `PageBuilder` and fills it. `tags` is a set of
  free-form labels. `annotation=False` hides all of the page's callout bubbles
  (see [Annotations](#annotations-callout-bubbles)). Returns a renderable `PageHandle`: putting it as a notebook cell's last
  expression displays that one slide inline (see [notebook.md](notebook.md)).
  The return value is safe to ignore in a normal `lecture.py`.

  `book` and `book_title` are **book-side knobs the deck never sees** (see
  [book.md](book.md)): several slides often cover one textbook topic, and a
  slide title is often the wrong heading for a book section.
  `book="merge"` folds the page's book-visible blocks into the previous
  page's section (a section's first page cannot merge); `book="skip"` drops
  the page from the book (e.g. the second page of an annotation-reveal pair,
  which would otherwise duplicate its figures); `book_title="…"` overrides
  the heading the book prints, while the deck and outline keep `p.title(...)`.

  ```python
  s.page(id="duel-chess-ai", body=duel_chess_ai,
         book_title="Case #1：Deep Blue 与国际象棋")
  s.page(id="duel-chess-scale", body=duel_chess_scale, book="merge")
  ```
- `lec.close(id, *, body, tags=(), annotation=True)` — sugar for a closing
  conclusion: a single **top-level page** (not a section), so it lands as a leaf
  in the outline with nothing to expand. Like any page, its title (the outline
  label) comes from `p.title(...)` in `body`. `tags` and `annotation` apply to
  that page. Call it last to land the conclusion at the end of the deck and the
  outline; pass `id` to `--pages` to render just it.

  ```python
  def closing(p):
      p.title("结语：我真正希望大家带走的东西")
      p.slide("进程给隔离，线程给共享。")

  lec.close("conclusion", body=closing)
  ```

- `lec.bridge(text, *, id=None)` — add a **transition page** (衔接页): a line or
  two of plain text, centered on an otherwise empty slide, marking the turn
  between topics. `SectionBuilder` has the same method, so a bridge can also sit
  between two pages inside a section.

  ```python
  s.bridge("排队模型建好了。回到 GPU：一台 instance 的 est() 从哪来？")
  ```

  A bridge is deliberately *not* a page:

  - **Plain text only.** No inline markdown, no `<mark>`, no title, no other
    blocks — and nothing chains off it: `bridge(...)` returns `None`, so there
    is no footnote, annotation, cite, or news. A transition with a number to
    source has outgrown being a bridge and should be a normal page.
  - **At most 3 lines.** Lines are stripped, blank lines dropped; empty text or
    more than 3 lines is a `ValidationError` at the call site.
  - **Not in the outline.** Neither the viewer's tree nor the PDF outline page
    lists it — it is a breath, not a knowledge point.
  - **No slide number.** The page prints no number and the count does not
    advance (Marp's `_paginate: skip`), so the pages around it stay
    consecutively numbered — the same machinery as an animation's held frames.
  - **Deck only.** The book and the transcript sheet never print it
    (`book="skip"` is forced); PPTX draws it as a centered text box.
  - In `--watch` reveal mode it arrives fully lit — one Enter and it is gone.

  `id` defaults to `bridge-1`, `bridge-2`, … (numbered lecture-wide); pass one
  to name the page for `--pages`. A pure-number `--pages` index still counts
  bridge slides, like an animation's frames.

`SectionBuilder` exposes the same `.section(...)`, `.page(...)`, and
`.bridge(...)` methods, and doubles as a context manager
(`with lec.section("…") as s:`). `close(...)` lives on `Lecture` only.

IDs must be unique across the whole lecture; a duplicate raises a
`ValidationError` at build time.

### Review: replaying another lecture's pages

A lecture often opens by looking back at pages taught earlier. `review_section`
builds a section out of pages **borrowed** from other lectures, rather than
copies of them — the source page is loaded at render time, so improving it
improves the review.

```python
# review.py, beside lecture.py
from lecturekit.dsl import review_section

def attach(lecture):
    review_section(lecture, "回顾：一致性协议", {
        "../two-phase-commit-replication": ["2pc-block"],
        "~/lab/other-course/consensus": ["quorum-intro", "commit-logging"],
    })

# lecture.py
import review
review.attach(lecture)          # wherever the review belongs in the deck
```

`review_section(lecture, title, sources, *, id=None, collapsed=False)` takes a
`{source lecture directory: [page ids]}` mapping, so **one review section may
span several lectures**. It returns the `SectionBuilder`, so an author can add
their own transition pages after the borrowed ones.

A source is always spelled out as a directory — there is no implicit sibling
lookup, because a review source need not live next door. A **relative** path
resolves against the file that calls `review_section` (for the usual `review.py`
beside `lecture.py`, that is the lecture directory), never against the working
directory; `~` and absolute paths are taken as given.

A page arrives exactly as it was authored — same title, blocks, figures,
footnotes, callout bubbles, tags, and citations. Nothing marks it as review on
the slide; the section title is what says so. Three things are rewritten, all
keyed on the **source lecture's own `Lecture(id=...)`** rather than the path
typed, so moving a lecture directory changes nothing:

| | |
| --- | --- |
| page id | `<source id>/<page id>`, so it cannot collide and `--pages src/quorum` still names it |
| image `src` | `assets/<source id>/…`; the renderers copy that lecture's `assets/` there, so the bundle stays self-contained |
| book | forced to `book="skip"` — a chapter must not reprint another chapter — and figure `ref`s are dropped with it |

Naming a page that carries [`p.frames(...)`](#frames-an-animation) brings **every
frame**: an animation is one slide, and half of one is a half-drawn figure.

Failures are refusals, not warnings — a missing review page should surface while
preparing, not on the projector. A page id that does not exist (the error lists
the ids that do), a source directory that is missing or fails to load, two
sources sharing one lecture id, or a lecture borrowing from itself each raise a
`ValidationError`.

Under `--watch`, the source lectures are watched alongside this one, so editing a
borrowed page reloads the deck showing it.

### The page body

`body` is called with a `PageBuilder` (`p`). It **must** call `p.title(text)`
exactly once (calling it twice, or never, is a `ValidationError`). Everything
else is a block, appended in call order.

```python
def body(p):
    p.title("Address translation")
    p.slide("The MMU maps virtual to physical addresses.")
    p.code("c", "int *x = malloc(4);")
```

### Page layout

`p.gap(spread=None, *, min_px=None, max_px=None)` opts the page into adaptive
spacing between viewer-visible blocks: the slide's leftover vertical space is
split evenly across the seams between blocks, each seam capped at `spread`
pixels. It is useful for slides with several short blocks that should fill the
vertical canvas instead of clustering near the top.

The cap is the whole knob, so it is what the call takes — a bigger cap absorbs
more of the leftover and pushes the blocks apart, a smaller one leaves the
slack at the bottom and keeps them near the top:

```python
p.gap()               # the default cap
p.gap(24)             # each seam grows to at most 24px
p.gap("fill")         # no cap: the blocks split the whole page
p.gap(24, min_px=12)  # …and the floor a seam keeps on a full page
```

`min_px` is that floor — what a seam is worth when there is nothing left to
distribute. It defaults to 8, except that a cap below 8 pulls the floor down
with it, so `p.gap(4)` means what it says.

The original spelling, `p.gap("auto", min_px=8, max_px=28)`, still works and
still carries its own smaller default cap, so pages written against it render
exactly as before. Giving the cap both positionally and as `max_px` is a
`ValidationError`, as is a `spread` that is neither an integer nor `"fill"`.

The setting is page-level. HTML, PDF, PNG, live preview, notebook inline slides,
and PPTX honor it. The PPTX renderer keeps each block's native geometry, then
distributes the remaining canvas between block groups. Pages that do not call
`p.gap(...)` keep the normal theme rhythm.

`p.gap(...)` is one page-wide *policy* — distribute leftover space evenly — so a
page sets it at most once (a second call is a `ValidationError`). When you want
an **exact** gap at **one** position rather than an even redistribution across
every seam, use a `spacer` block (below); a page may carry several.

### Page news

`p.news(...)` attaches after-class reading material to the page. News is
student-facing companion metadata, not a slide block: it does not appear in
Marp, PPTX, or normal viewer output unless a renderer explicitly asks for it.
The notebook renderer can show it next to the slide with `PageHandle.news()`.

```python
def body(p):
    p.title("The Scaling Law")
    p.slide("Loss scales predictably as model/data/compute grow.")
    p.news(
        "Scaling Laws for Neural Language Models",
        url="https://arxiv.org/abs/2001.08361",
        source="Kaplan et al.",
        date="2020",
        kind="paper",
        why="Original paper behind this slide; read the summary and Figure 1.",
        tags=["scaling-law", "paper"],
    )
```

`kind` is one of `news`, `paper`, `blog`, `video`, or `doc`. Optional fields are
`source`, `date`, `why`, `tags`, `image`, and `archived_url`. Multiple
`p.news(...)` calls keep author order.

### Page citations

`p.cite(...)` attaches a reference to the page. Like news, it is page metadata,
**not a slide block** — it never renders in the slide body. Citations are
collected at the end instead:

- the deck grows a trailing **参考文献** page listing each reference with the
  slide numbers that cited it (`(P3, P7)`), set in small type — it is a record
  to look up later, not something to read off the projector;
- the book collects them into a chapter-end **参考文献** section (no page
  numbers there).

Pass structured fields, or a BibTeX entry string as the first argument to have
its fields parsed; explicit keyword fields always override the parsed ones.

```python
def body(p):
    p.title("Transformer")
    p.slide("Self-attention replaces recurrence.")
    # structured
    p.cite(title="Attention Is All You Need", author="Vaswani et al.",
           year="2017", venue="NeurIPS", url="https://arxiv.org/abs/1706.03762")
    # or a BibTeX entry — parsed into the same fields
    p.cite("@inproceedings{vaswani2017, title={Attention Is All You Need}, "
           "author={Vaswani, Ashish and others}, year={2017}, booktitle={NeurIPS}}")
    # BibTeX underneath, a hand-written field on top
    p.cite("@article{...}", author="Vaswani et al.")
```

Fields: `title` (required — given directly or parsed from BibTeX; otherwise a
`ValidationError`), and optional `author`, `year`, `venue`, `url`, `key`. The
BibTeX reader is deliberately small: it lifts those fields, shortens a multi
author list to `Last et al.`, and handles a common subset of LaTeX escapes;
anything it cannot parse is left for the explicit fields to supply.

References are **deduplicated**: two `p.cite(...)` calls with the same `key`
(or, absent a key, the same title+year) collapse into one entry, and the deck
page lists every slide that cited it. `__references__` is the reserved id of the
auto-generated deck page — a hand-authored page may not use it.

## Blocks

A page is a flat list of blocks. Every block kind takes the optional
`only=`/`except_=` visibility filters described under
[Block visibility](#block-visibility), and the optional `key=` described under
[Translation keys](#translation-keys). Methods that can be annotated return a
handle for chaining (see [Footnotes](#footnotes) and
[Annotations](#annotations-callout-bubbles)).

| Method | Block kind | Purpose |
| --- | --- | --- |
| `p.cover(...)` | `cover` | Cover-page metadata, usually created through `lec.cover(...)`. |
| `p.slide(content)` | `slide` | A markdown text block — the main body of a page. |
| `p.code(language, content, …)` | `code` | A fenced code block in the given language (see [`pseudo`](#the-pseudo-language) and [marking lines](#marking-a-line)). |
| `p.link(label, url)` | `link` | A labelled hyperlink. |
| `p.image(src, …)` | `image` | An inline image (see [Images](#images)). |
| `p.frames(src, src, …)` | `image` ×N | An animation: one page, one figure per frame (see [Frames](#frames-an-animation)). |
| `p.side_image(src, …)` | `side_image` | An image in a side column; text reflows beside it. |
| `p.sidenote(title, text, …)` | `sidenote` | A boxed callout pointing at external material. |
| `p.highlight(text, …)` | `highlight` | A centered emphasis chip (see [Highlight](#highlight)). |
| `p.table(rows, *, headers, …)` | `table` | A GFM table (see [Tables](#tables)). |
| `p.architecture(…)` | `architecture` | A layered + modular system diagram (see [Architecture diagrams](#architecture-diagrams)). |
| `p.aside(content)` | `aside` | A secondary remark. |
| `p.spacer(px)` | `spacer` | A fixed vertical gap between blocks (see [Spacer](#spacer)). |
| `p.notes(content)` | `notes` | Speaker notes — emitted as a Marp presenter note (speaker view only), never shown on the slide. |
| `p.prose(content)` | `prose` | Textbook prose — rendered by the book, never by the deck (see [book.md](book.md)). |
| `p.demo(name, command, output=…, timeout=…)` | `demo` | A command, shown as a transcript; the deck can [run it and stream the output](usage.md#running-a-demo-from-the-deck). |

`slide`, `aside`, and the inline `**bold**`/`*italic*`/`` `code` ``/`[label](url)`
markdown inside cells and callouts pass straight through to the renderer.

### The `pseudo` language

`p.code(...)` normally hands its `language` to the renderer's own highlighter.
`pseudo` is the exception — a language lecturekit lexes itself, because lecture
pseudocode belongs to no real language and the three categories a highlighter
knows (keyword / string / comment) are the wrong three for it: there are no
strings, few comments, and maybe two control-flow words on a page.

```python
p.code("pseudo", """Acceptor receives <proposal, N>
    if  N < Nh
        reply <promise-reject>
    else
        Nh = N
        reply <promise-ok, Na, Va>""")
```

Four token classes get color:

| class | what it matches |
| --- | --- |
| keyword | a fixed, case-insensitive word list — `if` `else` `then` `while` `for` `foreach` `repeat` `until` `return` `break` `continue` `upon` `receive(s)` `send` `reply` `broadcast` `wait` `null` `nil` `true` `false` |
| message | anything in angle brackets: `<promise-ok, Na, Va>` |
| state | any name the block **assigns to** — `Nh = N` colors *every* `Nh` in the block, and `read-set[d] = …` names `read-set` |
| comment | `#` or `//` to end of line |

The `state` rule takes no author input: the state of a protocol is by
definition what its steps write to, so the assignment targets are exactly the
names a lecture wants to trace. An assignment is a name, spaces, then `=` —
anywhere on the line, so `Na = N; Va = V` names both and `Leader chooses Mn =
…` names `Mn`. A comparison never counts, because `V != null` and `N <= Nh`
put an operator where the `=` would have to be. A name that is only ever read
(a message field, a loop bound) stays plain — to color one, assign it.

Two spellings of a name count as one name, because pseudocode writes them
constantly. A **hyphenated** one is a name (`read-set`, `write-set`) — a
lecture names its state the way the slide's prose does, and the prose says
"read set"; the hyphen stays an operator everywhere else (`P-1` is still
arithmetic), since only names the block assigns to are read this way. And a
**subscripted or dotted** target names what is in front of the bracket:
`read-set[d] = …` and `bank[a] = …` write one entry of a table, which is still
writing the table.

Deliberate limits, so the lexer stays a lexer: a lone `>` with a `<` earlier on
the **same line** reads as a message (`a < b and c > d` is one), and the
keyword list is short on purpose — `and`, `or`, `do`, `on`, `end` are left out
because they turn up as ordinary prose inside a step.

Message and state take the figure palette's colors, so a `<accept, N, V>` in
the code block and the same message drawn in the diagram beside it read as one
thing. A keyword takes the palette's green rather than the blue every other
language's keywords get — blue is already the message's, and a keyword drawn
in the body's own navy is only bold, which at code size is no color at all. The
viewer and the book both render them; PPTX falls back to plain text, as it does
for the other geometry-aware blocks.

### Marking a line

`p.code("pseudo", src, mark=[2, 7])` washes those lines in the tone — the
marker pen of [`<mark>`](#inline-highlight-mark) at line scale, for the page
that says "these two lines are the new ones" and otherwise leaves the reader to
find them.

```python
SHADOW = """transfer(bank, a, b, amt):
    fcopy(bank, bank_temp)
    records = mmap(bank_temp, ...)
    records[a] = records[a] - amt
    records[b] = records[b] + amt
    fsync(bank_temp, ...)
    rename(bank_temp, bank)"""

p.code("pseudo", SHADOW, mark=[2, 7])                   # the two new lines
p.code("pseudo", SHADOW, mark=[6], tone="orange")       # where the crash lands
```

`mark` is a list of **1-based line numbers**, counting the block as the slide
shows it — the leading and trailing blank lines a triple-quoted literal
collects are stripped first, so line 1 is the first line of code. The numbers
live on the `p.code(...)` call rather than in the listing, which is what lets
one shared constant be marked differently on each page that shows it — the
usual shape for a listing that a lecture walks down step by step.

`tone` takes the same four as a [chip](#highlight) and a `<mark>` — `yellow`
(default), `orange`, `green`, `blue`. The wash runs the **height of the line**
rather than through its lower half: the unit being marked is a whole line, and
a half-height stroke under one reads as an underline. It stops where the code
does, so the block does not grow a band across it.

Every target draws it: the viewer (and so PDF/PNG), the book, the transcript
sheet, and PowerPoint — where it becomes PowerPoint's own text highlight, so
the line stays editable rather than arriving as a picture.

**`pseudo` only.** Another language is coloured by the renderer's own
highlighter, which leaves nowhere to put the wash, so `mark` on one is a
`ValidationError`. So is a line number past the end of the block, or one
landing on a blank line — refused rather than ignored, because an author who
renumbers a listing and forgets the mark would otherwise find out on the
projector, from a wash that quietly went missing.

### Auto-bold on `slide`

A slide is usually a headline claim with supporting bullets under it, so
`p.slide(...)` **bolds every flush-left prose line for you**. You write the
headline plainly; it renders bold.

```python
p.slide("""
数字化系统 scale 本质就是要做好两件事情：
- 可靠：数据对不对，东西会不会丢
- 性能：快不快，能否更好地 scale
""")
```

The first line renders bold; the bullets render normally.

A flush-left line is left alone when it is a list item (`-` `*` `+` `1.` `1)`),
a heading (`#`), a blockquote (`>`), a table row (`|`), a thematic break
(`---`), an image or raw HTML (`!` `<`), a line inside a ` ``` ` / `~~~` fence,
or a line that **already** contains `**` or `__` — there the author is
controlling emphasis by hand, and nesting would corrupt it.

`<mark>` is the one exception to the raw-HTML rule: a line starting with it is
DSL markup, not the author taking the line over, so it still bolds (see
[Inline highlight](#inline-highlight-mark)).

**Indentation is the escape hatch.** To keep a flush-left line unbolded, indent
it by one space:

```python
p.slide("""
这行会加粗
 这行不会
""")
```

**`autobold=False` is the same escape, block-wide.** A slide that is a few
plain lines rather than a headline over bullets would otherwise carry a leading
space on every one of them — an indent that reads as a typo and that a later
edit silently drops:

```python
p.slide("""
这几行都不加粗
每一行都是平的
""", autobold=False)
```

Only the bolding is off. `==keyword==` still expands, an explicit `**...**`
still means what it says, and the one-space indent still works for the odd line
inside a block that is otherwise autobolded. The choice rides the block, so a
[translation overlay](i18n.md) puts a replacement through the same rule rather
than re-bolding what the author kept flat.

This applies to `slide` only; `notes`, `prose`, and `aside` pass through
untouched (a textbook paragraph has no headline).

### Revealing a slide's bullets one at a time

The [reveal-on-Enter](usage.md#reveal-on-enter) unit is the block, so a `slide`
normally lights whole: headline and every bullet on one Enter. That is the right
unit for a slide whose bullets are one claim seen from several sides, and the
wrong one for the summary slide whose points are meant to land one by one.
`reveal="items"` splits that block into its own list items:

```python
p.slide("""
今天讲了这些
- 为什么要并发：一个线程扛不住这个 workload
- 并发的代价：race condition
- 并发下想要的性质：before-or-after atomicity
""", reveal="items")
```

One Enter lights the headline, then one bullet apiece. Whatever else the block
holds — a `image_right` figure, a second paragraph — is a step of its own, in
the order it is written; a nested list rides its parent item rather than
stepping separately, and the block's callout bubbles arrive with its last item.
The list itself is never dimmed, so the bullets hold their positions and each
one arrives in place instead of pushing the others down.

It is a `slide` keyword and needs something to step through: `reveal="items"` on
a block with no list item is a `ValidationError` at build time, rather than a
step that quietly does nothing.

**Live preview only.** The split exists in `--watch` reveal mode; `render`, a
non-watch `view`, and every export render the block whole, exactly as they do
without it.

### Inline highlight (`<mark>`)

A page of uniformly bold lines has no focus. Auto-bold has already spent
`**bold**` on the headline, so bold cannot also mark the one *word* the line
turns on — and the CJK face has no heavier cut to escalate to. `<mark>` is that
second, orthogonal gesture: a marker-pen stroke laid over the body of a word —
its top on the lowercase, ascenders and cap tops standing clear of it.

```python
p.slide("""
==Multi-Paxos==: replicate a ==sequence== of values
- the leader is ==orange:not== the primary
""")
```

`==keyword==` is the shorthand for `<mark>keyword</mark>`, and `==tone:keyword==`
for `<mark class="tone">keyword</mark>` — marking a word is the commonest edit a
slide gets, and thirteen characters of tag around a four-character word is
enough friction to skip it. The tag itself stays legal and means exactly the
same thing; the shorthand is rewritten into it before anything else looks at the
text, so the two never diverge:

```python
p.slide("""
<mark>Multi-Paxos</mark>: replicate a <mark>sequence</mark> of values
- the leader is <mark class="orange">not</mark> the primary
""")
```

It takes the same four tones as the [highlight chip](#highlight) — `yellow`
(the default, written as a bare `==…==` / `<mark>`), `orange`, `green`, `blue` —
because a marked keyword and a chip are one gesture at two scales. Markup
composes inside it (`==a *b*==`) and around it, and a line whose **first** word
is marked still auto-bolds.

The shorthand's body may not span a line, may not contain `==`, and must begin
and end with a non-space — which is what keeps an arithmetic `a == b == c` from
reading as a mark, since a comparison always pads its operator. A `` `code
span` `` or fence is left verbatim, there as for the tag. Nothing checks for a
half-written `==`: an unpaired one is ordinary text everywhere else in markdown,
so it stays ordinary text here. Where you want the mistake *refused* — a tone
typo, an unclosed span — write the tag, which is validated (below).

**It is legal in `p.slide(...)` text and nowhere else.** Not in a title (already
the loudest thing on the page — marking a word there says this matters more than
the loudest thing, which is a sign the title is too long), not in a caption,
sidenote, footnote, callout bubble, table cell, chip, or `prose`. A `code` block
is exempt from the check entirely, since a sample may legitimately *show*
`<mark>` as the HTML it is, as may an inline `` `code span` ``. Only slide text
is expanded, so a `==keyword==` typed into a caption or `prose` reaches the page
as those literal characters rather than as an error — visibly wrong on the
slide, which is the point.

Only two tag spellings are accepted — `<mark>` and `<mark class="<tone>">` with
double quotes and no other attributes. Everything else is a `ValidationError` at
build time: a misspelled tone, single quotes, an unclosed or nested tag, an
empty `<mark></mark>`, or the tag anywhere but slide text. Refused rather than
quietly dropped, because a highlight that silently fails to render is
indistinguishable from a word you never marked — you would find out on the
projector.

All three targets draw it: the viewer (and so PDF/PNG), PowerPoint (via its own
text highlight, so it stays editable), and the book. The book only ever sees one
through `p.slide(..., only=["latex"])` — a slide never reaches it otherwise.

### Math in slides

The viewer enables **MathJax** for every Marp deck. In `p.slide(...)`, use the
usual inline `$...$` or display `$$...$$` math syntax. Make the Python string a
raw string when it contains LaTeX backslashes, so Python does not treat commands
such as `\sum` or `\theta` as escapes:

```python
p.slide(r"""
 $$
 C_{naive} = B \sum_{t=0}^{N-1} C(L+t)
 $$
""")
```

**Indent a `$$` fence by one space**, as above. A flush-left `$$` is an ordinary
prose line to the [auto-bold](#auto-bold-on-slide) rule, which turns it into
`**$$**` — no longer a fence. The one-space indent is auto-bold's escape hatch.

A [`p.highlight(...)`](#highlight) chip takes the same `$…$` / `$$…$$`, and
needs no indent — see there for how a `$$` fence lands on one row of the chip.

This applies to the viewer, watch mode, PDF, and PNG exports because they share
the Marp deck. The PPTX export translates the same `$…$` / `$$…$$` into
**OMML**, PowerPoint's own equation markup, so a formula is typeset there too —
and stays editable in PowerPoint's equation editor rather than arriving as a
picture. See [usage.md](usage.md#math) for the vocabulary it covers.

### `slide` vs `prose`

`p.slide(...)` is the deck's body; `p.prose(...)` is the book's. A page carries
both, and each renderer sees only its own — so the same figure, table, or
sidenote is authored once and reused, while the running text is written for its
medium. See [book.md](book.md).

### Block visibility

Two cross-cutting controls decide whether a block is rendered:

1. **Renderer block tables.** Each renderer renders only the kinds it knows. The
   viewer renders `slide`, `code`, `link`, `image`, `side_image`, `sidenote`,
   `aside`, `demo`, and `table`. `notes` is emitted into the Marp deck as a
   **presenter note** — an HTML comment the speaker view shows (press `p`) but
   the audience slide never renders. The `latex` (book) target renders `prose`,
   `demo`, and the shared blocks, but never `slide`.
2. **Author overrides.** Every block accepts `only=[…]` / `except_=[…]`, a set of
   renderer names that force the block in or out for specific targets, **on top
   of** the renderer's own table. `only=["latex"]` therefore pulls a block into
   the book even though the book would not normally render its kind — which is
   how one `slide` can double as a page's prose.
3. **`.disable()`.** The chained handle method `.disable()` turns a block off in
   **every** target at once, ahead of both controls above — a disabled block
   renders nowhere and no `only=` can force it back. The block stays in the
   source and the serialized AST, so it holds finished text back rather than
   deleting it:

   ```python
   p.prose("这段书稿还没定稿，先不进书").disable()
   ```

   On a `prose` block (book-only) this means "written, but kept out of the book".
   A page whose only prose is disabled reverts to showing just its figures — with
   no `TODO` box and no dent in `--stats` (see [book.md](book.md)); a page that
   never had prose still shows the `TODO`.

### Translation keys

Every block-producing method takes `key="…"`, which pins that block's name in a
translation overlay (`i18n/<lang>.toml`) instead of letting it be numbered by
position:

```python
p.slide("这一页的核心问题", key="crux")     # -> intro.crux, not intro.slide.2
```

It is inert unless the lecture is rendered with `--lang`. A key is local to its
page, must not contain `.`, and must be unique within the page; a pinned block
does not consume a number in its kind's sequence, so pinning one never renumbers
its neighbours. See [i18n.md](i18n.md).

### Images

`p.image(src, *, alt="", caption=None, width_px=None, width_pct=None,
height_px=None, height_pct=None, framed=False, caption_align="center")`

An image renders as a `<figure>`. The caption sits below it and is **centered by
default**; pass `caption_align="left"` or `"right"` to re-align it (any other
value raises a `ValidationError`).

Size is **unit-explicit** — a bare number is never ambiguous. Pass `width_px` /
`height_px` for pixels or `width_pct` / `height_pct` for a percentage of the
slide. Passing both units for one dimension raises a `ValidationError`.

Setting one dimension leaves the other `auto`, so the **aspect ratio is always
preserved** — changing the width changes the rendered height to match (and vice
versa). A sized image is never squished to fit; if you size it larger than the
slide it simply overflows, so pick a width that fits. (`width_px`/`width_pct`
beyond the slide width are still capped to the slide.) An **unsized** image, by
contrast, is scaled down to fit the slide's leftover space.

`framed=True` draws a bordered, shadowed card around the image. It is off by
default, since transparent line diagrams (e.g. architecture SVGs) read better
unframed.

```python
p.image("arch.svg", width_px=480)                 # 480px wide
p.image("arch.svg", width_pct=60)                 # 60% of the slide width
p.image("photo.png", framed=True)                 # bordered + shadowed card
p.image("arch.svg", caption="图 1", caption_align="left")
```

`image` returns an `ImageHandle` whose setters mirror those keywords, so options
can be chained alongside a footnote or annotation. The last call for an option
wins:

```python
p.image("arch.svg").framed().width_px(480).footnote("来源：CSAPP 3e")
p.image("arch.svg", caption="图 1").caption_align("right")
```

`p.side_image(src, *, alt="", width=None, side="right")` places the image in a
side column and reflows the slide text into the other column. `side` is `"right"`
(default) or `"left"`; `width` (e.g. `"38%"`) sets the column width, defaulting
to a half-split.

`p.slide(...).image_right(src, *, alt="", width_px=None, width_pct=None,
height_px=None, height_pct=None)` floats a **small** image on the right of that
slide block's text, sharing the same horizontal band; the text wraps to its left
and flows under it once cleared. It chains like a footnote
(`p.slide("…").image_right("weekly.png", width_px=160)`) and can be combined with
`.footnote(...)` / `.annotate(...)`. Size is unit-explicit, exactly as for
`p.image`. It is valid **only on a `slide` block**; calling it elsewhere is a
`ValidationError`.

This is distinct from `side_image`: `side_image` splits the *whole slide* into
two full-height columns (a Marp split background), whereas `image_right` is a
single small figure beside one block of text that does not claim the full slide
height.

`p.row(*, caption=None)` lays out several images **side by side** in one band.
It returns a `RowHandle`; add images with `row.image(src, ...)`, each taking the
same options as `p.image` (`alt`, `caption`, `width_px`/`width_pct`,
`height_px`/`height_pct`, `framed`, `caption_align`). Images chain, and
`.footnote(...)` / `.annotate(...)` attach to the row as a whole.

```python
p.row(caption="进程 vs 线程")
 .image("proc.svg", width_px=300, caption="图1")
 .image("thread.svg", caption="图2", framed=True)
 .footnote("来源：CSAPP 3e")
```

With no per-image width, the images share the row width evenly; a per-image
`width_px`/`width_pct` pins that image's width and the rest split what's left.
Items are top-aligned, so each image's own caption sits below it at its natural
height; the optional `caption` renders below the whole row. An empty row (no
`.image(...)`) raises a `ValidationError` at build time.

This is distinct from the other image placements: `side_image` splits the whole
slide into two full-height columns, `image_right` floats one small image beside
a single text block, and `row` is a self-contained band of N images with no
slide text flowing around them.

### Frames (an animation)

`p.frames(*srcs, alt="", caption=None, width_px=None, width_pct=None,
height_px=None, height_pct=None, framed=False, caption_align="center",
ref=None)`

A build-up figure — the same slide, redrawn a step further each time. Write the
page **once** and list the frames; the page is expanded at build time into one
slide per source:

```python
def commit_logging(p):
    p.title("Commit logging 先写完整新状态")
    p.slide("log 先落盘，再改 bank file")
    p.frames(
        "assets/log-1.svg",
        "assets/log-2.svg",
        "assets/log-3.svg",
        width_px=900,
        caption="先写 log，再改数据",
    ).footnote("Saltzer & Kaashoek §9.3")

s.page(id="commit-logging", body=commit_logging)   # -> 3 slides
```

Only the image **source** varies within the animation. Size, caption, frame,
`ref`, footnotes, and callout bubbles are written once on the block and shared
by every frame — which is what makes "same figure, drawn a step further"
structural instead of a convention you have to keep. The options mirror
[`p.image`](#images), and the figure lands exactly where `p.frames(...)` was
called.

**Where a block sits relative to `p.frames(...)` decides when it shows.**
Blocks written *before* the call ride every frame — they are the setup the
animation plays under. Blocks written *after* it belong to the **finished
picture** and show only on the last frame, so a verdict chip or a concluding
line lands when the animation has played, not on frame 1:

```python
def lost_commit(p):
    p.title("Case：COMMIT 消息丢了")
    p.slide("coordinator 已决定 commit，消息没到 worker")   # every frame
    p.frames("lost-1.svg", "lost-2.svg", "lost-3.svg")
    p.highlight("worker 重问 coordinator 即可恢复")          # last frame only
```

The earlier frames keep the held blocks' **space** blank (the deck renders them
invisible rather than dropping them), so the figure and the text above it sit
at identical positions on every frame — the punchline appears in place instead
of pushing the layout around. In PPTX, where slides share no geometry, held
blocks are simply omitted from the earlier frames' slides. A `side_image` is
the one exception: it is a Marp split background claiming the whole slide, so
it stays visible on every frame wherever it is written.

A page carries **at most one** `frames` block (one page, one animation); a
second is a `ValidationError`, as is an empty list or a blank source. Frame
paths are listed one by one — no glob, no `{}` template — so the number and
order of frames are fixed by the source, not by what happens to be on disk.

**The frames are ordinary pages** with ids `<page id>-1` … `<page id>-N`, and
everything downstream treats them as such. What differs:

| | |
| --- | --- |
| outline | one row for the animation, linking to frame 1; `inspect` likewise lists it once |
| slide number | one number for the whole animation — every frame prints it, and the count advances once (see [usage.md](usage.md#two-numberings)) |
| `--pages` | the authored id selects the whole animation (`--pages commit-logging`), `commit-logging-2` one frame, a number is still a deck index |
| book | one section printing the **last** frame — the finished picture, post-animation blocks included — so a `ref` names that figure; the earlier frames drop out like `book="skip"` (see [book.md](book.md)) |
| `--watch` reveal | frame 1 steps through the blocks before the animation; the last frame steps through the blocks after it; the frames in between arrive fully lit, one Enter apiece (see [usage.md](usage.md#reveal-on-enter)) |
| `p.notes(...)` | rides every frame, so the speaker view has the script whichever frame is up |
| `p.cite(...)` | the animation is one slide to a reference: the backref reads `(P2)`, not `(P2, P3, P4)` |

This is the figure counterpart of the [annotation reveal
pair](#reveal-a-bubble-as-a-build-step): that one reveals a *bubble* over an
unchanged page, this one swaps the *figure* under unchanged text.

### Pages titled the same

`p.frames(...)` is the tidy way to tell one idea over several slides, but not the
only one: a page and the animation that redraws it, an [annotation reveal
pair](#reveal-a-bubble-as-a-build-step), a build-up carried by seven hand-written
pages — all of them repeat a title, because they are one knowledge point.

So a **run of consecutive sibling pages carrying the same title is one row and
one number**, exactly as an animation is:

```python
with lec.section("MapReduce") as s:
    s.page(id="complete-picture", body=overview)   # ┐ one outline row,
    s.page(id="step1-split",      body=step1)      # │ one number on the
    s.page(id="step2-fork",       body=step2)      # ┘ projector
```

The row links to the run's first slide and spans the rest; the deck prints the
run's number on every page of it (Marp's `_paginate: hold`, the same machinery
an animation's frames use), and a `p.cite(...)` on any of them backrefs that one
number. An animation inside a run counts as one unit — its frames already share
a title, so the group as a whole folds.

Nothing else changes: every page keeps its own id and is its own slide, so
`--pages step2-fork`, `slides.00N.png`, and navigation are untouched. `inspect`
keeps one line per page and marks the folded ones `[folded]`, since that is the
structural view.

Two limits, both about not silently swallowing a page:

- **Siblings only.** A run stops at a section boundary — folding across one
  would hand the next section's first page to the previous section's last row,
  leaving that section opening with no row of its own.
- **A [bridge](#the-tree) breaks it.** A breath between two pages says they are
  not one idea.

To get two rows, give the two pages two titles — a shared title is the whole
claim being read here. (The book is a separate question: it maps a page to a
heading of its own unless told otherwise, and `book="merge"` is how a run
becomes one book section. See [book.md](book.md).)

### Tables

`p.table(rows, *, headers, align=None)` renders a native GFM table. `headers` is
required (a GFM table must have a header row); `rows` is a list of rows, each a
list of cells. Cells carry inline markdown, passed straight through.

```python
p.table(
    headers=["机制", "开销", "隔离"],
    rows=[
        ["进程", "高", "强"],
        ["线程", "低", "弱"],
    ],
    align=["left", "right", "center"],
)
```

Rows must be rectangular — every row's width must equal the number of headers.
`align`, if given, is one of `left` / `center` / `right` per column and must have
one entry per column; omit it for all-left. A literal `|` in a cell is escaped
and newlines are collapsed so a row never breaks.

### Architecture diagrams

`p.architecture(*, caption=None, flow=None)` draws a **layered + modular** system
diagram: vertical layers stacked top-to-bottom, where each layer is a single box
holding a row of nested module boxes. It returns an `ArchHandle`; add layers with
`arch.layer(title, modules)`, read top-to-bottom exactly as the diagram renders.

```python
arch = p.architecture(caption="进程看到的是层层抽象", flow="down")
arch.layer("Application", ["Shell", "Editor", "Compiler"])
arch.layer("Kernel",      ["Scheduler", "Virtual Memory", "File System", ...])
arch.layer("Hardware",    ["CPU", "RAM", "Disk"])
```

- `arch.layer(title, modules)` appends one layer box; `title` is its bold black
  label, sitting on top of the nested module boxes (pass `None` or `""` for an
  unlabeled box). `modules` is a non-empty list of box labels. It returns `self`,
  so layers chain: `p.architecture().layer("App", […]).layer("Kernel", […])`.
- A module entry of `...` (the Python ellipsis literal, or the string `"..."` /
  `"…"`) renders as a faded, dashed placeholder box meaning **"more modules here,
  not drawn"** — e.g. `["Scheduler", "Virtual Memory", ...]`.
- `flow` ∈ `{None, "down", "up", "both"}` draws a chevron between **adjacent**
  layers indicating the depends-on / calls direction (`None` = plain stacked
  bands). It is between-layer flow only — there are no arbitrary module-to-module
  arrows.
- `caption` renders below the diagram, centered, like an image caption.

The whole diagram is pure HTML laid out by the theme's CSS (no measured geometry),
so it stays consistent with the deck and survives PDF export. Layout is implied by
structure: layers stack, modules nest side by side and share the layer width
evenly. Module and label text is plain (escaped) text, not markdown. An empty
diagram (no layers), a layer with no modules, a blank module, or an unknown `flow`
each raise a `ValidationError` at build time. Like any block, it chains
`.footnote(...)` and `.annotate(...)`.

### Sidenotes

`p.sidenote(title, text, *, link=None, logo=None)` is a boxed callout pointing at
external material: a logo (default 📖, or a custom emoji/glyph or image path), a
bold title (linked when `link` is given), and body text. The body supports a
small slice of inline formatting — `**bold**`, `*italic*`, `` `code` ``,
`[label](url)` links, blank lines (paragraph breaks), and the whitelisted inline
tags `<u> <b> <i> <em> <strong> <code> <sup> <sub> <br>`. Anything else is
escaped — except [`<mark>`](#inline-highlight-mark), which is rejected outright
rather than escaped, here as everywhere outside slide text. The logo floats so
the body text wraps around it.

The same inline-formatting rule applies to footnote and annotation bodies.

### Highlight

`p.highlight(text, *, tone="yellow")` is the punchline gesture: one short
phrase, set large and bold, centered, coloured in the tone and framed by a
hairline rule in it, with a soft shadow lifting the card off the page. Use it
for the sentence a slide exists to land — a term being introduced, a verdict,
the punchline. The frame is what makes it read as one object; nothing is painted
inside it, because a fill under a sentence that wraps stops hugging its text and
becomes a band across the slide.

```python
p.highlight("Underloaded")
p.highlight("Overloaded", tone="orange").footnote("λ > μ 时")
```

It is an ordinary block, not a page mode: a page can carry a title, bullets, a
highlight, and a figure. Only the **horizontal** centering is its business — it
never claims the page's vertical space.

`tone` is one of `yellow` (default), `orange`, `green`, `blue` — a closed set, so
a deck stays consistent; an unknown tone is a `ValidationError`. An empty `text`
is likewise rejected.

**Newlines are kept**, and grow the same chip taller rather than starting a
second one:

```python
p.highlight("""Underloaded
一半的队列是空的""")
```

Each line is stripped, so a triple-quoted literal indents naturally. A blank
line is not a paragraph break — a highlight is one utterance; write two
`highlight` blocks for two.

Each line takes the same inline formatting as a sidenote body — `**bold**`,
`*italic*`, `` `code` ``, `[label](url)`, and the whitelisted inline tags. It is
**not** autobolded: the block is already bold, and the auto-bold rule applies to
`slide` alone. `<mark>` is likewise rejected here — the chip is already one
unbroken emphasis, so marking a word inside one is emphasis on emphasis.

**Math too.** A punchline is often the formula the page spent ten slides
earning, so a chip takes `$…$` and `$$…$$` exactly as slide text does:

```python
p.highlight(r"每步代价 $T \approx 2PN/W$")
p.highlight(r"""
$$
C_{naive} = B \sum_{t=0}^{N-1} C(L+t)
$$
""")
```

A `$$` fence is **one row of the chip**, however many lines it was typed across
— a chip's rows are its lines, and a formula is one utterance. It needs no
leading space (auto-bold, the reason a `$$` fence in a `slide` wants one, never
applies here). Display math is set in display style *on that row* rather than in
a block of its own: the chip is already a centered line of the slide, so there
is nowhere else for it to go. It renders in all four targets — the deck (and so
PDF/PNG), the book, the transcript sheet, and PowerPoint, where it stays an
editable equation.

Because the chip is already at 700 — and the CJK face has no heavier cut to fall
back on — `**bold**` inside one cannot go bolder. The deck steps those words
back to the body's navy instead, a second voice against the tone. The book and
PPTX have no equivalent, so there `**bold**` inside a highlight renders as plain
chip text.

Like any block it chains `.footnote(...)` and `.annotate(...)`, and takes
`only=` / `except_=`. The footnote marker rides the chip's own line rather than
claiming one of its own.

### Demos

`p.demo(name, command, output=None, description=None, timeout=None)` is a
command the lecture runs on stage.

```python
p.demo("看汇编", "gcc -O2 -S demo.c -o - | grep -A3 return_1",
       description="当场编译,当场看输出")
```

`command` may be several lines; they run in order, as one shell would read them.
A line that ends in a backslash, or that leaves a quote open, continues the one
above it and gets no prompt of its own on the slide:

```python
p.demo("分四步走一遍", """cd examples
gcc -E mini_ollama.c -o mini_ollama.i     # see what the macros expanded to
gcc -S mini_ollama.i -o mini_ollama.s     # see what the assembly looks like""")
```

`output=` is what the command printed when **you** ran it. Every target shows it,
so the page carries its point whether or not anyone presses the button, and the
PDF and the PPTX carry it too. Pressing the button does not touch it: the live
output goes to a drawer and the slide holds still.

```python
p.demo("列出相关进程", "ps -eo pid,comm,args | grep '[o]llama'",
       output="""1832  ollama         /usr/local/bin/ollama serve
1904  ollama-runner  ... --model ~/.ollama/models/blobs/sha256-...""")
```

`timeout=` is in seconds. `timeout=0` says the command has no natural end
(`ollama serve`) and is stopped by hand — or by turning the page, which stops
every run the slide started; omitted, the session's `--demo-timeout` applies.

The **deck** draws a one-line chip for a bare one-liner, and a transcript box —
`pre`'s own box, so a page that used to hold a `p.code(...)` of the same thing
keeps its proportions — as soon as the command runs to several lines or carries
an `output=`. The ▶ button is inert in a rendered bundle and live under
`view --watch --demo`; see
[Running a demo from the deck](usage.md#running-a-demo-from-the-deck) for what
pressing it does and what it can reach. The **book** prints a 动手试试 box and
the **PPTX** the same still transcript the deck shows at rest — neither can run
anything, so `output=` is the whole of what their readers get. The transcript
sheet skips the block.

Commands run in the **lecture directory**, so a demo refers to its files the way
the lecture does (`demo.c`, not a path). Like any block it takes `only=` /
`except_=`, and `.disable()` takes it out of every target — including out of the
set of commands the deck can run at all.

`name` and `description` are translated; `command` and `output` are not, for the
same reason code is not — a translated command is a different command.

### Spacer

`p.spacer(px)` inserts a fixed vertical gap `px` pixels tall between the blocks
around it. `px` must be a positive integer. Unlike `p.gap(...)` — a page-wide
policy that distributes leftover space *evenly* across every seam — a spacer is a
manual, exact gap at one position, so a page may carry as many as it needs:

```python
p.slide("第一步：先建立问题")
p.spacer(32)                       # 空一档，把下一块推开
p.slide("第二步：再谈解决方案")
```

Like any block it accepts `only=` / `except_=`. It is meant for normal pages;
combining it with `p.gap(...)` on the same page mixes two spacing policies and
is not recommended. The viewer/Marp deck (HTML, PDF, PNG, live preview) honors
it; PPTX ignores it, like the other geometry-only blocks. A spacer is pure
whitespace, so in `--watch` reveal mode it is always visible and never claims a
reveal step of its own.

### Figure refs (book only)

A figure block (`image`, `row`, `architecture`) can carry a **ref** — an
author-chosen anchor that the book's prose cites with `[@name]`, rendering as a
numbered reference ("图 2.3") in LaTeX:

```python
p.image("assets/chess-eval.png", caption="评估函数给局面打分", ref="chess-eval")
p.prose("如 [@chess-eval] 所示，评估函数不必搜到终局。")
```

`ref` is a keyword on `image` / `row` / `architecture` and a chainable
`.ref(name)` on their handles. Names are letters/digits/dashes/underscores,
unique within a lecture, and **require a caption** — the figure number the
reference resolves to rides the caption, so an uncaptioned ref is a
`ValidationError`. `[@lec03:name]` references a figure in another chapter.

The token only means something to the book: prose, sidenote bodies, captions,
and footnotes resolve it; the deck never renders prose, and a `[@name]` typed
into deck-visible text stays literal. A `[@name]` matching no ref in the
lecture logs a warning at book render time (LaTeX prints `??`).

## Footnotes

A footnote is not its own block kind — it is **chained onto** the block it
annotates, so the source of a figure or a caveat on a claim stays next to what it
refers to. Any block handle exposes `.footnote(text)`, and a block may carry
several:

```python
p.image("arch.svg", caption="地址转换").footnote("来源：[CSAPP](https://csapp.cs.cmu.edu) 3e")
p.slide("每个进程看到独立地址空间").footnote("故障不会跨进程传播").footnote("See **CSAPP** §9")
```

The annotated block gets a superscript number; the footnotes collect as small,
grey text pinned to the slide's bottom-left, numbered to match (sequentially
across the whole slide). Footnote bodies use the same inline formatting as a
sidenote. **The marker never claims a line of its own.** On a figure (`image`,
`row`, `architecture`) it rides the figure: on the figure's caption when it has
one, otherwise as a small superscript pinned to the top-right corner (the
image's own corner for a single image, the figure box's for a row/diagram). A
`code` block or `table` ends in a syntactic line (a closing fence, a `| … |`
row) that the marker cannot be appended to, so it is parked on the block's
bottom-right corner instead. On a text block it rides the end of the last line
at zero width, so a line that already fills the column keeps its own wrapping
and the number hangs into the slide's right margin rather than dropping onto a
line by itself.

## Annotations (callout bubbles)

Where a footnote is quiet marginal text, an `annotate(...)` is a loud callout — a
speech bubble that **floats over the slide** to draw the eye. Like a footnote it
chains onto the block it refers to, and a block may carry several:

```python
p.image("arch.svg").annotate("注意这条曲线", at="top-right", dx=-12)
p.image("arch.svg").annotate("先看这里", at="center").annotate("再看这里", at="bottom")
```

`.annotate(text, *, at="top-right", dx=0, dy=0)`. `at` is one of nine anchors
naming where on the slide the bubble lands — `top-left`, `top`, `top-right`,
`left`, `center`, `right`, `bottom-left`, `bottom`, `bottom-right` (default
`top-right`). `dx` / `dy` nudge it from there in pixels (`+x` right, `+y` down).
The bubble sits *above* slide content and may cover it — that is the point: a note
can land in an already-full slide. Bubble text uses the same inline formatting as
a sidenote.

Positioning is the author's job — nothing measures a block's laid-out position, so
the bubble's tail is a cosmetic speech-bubble cue, not a precise arrow.

### Reveal a bubble as a build step

A page can hide all of its bubbles with `annotation=False`. Author the same body
twice — off, then on — and paging forward makes the callout pop in, no animation
machinery required:

```python
def fig(p):
    p.title("The jump")
    p.image("chart.svg").annotate("the jump happens here", at="center", dy=-40)

lec.page(id="jump-1", body=fig, annotation=False)  # chart only
lec.page(id="jump-2", body=fig, annotation=True)   # bubble revealed
```

(The two steps need distinct, unique `id`s, like any two pages.)

The two share a title, so the outline prints one row for them and the deck one
number — see [Pages titled the same](#pages-titled-the-same).
