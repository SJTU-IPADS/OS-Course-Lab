"""Per-block LaTeX emitters: ``(block, ctx) -> str``.

The book's block table deliberately omits ``slide``: slides and prose are
parallel bodies over the same page tree. ``notes``, ``side_image``,
``image_right`` and annotation bubbles are projection-only and never reach here.
"""

from __future__ import annotations

import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

from lecturekit import i18n, model, pseudo

from .assets import AssetCopier
from .text import blocks as md_blocks
from .text import MARK_COLORS, escape, inline

LATEX_KINDS = frozenset({
    "prose", "code", "link", "image", "row", "table",
    "sidenote", "architecture", "demo", "aside", "highlight",
})

# Fraction of \textwidth for an image the author never sized.
_DEFAULT_WIDTH = 0.8

_COLUMN_SPEC = {"left": "l", "center": "c", "right": "r"}

_FLOW_ARROWS = {"down": r"$\downarrow$", "up": r"$\uparrow$", "both": r"$\updownarrow$"}

# Tone -> the \definecolor name the preamble declares for the chip's ink.
_HIGHLIGHT_COLORS = {
    "yellow": "lkChipYellow",
    "orange": "lkChipOrange",
    "green": "lkChipGreen",
    "blue": "lkChipBlue",
}

# What graphicx can embed under XeLaTeX. A .gif is an animation: it has no place
# in print, and graphicx cannot even read its bounding box — so rather than fail
# the whole book, the figure renders as a labelled placeholder.
_EMBEDDABLE = frozenset({".png", ".jpg", ".jpeg", ".pdf", ".eps"})


@dataclass
class Ctx:
    lecture_id: str
    page_id: str
    slide_width: int
    assets: AssetCopier
    asset_root: Path | None = None
    # The chapter's language, for lecturekit's own fixed strings (the figure
    # prefix a `[@ref]` renders as, the demo box's title). The author's text is
    # already translated by the time it gets here; see `lecturekit.i18n`.
    lang: str | None = None
    # Figures are numbered per page so labels stay stable as pages move.
    figure_index: int = 0
    # Footnotes deferred out of a float; see `_figure`.
    pending_footnotes: list[str] = field(default_factory=list)

    def next_label(self) -> str:
        self.figure_index += 1
        # Page ids may hold underscores; a label is safest with none.
        page = self.page_id.replace("_", "-")
        return f"fig:{self.lecture_id}-{page}-{self.figure_index}"

    def ref_label(self, name: str) -> str:
        """The `\\label` key for an author-named figure ref.

        A bare name is namespaced by this lecture's id; ``lecid:name`` names a
        figure in another chapter (the author's job to get right — a chapter
        cannot see its siblings at render time).
        """
        lecture, _, bare = name.rpartition(":")
        return f"fig:{lecture or self.lecture_id}-{bare}"

    def resolve_ref(self, name: str) -> str:
        """``[@name]`` in prose → an inline figure reference."""
        return r"%s~\ref{%s}" % (i18n.ui(self.lang, "figure"), self.ref_label(name))


def emit_block(block: model.Block, ctx: Ctx) -> str:
    try:
        emitter = _EMITTERS[block.kind]
    except KeyError:
        raise model.ValidationError(
            f"LaTeX renderer cannot emit block kind: {block.kind}"
        )
    return emitter(block, ctx)


def emit_news(items: tuple[model.NewsItem, ...], lang: str | None = None) -> str:
    """A chapter's after-class reading, as an unnumbered section."""
    if not items:
        return ""
    lines = [
        r"\section*{%s}" % i18n.ui(lang, "further_reading"),
        r"\begin{itemize}",
    ]
    for item in items:
        entry = r"\item \href{%s}{%s}" % (item.url, inline(item.title))
        meta = " ".join(part for part in (item.source, item.date) if part)
        if meta:
            entry += f" — {inline(meta)}"
        if item.why:
            entry += f"\\\\ \\textit{{{inline(item.why)}}}"
        lines.append(entry)
    lines.append(r"\end{itemize}")
    return "\n".join(lines)


def emit_citations(
    citations: tuple[model.Citation, ...], lang: str | None = None
) -> str:
    """A chapter's references, as an unnumbered section (no deck page backrefs)."""
    if not citations:
        return ""
    lines = [
        r"\section*{%s}" % i18n.ui(lang, "references"),
        r"\begin{itemize}",
    ]
    for citation in citations:
        if citation.url:
            title = r"\href{%s}{%s}" % (citation.url, inline(citation.title))
        else:
            title = inline(citation.title)
        entry = r"\item " + title
        head = ", ".join(part for part in (citation.author, citation.year) if part)
        meta = ". ".join(part for part in (head, citation.venue) if part)
        if meta:
            entry += f" — {inline(meta)}"
        lines.append(entry)
    lines.append(r"\end{itemize}")
    return "\n".join(lines)


def _width_fraction(width: str | None, slide_width: int) -> float:
    """A DSL length (``"480px"`` / ``"60%"`` / ``None``) as a fraction of the text width."""
    if width is None:
        return _DEFAULT_WIDTH
    if width.endswith("%"):
        return float(width[:-1]) / 100
    if width.endswith("px"):
        return min(float(width[:-2]) / slide_width, 1.0)
    return _DEFAULT_WIDTH


def _fmt(value: float) -> str:
    return f"{value:g}"


def _footnotes(block: model.Block, ctx: Ctx) -> str:
    return "".join(
        r"\footnote{%s}" % inline(note, refs=ctx.resolve_ref)
        for note in block.footnotes
    )


def _graphic(path: str, width: str, framed: bool) -> str:
    """``\\includegraphics`` for an embeddable format, else a visible placeholder."""
    if Path(path).suffix.lower() not in _EMBEDDABLE:
        print(
            f"lecturekit: cannot embed {path} in LaTeX; rendering a placeholder",
            file=sys.stderr,
        )
        return r"\bookunrenderable{%s}" % escape(path)
    graphic = r"\includegraphics[width=%s]{%s}" % (width, path)
    return r"\fbox{%s}" % graphic if framed else graphic


def _includegraphics(content: dict, ctx: Ctx) -> str:
    path = ctx.assets.copy(ctx.lecture_id, ctx.asset_root, content["src"])
    fraction = _fmt(_width_fraction(content.get("width"), ctx.slide_width))
    return _graphic(path, rf"{fraction}\textwidth", bool(content.get("framed")))


def _figure(body: str, caption: str | None, block: model.Block, ctx: Ctx) -> str:
    """Wrap ``body`` in a centered float, with a caption, label, and footnotes.

    A ``\\footnote`` inside a float is silently dropped by LaTeX, so a figure's
    footnotes become a ``\\footnotemark`` in the caption plus a ``\\footnotetext``
    emitted after the float.

    An uncaptioned figure gets no ``\\caption`` at all — an empty one would still
    print a bare "Figure 3:" — and therefore no number to ``\\label``. Its
    footnote marks ride a ``\\caption*`` instead, which prints no number.

    An author-named ``ref`` on the figure replaces the per-page auto label, so
    prose can cite it with ``[@name]`` and the key survives page reshuffles.
    """
    lines = [r"\begin{figure}[htbp]", r"\centering", body]
    marks = r"\protect\footnotemark" * len(block.footnotes)
    if caption:
        lines.append(r"\caption{%s}" % (inline(caption, refs=ctx.resolve_ref) + marks))
        ref = block.content.get("ref") if isinstance(block.content, dict) else None
        lines.append(r"\label{%s}" % (ctx.ref_label(ref) if ref else ctx.next_label()))
    elif marks:
        lines.append(r"\caption*{%s}" % marks)
    lines.append(r"\end{figure}")
    for note in block.footnotes:
        lines.append(r"\footnotetext{%s}" % inline(note, refs=ctx.resolve_ref))
    return "\n".join(lines)


def _prose(block: model.Block, ctx: Ctx) -> str:
    return md_blocks(block.content, refs=ctx.resolve_ref) + _footnotes(block, ctx)


def _aside(block: model.Block, ctx: Ctx) -> str:
    body = md_blocks(block.content, refs=ctx.resolve_ref) + _footnotes(block, ctx)
    return "\\begin{quote}\n\\small %s\n\\end{quote}" % body


_DISPLAY_MATH_RE = re.compile(r"\$\$(.+?)\$\$", re.DOTALL)


def _display_math_inline(line: str) -> str:
    """``$$…$$`` -> ``$\\displaystyle …$``.

    A chip's rows are the cells of a one-column tabular, and TeX cannot open
    display math inside a cell. Inline math set in display style is the same
    formula at the same size, and it stays on the row it was written on.
    """
    return _DISPLAY_MATH_RE.sub(
        lambda m: r"$\displaystyle %s$" % m.group(1).strip(), line
    )


def _highlight(block: model.Block, ctx: Ctx) -> str:
    """A centered chip. Lines become tabular rows, so it grows taller, not wider.

    A plain ``\\footnote`` after the macro would land outside the ``center``
    group and open a paragraph of its own, dropping the mark in the margin. So
    the marks ride the macro's optional argument — just outside the chip, as in
    the deck — and the texts follow, the same split ``_figure`` uses.
    """
    content = block.content
    color = _HIGHLIGHT_COLORS[content["tone"]]
    body = r" \\ ".join(
        inline(_display_math_inline(line), refs=ctx.resolve_ref)
        for line in model.highlight_lines(content["text"])
    )
    marks = r"\footnotemark" * len(block.footnotes)
    out = r"\lkhighlight%s{%s}{%s}" % (f"[{marks}]" if marks else "", color, body)
    for note in block.footnotes:
        out += "\n" + r"\footnotetext{%s}" % inline(note, refs=ctx.resolve_ref)
    return out


# `pseudo` token kind -> the colour macro the preamble defines for it.
_PSEUDO_MACROS = {
    "keyword": r"\lkpskw", "message": r"\lkpsmsg",
    "state": r"\lkpsst", "comment": r"\lkpscm",
}


def _pseudo(content: str, marked: set[int], tone: str) -> str:
    """Lecture pseudocode, coloured by lecturekit's own lexer.

    `listings` cannot do this: `pseudo` has no grammar to hand it, and the
    categories that want colour (messages, state names) are not the ones it
    knows. So the block is lexed (see `lecturekit.pseudo`) and emitted as
    ordinary typeset text in a monospace environment. Spaces become `~` so the
    indentation survives — pseudocode is laid out by its indentation, and TeX
    would otherwise collapse it.

    A line in `marked` sits on a wash of `tone`, the marker pen at line scale.
    """
    lines = []
    for number, tokens in enumerate(pseudo.tokenize(content), start=1):
        parts = []
        for kind, text in tokens:
            body = escape(text).replace(" ", "~")
            macro = _PSEUDO_MACROS.get(kind)
            parts.append("%s{%s}" % (macro, body) if macro else body)
        line = "".join(parts) or "~"
        if number in marked:
            line = r"\lkpshl{%s}{%s}" % (MARK_COLORS[tone], line)
        lines.append(line)
    return "\\begin{lkpseudo}\n%s\n\\end{lkpseudo}" % "\\\\\n".join(lines)


def _code(block: model.Block, ctx: Ctx) -> str:
    language = block.content.get("language") or ""
    if language == "pseudo":
        return _pseudo(
            block.content["content"].strip("\n"),
            set(block.content.get("mark") or ()),
            block.content.get("tone") or model.DEFAULT_HIGHLIGHT_TONE,
        ) + _footnotes(block, ctx)
    option = f"[language={language}]" if language else ""
    return "\\begin{lstlisting}%s\n%s\n\\end{lstlisting}%s" % (
        option, block.content["content"], _footnotes(block, ctx)
    )


def _link(block: model.Block, ctx: Ctx) -> str:
    return r"\href{%s}{%s}%s" % (
        block.content["url"], inline(block.content["label"]), _footnotes(block, ctx)
    )


def _image(block: model.Block, ctx: Ctx) -> str:
    return _figure(_includegraphics(block.content, ctx), block.content.get("caption"), block, ctx)


def _row(block: model.Block, ctx: Ctx) -> str:
    items = block.content["items"]
    share = _fmt(0.9 / max(len(items), 1))
    parts = []
    for item in items:
        path = ctx.assets.copy(ctx.lecture_id, ctx.asset_root, item["src"])
        caption = inline(item["caption"]) if item.get("caption") else ""
        graphic = _graphic(path, r"\linewidth", bool(item.get("framed")))
        parts.append(
            r"\begin{subfigure}[t]{%s\textwidth}%s\caption*{%s}\end{subfigure}"
            % (share, graphic, caption)
        )
    return _figure("\n".join(parts), block.content.get("caption"), block, ctx)


def _table(block: model.Block, ctx: Ctx) -> str:
    headers = block.content["headers"]
    align = block.content.get("align") or ["left"] * len(headers)
    spec = "".join(_COLUMN_SPEC[a] for a in align)
    lines = [
        r"\begin{center}",
        r"\begin{tabular}{%s}" % spec,
        r"\toprule",
        " & ".join(inline(h) for h in headers) + r" \\",
        r"\midrule",
    ]
    for row in block.content["rows"]:
        lines.append(" & ".join(inline(cell) for cell in row) + r" \\")
    lines += [r"\bottomrule", r"\end{tabular}", r"\end{center}"]
    return "\n".join(lines) + _footnotes(block, ctx)


def _sidenote(block: model.Block, ctx: Ctx) -> str:
    content = block.content
    title = inline(content["title"])
    if content.get("link"):
        title = r"\href{%s}{%s}" % (content["link"], title)
    body = md_blocks(content["text"], refs=ctx.resolve_ref) + _footnotes(block, ctx)
    return "\\begin{booksidenote}{%s}\n%s\n\\end{booksidenote}" % (title, body)


def _demo(block: model.Block, ctx: Ctx) -> str:
    """A command as the book has to print it: verbatim, with its output.

    The book cannot run anything, so the recorded output is the whole of what a
    reader gets — and it goes in a listing rather than through `inline`, because
    a shell transcript is text a machine wrote and every character in it is
    load-bearing.
    """
    from lecturekit import demo as demo_module

    content = block.content
    lines = [
        r"\begin{tcolorbox}[colback=blue!3,colframe=blue!40,title=%s]"
        % i18n.ui(ctx.lang, "demo")
    ]
    lines.append(r"\textbf{%s}\\" % inline(content["name"]))
    if content.get("description"):
        lines.append(inline(content["description"]) + r"\\")
    transcript = demo_module.prompt_lines(str(content["command"]))
    if content.get("output"):
        transcript += str(content["output"]).strip("\n").split("\n")
    lines.append(
        "\\begin{lstlisting}\n%s\n\\end{lstlisting}" % "\n".join(transcript)
    )
    lines.append(r"\end{tcolorbox}")
    return "\n".join(lines) + _footnotes(block, ctx)


def _architecture(block: model.Block, ctx: Ctx) -> str:
    content = block.content
    arrow = _FLOW_ARROWS.get(content.get("flow") or "")
    layers = []
    for layer in content["layers"]:
        modules = " & ".join(inline(m) for m in layer["modules"])
        spec = "|" + "c|" * len(layer["modules"])
        rows = [r"\hline", modules + r" \\", r"\hline"]
        title = layer.get("title")
        head = r"\textbf{%s}\\[2pt]" % inline(title) if title else ""
        layers.append(
            "%s\\begin{tabular}{%s}\n%s\n\\end{tabular}" % (head, spec, "\n".join(rows))
        )
    separator = f"\n\n{arrow}\n\n" if arrow else "\n\n"
    return _figure(separator.join(layers), content.get("caption"), block, ctx)


_EMITTERS = {
    "prose": _prose,
    # A slide reaches the book only when the author forces it in with
    # `only=["latex"]` — "this passage is word for word the same in both". Its
    # content is the same markdown string a prose block holds, so it renders the
    # same way; without this entry that documented escape hatch raises.
    "slide": _prose,
    "aside": _aside,
    "highlight": _highlight,
    "code": _code,
    "link": _link,
    "image": _image,
    "row": _row,
    "table": _table,
    "sidenote": _sidenote,
    "demo": _demo,
    "architecture": _architecture,
}
