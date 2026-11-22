"""Every block the deck and the PPTX draw, with every inline form inside them.

`tests/test_check_pptx.py` renders this lecture to both and compares them.
"""

from lecturekit.dsl import Lecture

lecture = Lecture(id="allblocks", title="Every block")
lecture.variable("T", 1.0, min=0, max=10)
lecture.cover("Every block (and field)", author="A_B & *C*", time="2026")


def text(p):
    p.title("Text with `code` and $x_i$")
    p.slide(r"""A headline with ==a mark== and ==orange:another==
- **bold** *italic* ***both*** ~~gone~~ `x < y`
  - nested, with a [link](https://example.org/a_b)
- escaped \*stars\* and &amp; an entity :smile:
- `--flag` opens this item

3. third
4. fourth

<div style="display:grid; grid-template-columns: 1fr 1fr"><div>left <b>cell</b></div><div>right<br>cell</div></div>
<span hidden>unseen</span>
""")


def formulas(p):
    p.title("Formulas")
    p.slide(r"""Inline $\hat{w} \oplus b$ and $\sum_{i=1}^{N} O(i) = O(N)$, then $55 and $4
 $$
 \begin{aligned} a &= \underbrace{b+c}_{\text{键}} \\ d &= \left(\frac{e}{\sqrt{f}}\right) \end{aligned}
 $$
 $$
 f(x) = \begin{cases} 1 & x > 0 \\ 0 & \text{否则} \end{cases}
 $$
""")


def raw(p):
    p.title("Blocks inside text")
    p.slide("""Before the listing

```c
int x = 1;

return x;
```

| name | cost |
|:--|--:|
| read | $O(1)$ |

> quoted

After <img src="assets/inline.png" width="24"> the picture
""")


def listings(p):
    p.title("Listings")
    p.code("c", "int a;\nint b = a < 1 && *p;").footnote("its *origin*")
    p.code("pseudo", "read(x)\nwrite(x)", mark=[2])
    p.demo("run", "cc main.c && ./a.out", output="42", description="what it prints",
           pwd="examples", files=["main.c"])
    p.demo("", "echo ok")


def data(p):
    p.title("Tables")
    p.table([["`a`", "**b**"], ["$n^2$", "x | y"]], headers=["First", "Second"],
            align=["left", "right"])
    p.columns("Left\n- one", "Right\n- two")
    p.highlight("The cost is $O(N)$", tone="orange")


def figures(p):
    p.title("Figures")
    p.image("assets/figure.png", alt="a figure", caption="A *caption* with $5").footnote("its source")
    row = p.row(caption="Two of them")
    row.image("assets/left.png", caption="left")
    row.image("assets/right.png", caption="right")


def containers(p):
    p.title("Side by side")
    with p.flex("row", halign="center"):
        p.slide("In a row")
        p.image("assets/inline.png")
    with p.flex("column"):
        p.slide("Under it")
        p.code("pseudo", "done")


def beside(p):
    p.title("Beside a picture")
    p.side_image("assets/side.png", side="left")
    p.slide("Text beside it").annotate("a bubble", at="bottom-right")
    p.sidenote("Note", "a **sidenote** with `code`")
    p.link("Reading", "https://example.org/reading")
    p.spacer(10)
    p.notes("for the lecturer alone")


def animation(p):
    p.title("An animation")
    p.slide("Above the frames")
    p.frames("assets/step-1.png", "assets/step-2.png", caption="Step by step")
    p.highlight("Held until the last frame")


def drawing(p):
    p.title("Layers")
    arch = p.architecture(caption="A stack")
    arch.layer("Top", ["app", "library"])
    arch.layer("Bottom", ["kernel"])
    p.aside("for the reader alone")


def controls(p):
    p.title("Controls")
    p.slide("The temperature is $T$")
    p.slider("T", marks=[0, 1, 10])
    p.input("T", label="*as written* $T$")
    p.cite(title="A Paper", author="Someone", year="2020")


with lecture.section("Blocks", id="blocks") as s:
    s.bridge("First part\nwhat *every* block says")
    for name, body in (
        ("text", text), ("formulas", formulas), ("raw", raw), ("listings", listings),
        ("data", data), ("figures", figures), ("containers", containers), ("beside", beside), ("animation", animation),
        ("drawing", drawing), ("controls", controls),
    ):
        s.page(name, body=body)
