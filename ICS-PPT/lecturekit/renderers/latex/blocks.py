"""Helpers the elements' ``to_latex`` methods emit with.

Each element class (``lecturekit.elements``) says how it reads in the book; this
module holds what several of them share. ``slide`` has no ``to_latex``: slides
and prose are parallel bodies over the same page tree. ``notes``, ``side_image``
and annotation bubbles are projection-only and never reach here.
"""

from __future__ import annotations

import re
import sys
from dataclasses import dataclass
from pathlib import Path

from lecturekit import i18n, model, pseudo

from .assets import AssetCopier
from .text import MARK_COLORS, escape, inline

# Fraction of \textwidth for an image the author never sized.
DEFAULT_WIDTH = 0.8

COLUMN_SPEC = {"left": "l", "center": "c", "right": "r"}

FLOW_ARROWS = {"down": r"$\downarrow$", "up": r"$\uparrow$", "both": r"$\updownarrow$"}

# Tone -> the \definecolor name the preamble declares for the chip's ink.
HIGHLIGHT_COLORS = {
    "yellow": "lkChipYellow",
    "orange": "lkChipOrange",
    "green": "lkChipGreen",
    "blue": "lkChipBlue",
}

# What graphicx can embed under XeLaTeX. A .gif is an animation: it has no place
# in print, and graphicx cannot even read its bounding box — so rather than fail
# the whole book, the figure renders as a labelled placeholder.
EMBEDDABLE = frozenset({".png", ".jpg", ".jpeg", ".pdf", ".eps"})


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
    return block.to_latex(ctx)


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


def width_fraction(width: str | None, slide_width: int) -> float:
    """A DSL length (``"480px"`` / ``"60%"`` / ``None``) as a fraction of the text width."""
    if width is None:
        return DEFAULT_WIDTH
    if width.endswith("%"):
        return float(width[:-1]) / 100
    if width.endswith("px"):
        return min(float(width[:-2]) / slide_width, 1.0)
    return DEFAULT_WIDTH


def fmt(value: float) -> str:
    return f"{value:g}"


def footnotes(block: model.Block, ctx: Ctx) -> str:
    return "".join(
        r"\footnote{%s}" % inline(note, refs=ctx.resolve_ref)
        for note in block.footnotes
    )


def _graphic(path: str, width: str, framed: bool) -> str:
    """``\\includegraphics`` for an embeddable format, else a visible placeholder."""
    if Path(path).suffix.lower() not in EMBEDDABLE:
        print(
            f"lecturekit: cannot embed {path} in LaTeX; rendering a placeholder",
            file=sys.stderr,
        )
        return r"\bookunrenderable{%s}" % escape(path)
    graphic = r"\includegraphics[width=%s]{%s}" % (width, path)
    return r"\fbox{%s}" % graphic if framed else graphic


def includegraphics(content: dict, ctx: Ctx) -> str:
    path = ctx.assets.copy(ctx.lecture_id, ctx.asset_root, content["src"])
    fraction = fmt(width_fraction(content.get("width"), ctx.slide_width))
    return _graphic(path, rf"{fraction}\textwidth", bool(content.get("framed")))


def figure(body: str, caption: str | None, block: model.Block, ctx: Ctx) -> str:
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


DISPLAY_MATH_RE = re.compile(r"\$\$(.+?)\$\$", re.DOTALL)


def display_math_inline(line: str) -> str:
    """``$$…$$`` -> ``$\\displaystyle …$``.

    A chip's rows are the cells of a one-column tabular, and TeX cannot open
    display math inside a cell. Inline math set in display style is the same
    formula at the same size, and it stays on the row it was written on.
    """
    return DISPLAY_MATH_RE.sub(
        lambda m: r"$\displaystyle %s$" % m.group(1).strip(), line
    )


# `pseudo` token kind -> the colour macro the preamble defines for it.
PSEUDO_MACROS = {
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
            macro = PSEUDO_MACROS.get(kind)
            parts.append("%s{%s}" % (macro, body) if macro else body)
        line = "".join(parts) or "~"
        if number in marked:
            line = r"\lkpshl{%s}{%s}" % (MARK_COLORS[tone], line)
        lines.append(line)
    return "\\begin{lkpseudo}\n%s\n\\end{lkpseudo}" % "\\\\\n".join(lines)


PARBOX_ALIGN = {"left": r"\raggedright", "center": r"\centering", "right": r"\raggedleft"}


def width_column(align: str, share: float) -> str:
    r"""A `p` column taking ``share`` of the line, less its own padding.

    Each column pads both sides by \tabcolsep, so taking it off every column
    keeps the table exactly \linewidth wide.
    """
    return r">{%s\arraybackslash}p{\dimexpr %s\linewidth-2\tabcolsep\relax}" % (
        PARBOX_ALIGN[align], fmt(share)
    )


# Delimiter pairs for the weights of demo output lines under `bold=`, tried in
# order; the first two the output does not contain are used. `listings` drops
# them from the page.
WEIGHT_DELIMS = [("(@", "@)"), ("<@", "@>"), ("[@", "@]"), ("(!", "!)"), ("<!", "!>")]


