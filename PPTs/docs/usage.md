# Usage

The full pipeline: how to inspect, build, render, and view a lecture source. For
what you can write in `lecture.py`, see [dsl.md](dsl.md).

Lecture sources live **outside** this repo. The examples use `$LEC` for a lecture
directory (a folder containing `lecture.py` + optional `pages.py` + `assets/`):

```bash
LEC=path/to/your/lecture
```

## Pipeline

Four commands, each doing one thing. `inspect` and `build` are pure and need no
Node; `render` and `view` produce a complete viewer bundle and run Marp (the
viewer's own tool) to build `slides.html`.

```bash
# 1. Print and validate the lecture tree (human-readable)
python3 -m lecturekit.cli inspect "$LEC"

# 2. Emit the full lecture AST as JSON on stdout (pipeable)
python3 -m lecturekit.cli build "$LEC" | jq .

# 3. Render a complete viewer bundle (writes build/<id>-viewer by default)
python3 -m lecturekit.cli render "$LEC" --out build/lecture-viewer

# 4. Render, build the deck, and open it in your browser
python3 -m lecturekit.cli view "$LEC"
```

## The rendered bundle

A rendered bundle contains:

- `lecture.json` — the viewer's projection of the AST
- `slides.md` — Marp source
- `index.html` / `viewer.css` / `viewer.js` / `outline.css` — the outline shell
- `outline.html` — a standalone print page used only for the PDF outline
- `slides.html` — the Marp-rendered deck (pages only)
- `demos.json` — `p.demo(...)` commands by id, rewritten every render (see
  [Running a demo from the deck](#running-a-demo-from-the-deck)); inert without
  a live server

The viewer shell **is** the outline: each page entry shows its slide number.
Click a page to open it as a Marp slide; use `←` / `→` to page and the
"≡ 大纲" button to return to the outline. The deck itself carries no outline
slide.

### Two numberings

An [animation](dsl.md#frames-an-animation) is several slides but one *idea*, and
the deck counts it as one. Every frame prints the **same** number, the count does
not advance across them, and the page after a three-frame animation at 12 is 13
— not 15. The outline row and a `p.cite(...)` backref use that same number, so
what the projector shows, what the outline lists, and what a reference points at
are one number.

A run of [pages titled the same](dsl.md#pages-titled-the-same) is treated the
same way, for the same reason: one title is one idea, so consecutive sibling
pages sharing one get a single outline row and a single number, held across the
run.

A [bridge page](dsl.md#the-tree) (`lec.bridge(...)`) goes further: it is zero
ideas, so it prints no number at all and the count does not advance — the pages
around it stay consecutively numbered, and the outline never lists it.

Build tooling still counts **slides**: `--pages 4` is the 4th slide in flat deck
order and `slides.004.png` is that slide's image, both unaffected by animations
(or bridges) earlier in the deck. So on a deck with animations the shown number is lower than
the `--pages` index — that is the one place the two numberings are meant to
disagree. Selecting a page by **id** (`--pages commit-logging`) sidesteps it.

**Presenter notes.** A page's `p.notes(...)` blocks are emitted into the deck as
Marp presenter notes. Open the deck (`slides.html`) and press `p` to open the
speaker view — it shows the notes and the next slide in a separate window, while
the audience-facing deck never renders them.

Open a previously rendered bundle later with:

```bash
scripts/view-viewer.sh build/lecture-viewer
```

### Marp is a viewer detail

Marp is not a separate target — it is the tool the viewer uses to turn `slides.md`
into `slides.html`, living in `lecturekit/renderers/viewer/marp.py`. Rendering
requires Node.

Pass `--no-build` to `render` / `view` to skip Marp and write only the text bundle
(no Node required).

`slides.html` also carries a small controller of ours, `assets/svg-scope.js`.
Marp bakes marpit-svg-polyfill into every deck; on WebKit that polyfill re-runs
over **every** slide on **every** animation frame, and each slide costs a forced
full-document layout. On a 241-slide lecture that measured 10.1 ms per frame in
Safari — 61% of a core, burned for as long as the tab is on screen. The
controller leaves the polyfill's `data-marpit-svg` on the slide you can actually
see and parks it on the rest, which brings the same pass to 0.05 ms. Overview,
presenter view, and printing show every slide, so there it is restored on all of
them.

### Rendering offline

Run this once, with network, and nothing in a render reaches out again:

```bash
scripts/prepare.sh          # vendors marp-cli into node_modules/
```

Marp is looked up in this order: `$LECTUREKIT_MARP`, the vendored
`node_modules/.bin/marp`, a `marp` on `PATH`, and finally `npx`. Only that last
one talks to the npm registry — and it is the one to avoid on a bad network:
resolving the package hangs for minutes when the network is up but the registry
is unreachable (npm's fetch timeout is 5 minutes, times three attempts), so
`marp --watch` never starts, `slides.html` is never rebuilt, and the live viewer
keeps showing the previous build. A watch session says so after 20 seconds
without a deck rather than leaving you to guess.

The deck itself is self-contained: fonts are bundled with the theme, so no slide
load fetches anything. Only the `--pdf`/`--png` exports need more than Node — a
local Chrome — and `npx` remains the fallback for a machine that has never run
`prepare.sh`.

## Rendering a subset of pages

Pass `--pages` to `render` / `view` to restrict the build to a subset — handy for
exporting a single slide. It is a one-shot selection, so it **cannot be combined
with `--watch`** (see [Live preview](#live-preview-edit-and-watch)); passing both
is an error. The selector is a comma-separated list of:

- a page id (e.g. `motivation`),
- the id of a page carrying [`p.frames(...)`](dsl.md#frames-an-animation), which
  selects every frame of that animation,
- a 1-based deck index in flat deck order (e.g. `4`), or
- an inclusive index range (e.g. `3-7`).

Pure-number tokens are always read as deck indices.

```bash
python3 -m lecturekit.cli render "$LEC" --pages 3-7        # a range
python3 -m lecturekit.cli render "$LEC" --pages 2,4,6      # specific pages
python3 -m lecturekit.cli render "$LEC" --pages motivation # one page by id
python3 -m lecturekit.cli render "$LEC" --pages commit-log # a whole animation
```

An animation's frames are pages in their own right, so `commit-log-2` picks one
frame out of it.

The whole bundle (`lecture.json`, the outline, `slides.md`, and any deck build)
reflects only the selected pages; sections survive only if they still hold one.
Unknown ids or out-of-range indices are an error.

## Rendering in another language

`--lang LANG` lays the lecture's `i18n/<LANG>.toml` translation overlay over the
text before anything is rendered, so the whole pipeline — viewer, PDF/PNG, PPTX,
transcript, book — produces that language. It works on `inspect`, `build`,
`render`, `view` and `book`; without it, the Python source renders as written.

```bash
python3 -m lecturekit.cli view "$LEC" --lang en          # -> build/<id>-en-viewer
python3 -m lecturekit.cli render "$LEC" --lang en --strict
python3 -m lecturekit.cli i18n extract "$LEC" --lang en  # write/refresh the overlay
python3 -m lecturekit.cli i18n check "$LEC" --lang en    # what is missing or stale
```

The default output directory carries the language (`build/<id>-<lang>-viewer`,
`-pptx`, …) so two languages of one lecture do not overwrite each other; an
explicit `--out` still wins. A string with no overlay entry keeps its baseline
text and is washed faintly on the slide; `--strict` (on `render`/`view`/`book`)
refuses to render instead. Under `--watch`, editing `i18n/<lang>.toml` reloads
the deck like any other source edit.

Naming a language with no overlay file is an error, not a silent fall back to
the baseline — `--lang em` is a typo, and you want to hear about it before class.

The full model, the key rules and the file format are in [i18n.md](i18n.md).

## PDF export

Pass `--pdf` to `render` / `view` to also export the deck. The exported PDF is
named after the lecture: its title is slugified (e.g. `Operating Systems` →
`operating-systems.pdf`), the same rule that derives section/page ids.

```bash
python3 -m lecturekit.cli render "$LEC" --out build/lecture-viewer --pdf
```

The PDF is assembled in three steps so a large outline never clips:

1. the outline is printed to its own content-sized page via headless Chrome
   (`outline.pdf`),
2. Marp exports the pages (`pages.pdf`), and
3. the two are merged into `<slug>.pdf` with pypdf — the intermediates are then
   removed.

Each page entry on the outline page is a clickable link that jumps to its slide:
Chrome prints the outline's `<a href>`s as link annotations and the merge step
rewrites them into internal go-to-page links.

This needs a local Chrome/Chromium (set `CHROME_PATH` if it is not on the default
path) and embeds a bundled CJK font (Noto Sans SC) so Chinese text and local
images render in Marp's headless Chromium. The live HTML viewer is unaffected and
keeps using your system fonts.

## PNG export

Pass `--png` to `render` / `view` to also export one image per page, named
`slides.001.png`, `slides.002.png`, … Combined with `--pages` this exports a
single page as an image:

```bash
python3 -m lecturekit.cli render "$LEC" --pages 4 --png   # -> slides.001.png
```

Like the PDF pages, PNGs render in headless Chromium with the bundled CJK font,
so it needs a local Chrome/Chromium. `--pdf` and `--png` can be combined.

## PowerPoint export

`render --to pptx` writes an **editable** PowerPoint deck instead of a viewer
bundle. Unlike the PDF/PNG exports (which render the Marp deck to images), this
is a native renderer: each page becomes a slide of real text frames, tables,
pictures, and shapes you can edit in PowerPoint — no Marp, no Chrome.

```bash
python3 -m lecturekit.cli render "$LEC" --to pptx          # -> build/<id>-pptx/<slug>.pptx
python3 -m lecturekit.cli render "$LEC" --to pptx --out build/deck
```

The deck is named after the lecture title (slugified), like the PDF export, and
`--pages` works the same way. It needs no Node or Chrome — only the
`python-pptx` dependency (plus an SVG renderer if the deck has vector figures,
see below).

### SVG figures

PowerPoint cannot embed SVG, so the renderer **rasterizes each `.svg` to a PNG**
on the way in — write `p.image("assets/fig.svg")` as usual and keep no second
copy of the figure. The PNG is temporary: its bytes go into the `.pptx` and the
file is discarded.

The conversion is delegated to whichever SVG renderer the machine has, in this
order: **rsvg-convert** (`brew install librsvg`), **cairosvg**
(`pip install cairosvg`), **Inkscape**. Rendering happens at 2× the figure's
intrinsic size, so it survives PowerPoint's zoom and print resampling.

With none of them installed — or on an SVG the backend chokes on — that one
figure is skipped, its caption is kept, and the CLI names the file on stderr.
The rest of the deck exports normally.

### Math

`$…$` and `$$…$$` are translated into **OMML**, PowerPoint's own equation
markup, so an equation is typeset by PowerPoint and stays editable in its
equation editor — it is not a picture. Display math is centered on its own line;
inline math sits in the run of text around it. It works in slide text, table
cells, asides, highlights, and footnotes — everywhere this renderer parses
inline markdown. Page titles, figure captions, and sidenote bodies are still
drawn as plain text here, so `$…$` in one of those reaches the slide as source
(as `**bold**` in one already does).

The translation covers what lecture math uses:

| | |
| --- | --- |
| scripts | `x_i`, `x^2`, `C_a^b` (folded into one base) |
| fractions and roots | `\frac{a}{b}`, `\sqrt{x}`, `\sqrt[3]{x}` |
| big operators | `\sum` `\prod` `\int` … with `_`/`^` limits |
| delimiters | `\left( … \right)`, auto-sized around what they hold |
| upright text | `\mathrm` `\operatorname` `\text` `\mathsf` `\mathbf` |
| script styles | `\mathbb` `\mathcal` `\mathfrak` |
| symbols | Greek, `\times` `\approx` `\in` `\to` `\infty` … |
| spacing | `\,` `\;` `\!` `\quad` `\qquad` |

Anything else — matrices, `align` environments, accents — is not parsed and
falls through as literal text, so an unrecognized command shows its own name
rather than taking the export down with it. Each equation also carries a
plain-text fallback for readers that do not implement the math extension.

A `$$` fence must be **indented by one space**; see the note in
[dsl.md](dsl.md#math-in-slides).

**Style is faithful but layout is approximate.** Colors, fonts, explicit image
sizes, and per-block styling are ported from the viewer theme
(`themes/basic-office.css`), but with no browser to measure text, blocks are
stacked top-to-bottom by an *estimated* height; spacing won't match the viewer
exactly. The text boxes auto-fit on open, so the rendered result is clean — just
not pixel-identical.

This first version covers the core blocks (slide markdown, code, link, image,
table, aside, sidenote, footnotes). The geometry-heavy blocks — architecture
diagrams, annotation bubbles, and slide float-image text-wrap — are skipped for
now; a deck that uses them still exports, just without those elements.

### Fonts

The deck names the viewer theme's fonts: **Lato** for Latin text and
**PingFang SC** for CJK. PowerPoint can only use locally-installed fonts, so for
the deck to match the viewer those fonts must be present (e.g.
`brew install --cask font-lato`). If PowerPoint can't resolve `PingFang SC` on a
machine (it is a reserved system font), Chinese may render as boxes; override the
CJK face with an installed one via `LECTUREKIT_PPTX_CJK_FONT`:

```bash
LECTUREKIT_PPTX_CJK_FONT="Hiragino Sans GB" \
  python3 -m lecturekit.cli render "$LEC" --to pptx
```

## Live preview: edit and watch

```bash
python3 -m lecturekit.cli view "$LEC" --watch
```

`--watch` starts a local dev server (default port `3030`, override with `--port`)
instead of opening a static `file://` page, and keeps rendering while you edit.
Saving any source file under the lecture directory (`lecture.py`, imported modules
like `pages.py`, or assets) re-renders the viewer and auto-refreshes the browser —
no manual reload.

A watch session always renders the **whole** lecture, and `--pages` is refused
alongside `--watch`. There is nothing to narrow: the viewer remembers which page
you are on (by page id) across every live reload, so watching the whole lecture
already lands you back on the page you are editing. A pinned selection could only
go stale — renaming or deleting the selected page would leave every later render
failing with no way to recover short of restarting.

A lecture that borrows [review pages](dsl.md#review-replaying-another-lectures-pages)
watches those source lectures too — they are part of this deck, so editing a
borrowed slide reloads it. Each extra root is named on startup. Adding or
dropping a review source mid-session re-arms the watcher on the next render; no
restart needed.

### Running a demo from the deck

A [`p.demo(...)`](dsl.md#demos) block draws the command on the slide with a ▶
button beside it. The button is dead everywhere except a watch session started
with `--demo`:

```bash
python3 -m lecturekit.cli view "$LEC" --watch --demo
```

Press it and the command runs **in the lecture directory**. Its output — stdout
and stderr merged — appears in a drawer along the bottom of the deck **as it is
produced**, not when the command is over: a model answering a token at a time is
watched, not waited for. The drawer counts the seconds while it runs and ends
with the exit status and the total. The slide itself does not move — whatever
`output=` recorded stays where it is.

Carriage returns are honoured, so a download's progress bar redraws one line
instead of printing four hundred; terminal escapes (a spinner, a colour) are
dropped, because the drawer is a `<pre>` and would otherwise show them.

**Several at once.** Every press of ▶ starts a *new* run and gives it a tab
along the top of the drawer, and runs on the same slide run side by side. That
is what a page needs when one of its commands is a server: `ollama serve` holds
its own tab while `ollama pull` and `ollama run` talk to it from the next two.
A tab is labelled with its command — `ollama run llama3.2 (2)` for the second
press of the same one — and carries a dot: pulsing while the run is alive, green
when it exited 0, red when it did not. Click a tab to read that run's output;
the ones behind it keep collecting theirs meanwhile.

**Putting things away.** ▾ in the head row hides the drawer and stops nothing;
a pill in the corner (`▲ 2 running`) brings it back, and `Escape` hides it too
(a second press goes back to being Marp's slide-grid toggle). ✕ *on a tab* is
the other thing: it closes that run — stopping it first if it is still going,
since the tab is the only handle it has — and the drawer closes with its last
tab. ■ stops the run you are looking at but keeps its tab, for when you want to
read what it printed. **Turning the page stops everything the slide started**:
every run of the slide you leave is killed and the rack emptied, so a lecture
never trails a server behind it.

There is no stop *request* behind any of this: the browser drops the connection,
and the server kills that command's whole process group when its next write has
nowhere to go. So a command with no natural end — `ollama serve` — is a
perfectly good demo.

A demo is killed after 120 seconds unless it says otherwise, and its output is
truncated past 512 KB. `--demo-timeout S` moves the default and `--demo-timeout 0`
removes it; a block's own [`timeout=`](dsl.md#demos) wins over both, and
`timeout=0` on a block is how `ollama serve` gets to keep running. The process
gets its own session, so a pipeline is killed whole rather than leaving its tail
behind. `--demo` without `--watch` is refused rather than silently doing nothing.

**The browser cannot invent a command.** The button carries a hash of the
command, not the command; the server maps that id back to what the author wrote via
`demos.json`, which every render rewrites. Nothing in a request can become a
command, an id that is not in the table runs nothing, and deleting the block
stops its id resolving on the next render. `.disable()` takes a demo out of the
table too.

What this does *not* do is make the deck safe to point at strangers. `--demo`
means the machine will run the lecture's own shell commands when someone presses
a button on a page served from `127.0.0.1` — it is the same trust you already
extend by letting the dev server import and execute `lecture.py`, scoped to
one more surface. Leave it off unless you are the one at the lectern.

While a demo runs — and for a moment after — file changes under the lecture
directory do **not** reload the deck. Otherwise the object file a demo just
compiled would look like an edit and refresh the page out from under its own
output. A source edit saved inside that window renders but waits for the next
save to reach the browser.

Under the hood it runs a persistent `marp --watch` process so each rebuild stays
warm, watches the lecture source for changes, and pushes a reload over
Server-Sent Events. marp's routine per-rebuild `INFO` logging is filtered out so
the console stays quiet; warnings and build errors still pass through. A render
error mid-edit is logged to the terminal and the last good output is kept on
screen; fix the file and it resumes. Press `Ctrl-C` to stop. Requires Node.

marp bakes its own WebSocket reload client into every deck it watches. Normally
that is the channel the slide iframe reloads on; under `--demo` it is stripped
and the deck rides the same Server-Sent Events channel as the rest of the
viewer, because only that one can tell a demo's build artifacts from an edit.

### Debouncing a burst of changes

Changes are **coalesced**: a batch of file writes renders once, `--debounce MS`
after the last write (default `400`). This is what keeps a bulk or automated
edit — a script rewriting many files with more than a few milliseconds between
them — from triggering one full render per file. The cost is a little live-reload
latency after a single hand edit; lower it for a snappier feel:

```bash
python3 -m lecturekit.cli view "$LEC" --watch --debounce 50    # snappy single-file reloads
python3 -m lecturekit.cli view "$LEC" --watch --debounce 1500  # settle a long automated batch
```

The window resets on every change, so as long as `MS` exceeds the gap between
writes, an entire burst collapses into a single render.

### Reveal-on-Enter

In `--watch` mode a page's body starts dimmed (grey). Press **Enter** to reveal
the next block; once every block on the page is shown, Enter pages to the next
slide. Paging back shows a slide fully. The reveal unit is the block — a `slide`
block reveals in one Enter, not line by line; a `side_image` background is always
visible, and a block's callout bubbles reveal with it.

One slide can ask for a finer unit:
[`p.slide(..., reveal="items")`](dsl.md#revealing-a-slides-bullets-one-at-a-time)
steps through that block's bullets one Enter at a time. Everything else on the
page still reveals block by block.

An [animation](dsl.md#frames-an-animation) is the exception: a block steps on
the frame where it first appears. The first frame steps through the blocks
written before `p.frames(...)`; the middle frames repeat that text, so dimming
them again would mean pressing Enter through the same bullets N times — they
arrive fully lit, one Enter apiece, which is what makes paging through them
feel like the animation running. The last frame then steps through the blocks
written after the animation (held blank until there), so the punchline still
takes its own Enter.

This is live-preview only: `render`, non-watch `view`, and the PDF/PNG exports are
unaffected and always render fully. The controller is injected into the served
`slides.html` in memory, so it never touches the on-disk deck.

Pass `--no-reveal` with `--watch` for a plain live preview (live-reload only, no
dimming or stepping):

```bash
python3 -m lecturekit.cli view "$LEC" --watch --no-reveal
```

## Test

```bash
python3 -m pytest              # the whole suite
python3 -m pytest tests/test_marp.py -v
python3 -m pytest -k highlight
```

Install it first with `pip install -e ".[dev]"`.

Most tests are stdlib `unittest.TestCase` classes, but a few need pytest's
`monkeypatch` / `tmp_path` fixtures and are written as bare `def test_*`
functions. **pytest is the runner for the suite**: `unittest discover` collects
the classes only and silently skips those files, so it reports a pass while
~50 tests never ran.
