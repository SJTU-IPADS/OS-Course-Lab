# Rendering slides inline in Jupyter

A lecturekit page can render itself as a slide **inline in a notebook cell**,
pixel-identical to the deck. The notebook becomes the deck: build an ordinary
`Lecture` across cells with the normal DSL, and each `.page(...)` renders its
slide as the cell output. Live code demos are just ordinary cells that follow.

```python
import lecturekit.dsl as lk

lec = lk.Lecture(id="live", title="A Story of Scaling", assets=".")
s   = lec.section("Case study")

def compiler_slide(p):
    p.title("在课堂上第一次感受 ChatGPT 的威力")
    p.slide("**问 ChatGPT: 编译器会做怎样的优化?**")
    p.code("c", "int return_1() { return 1; }")
    p.image("assets/foo.png")   # resolves under <assets>/assets/foo.png

h = s.page("case", body=compiler_slide)
h                                  # cell output IS the rendered slide
```

```python
# next cell — an ordinary live demo
!gcc -O2 -S demo.c && cat demo.s
```

If the page body includes `p.news(...)` reading items, show them in a companion
cell:

```python
h.news()
```

The companion renders a compact "Related Reading" panel for that page. It is
not part of the slide deck.

## How it works

- `.page(...)` (on both `Lecture` and a section) returns a **`PageHandle`**. It
  implements `_repr_html_()`, so Jupyter (Lab, classic, VS Code, nbconvert)
  renders it as the cell output automatically — no explicit call needed.
- `PageHandle.news()` returns a second notebook-renderable object for the
  page's related reading material.
- The handle renders the one page through the **exact Marp pipeline** the deck
  and watcher use, so the inline slide is identical to the deck by construction.
  A page carrying [`p.frames(...)`](dsl.md#frames-an-animation) is several
  slides, so the cell shows the animation and arrows through it.
- Local `assets/…` images are inlined as `data:` URIs, so the slide is
  self-contained: it survives inside the cell's `iframe` and inside an exported
  notebook with no file server.

## The asset root

`Lecture(..., assets=<dir>)` names the directory that **contains** the `assets/`
folder, exactly like a lecture directory. Authors write the `assets/` prefix in
`src` (`p.image("assets/foo.png")`). Precedence for the asset root:

1. an explicit `assets=` passed to the renderer,
2. `Lecture(assets=...)`,
3. the module default `lecturekit.notebook.assets_dir` (defaults to `"."`, the
   notebook's cwd).

`assets` is inert for the normal CLI build, which derives the asset root from
the lecture directory.

## Caching

Rendering runs Marp (~1–2s uncached). Results are cached on a content hash of
the page's blocks + title + ratio + referenced asset mtimes, in two layers: an
in-process dict (instant within a session) and an on-disk bundle per hash
(skips Marp across kernel restarts). Re-running an unchanged cell is instant.
Only new or edited slides invoke Marp. Pass nothing special to use the cache;
force a fresh render from code with `notebook.render_page_html(..., cache=False)`.

Requires Node for the first (uncached) render of each slide, exactly like the
CLI `view`/`render` commands.
