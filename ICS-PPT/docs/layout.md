# Page layout: elements, boxes, alignment

How a page places what it holds. One mechanism serves every element: each
element has a box, two alignments place it in that box, and two containers
(a column and a row) hand boxes to the elements inside them. The older layout
calls (`side_image(valign=)`, `image_right`, `row`, `columns`, `gap`, `spacer`,
`under_side_image`) are described at the end in these terms.

## Elements

Everything on a page is a **`LecturekitElement`** (`lecturekit.model`). It
carries two fields; `aligned(**axes)` returns an immutable copy with either
field changed and validated:

| field | values | default |
| --- | --- | --- |
| `halign` | `left`, `center`, `right` | `None` |
| `valign` | `top`, `middle`, `bottom` | `None` |

Three classes are elements:

- **`Block`**, a leaf: every block kind of [dsl.md](dsl.md#blocks) (`slide`,
  `code`, `image` and each frame of `frames`, `table`, `architecture`,
  `highlight`, `demo`, `row`, `columns`, `sidenote`, `aside`, `link`,
  `spacer`).
- **`Flex`**, a container: a `direction` (`column` or `row`) and its
  children, which are elements.
- **`Page`**: the root container, a column holding the page's top-level
  elements. Its title is not one of them.

`None` means the theme's default for that element: text and code sit at the
left, figures, tables and highlight chips in the centre, everything at the top.

## Boxes

An element's **box** is the space its container gives it.

- The page's box is the band under the title: from the title's rule to the
  slide's bottom padding, across the content width (on a
  [side_image](dsl.md#side-images) page, across the text column).
- A **column** gives each child a box as wide as the column and as tall as the
  child.
- A **row** gives each child a box as tall as the row, which is as tall as its
  tallest child. An image with its own width (`width_px` / `width_pct`) keeps
  it; the other children share what is left equally, after the theme's gutter
  between neighbours.

## Alignment

Alignment places an element's content in its box. It moves the element as a
whole: a text block aligned `center` keeps its lines aligned with each other,
and a figure keeps its caption under it.

- On a **leaf**, the content is what the block draws.
- On a **container**, the content is its children. Along the container's
  direction they move as one group: `valign` on a column, `halign` on a row.
- An element whose alignment is unset on an axis takes its container's, so
  `valign="middle"` on a row sets every child against the row's middle unless
  a child says otherwise. The page passes its own to its top-level elements in
  the same way.

Where the box has no room to spare on an axis, alignment on that axis changes
nothing. A leaf in a column is exactly as tall as its box, so its `valign` has
no effect; a group taller than the page's band stays at the top.

## The DSL

Every element has the two methods; each returns its handle, so they chain with
`.footnote(...)` and the rest. The last call wins.

```python
p.valign("middle")                     # the page: its elements as one group
p.image("assets/a.svg").halign("left")  # one leaf
p.frames("f-1.svg", "f-2.svg").halign("right")   # every frame

with p.flex("row", valign="middle"):   # a container
    p.slide("the rule the figure shows")
    p.image("assets/b.svg", width_px=420)
```

`p.flex(direction="column", *, halign=None, valign=None)` opens a container
and is used as a `with` block: every block written inside it is its child,
and containers nest. Leaving it out, or leaving it empty, is a
`ValidationError`. `p.halign(...)` and `p.valign(...)` set the page's own.

A container takes no `only=`, footnotes or callouts: it is layout, and its
children carry their own. It may not hold a `side_image` or a block set
`under_side_image()`, which belong to the page's image column. The handle of a
`side_image` has no `halign` / `valign` either: the image column follows the
page (see below).

Alignment and `p.gap(...)` both claim the leftover space of the page, so a page
whose `valign` is `middle` or `bottom` cannot also call `p.gap(...)`.

In a row, percentages on an image are resolved against the row's width;
the image then fills that allotted box. The gutter is the theme token
`--layout-gutter`. A container is one unit for page-level `p.gap(...)`.
Empty containers after target filtering draw no box.

## In the model

A page's `blocks` stay one flat list in document order. A container is written
into it the way HTML writes an element, as an opening `flex` block and a
closing `end` block around its children; `model.layout(blocks)` reads the list
back into the tree of elements. Everything that reads blocks one by one (the
translation overlay, the book, the transcript, `p.frames`, review pages) sees
the same leaves in the same order as before, and passes the two markers over.

## Renderers

**Viewer** (HTML, PDF, PNG, live preview). The slide body is a flex column.
The page's `valign` puts a flexible spacer above its elements (`middle`,
`bottom`) and below them (`middle`). An element inside a container, or one
whose `halign` is set, is drawn in a box (`.lk-box`, a flex column whose
`align-items` / `justify-content` carry the alignment) around its content
(`.lk-content`, laid out like the slide body); a container is a box of boxes.
Elements without either render exactly as they always did.

The box itself carries reveal and held-frame state, so changing either state
keeps the same geometry. An annotation remains outside the boxes, in slide
coordinates; footnotes stay at the slide's bottom.

**PPTX.** Each element is drawn by its drawer into its box, then its shapes are
moved as one group: right or to the centre by the room the box leaves beside
them, down by the room below. A text box is first narrowed to its estimated
widest line. A table keeps the full width of its box, as PowerPoint draws it.

With the new alignment or containers, an animation reserves the finished
layout's space in PPTX too: held shapes are measured, then removed before the
file is written. Existing pages without the new API keep their old geometry.

**Transcript and book.** A printed page has no slide to align in: both ignore
`halign` and `valign`, and print a container's children in order.

## The older calls in these terms

| call | as elements |
| --- | --- |
| `p.side_image(..., valign=...)` | the page is a row of two boxes, the text column and the image column; `valign` is the page's `valign`, and the image column follows it. The image column keeps its own geometry (it runs to the slide's edges). |
| `p.slide(...).under_side_image()` | the image column is a column: the picture, then this text |
| `p.slide(...).image_right(src, valign="middle")` | a row of the text and the picture, both at `valign="middle"`; kept as one block so its translation key and its PowerPoint geometry stay. With `valign="top"` the text flows round the picture, which no box can express. |
| `p.row().image(...)` | one figure: a row of images under one caption and one `ref` |
| `p.columns(a, b)` | a row of text boxes, kept as one block with one key per column |
| `p.gap(...)` | the page column's rule for its leftover space: shared between the seams |
| `p.spacer(px)` | a leaf with a fixed height and nothing in it; PPTX keeps its legacy behaviour of ignoring it |
| `p.highlight(...)`, figures, tables | leaves whose default `halign` is `center` |
