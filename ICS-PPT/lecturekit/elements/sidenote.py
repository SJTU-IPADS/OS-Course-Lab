"""The `sidenote` element."""
from __future__ import annotations

from ..model import Block, element


@element
class Sidenote(Block):
    """A note in the margin column."""
    KIND = "sidenote"
    TARGETS = frozenset({"viewer", "pptx", "latex", "transcript"})

    def to_marp(self, ctx):
        from ..markup.fields import escape_html, inline_md
        from ..renderers.viewer.blocks import (
            SIDENOTE_WHEEL_SIZE, sidenote_classes, sidenote_logo_html,
        )
        # The first sidenote of a page keeps the bare markup; later ones cycle
        # through the theme's colors, so each counts itself in.
        index = ctx.sidenote_index
        ctx.sidenote_index += 1
        note = self.content
        title = f'{escape_html(note["title"])}：'
        if note.get("link"):
            title_html = (
                f'<a class="sidenote-title" href="{escape_html(note["link"])}"'
                f' target="_blank" rel="noopener noreferrer">{title}</a>'
            )
        else:
            title_html = f'<span class="sidenote-title">{title}</span>'
        logo_html = sidenote_logo_html(note.get("logo"))
        # Cycle through the theme's color wheel so several sidenotes on one slide
        # don't read as a monotonous block. The first (index 0) keeps the bare
        # markup and inherits --sidenote-1 from .sidenote; later ones pick the next
        # wheel slot via an inline background (positional CSS can't reach into
        # Marp's svg>foreignObject>section DOM, so the color is assigned here).
        style = ""
        if index:
            slot = index % SIDENOTE_WHEEL_SIZE + 1
            style = f' style="background: var(--sidenote-{slot})"'
        html = (
            f'<aside class="{sidenote_classes(note)}"{style}>{logo_html}'
            f'<p class="sidenote-body">{title_html}{inline_md(note["text"])}</p>'
            f'</aside>'
        )
        return [html, ""]

    def to_pptx(self, ctx):
        """The box, then the logo, then the text over them: the deck's reading order."""
        from dataclasses import replace
        from pptx.enum.shapes import MSO_SHAPE
        from pptx.util import Emu, Pt
        from .. import model
        from ..renderers.pptx import theme
        from ..renderers.pptx.blocks import (
            GAP,
            LOGO_SIZE,
            add_picture,
            natural_size,
            outline,
            resolve_image,
            shape_at,
            solid,
            textbox,
        )
        from ..renderers.pptx.draw import Look, runs_height, space_lines, write_runs
        from ..renderers.pptx.layout import fit_within
        from ..renderers.pptx.text import Run, runs
        note = self.content
        raw_logo = note.get("logo")
        fill = theme.SIDENOTE_WHEEL[ctx.sidenote_index % len(theme.SIDENOTE_WHEEL)]
        ctx.sidenote_index += 1

        # An image logo embeds as a picture in a left gutter (the theme floats it);
        # an emoji/glyph logo stays an inline text run. A path that won't resolve
        # falls back to the default book glyph rather than leaking the path as text.
        is_image = model.logo_is_image(raw_logo)
        logo = resolve_image(raw_logo, ctx) if is_image else None
        head = [
            replace(run, bold=True, link=note.get("link"))
            for run in runs(ctx.read.plain(f"{note['title']}："))
        ]
        items = head + runs(ctx.read.inline(note["text"]))
        if logo is None:
            glyph = "📖" if is_image or not raw_logo else raw_logo
            items.insert(0, Run(f"{glyph} "))

        inset, pad = Emu(Pt(10)), Emu(Pt(8))
        gutter = LOGO_SIZE + pad if logo else 0
        left, top = ctx.layout.content_left, ctx.cursor.top
        width = max(1, ctx.layout.content_width - 2 * inset - gutter)
        size = theme.SIDENOTE_PT
        text_height = runs_height(items, size, width)
        height = max(text_height, LOGO_SIZE if logo else 0) + 2 * pad
        box = shape_at(ctx, MSO_SHAPE.ROUNDED_RECTANGLE, left, top,
                       ctx.layout.content_width, height)
        solid(box, fill)
        outline(box, theme.SIDENOTE_BORDER, 1.0)
        if logo:
            logo_width, logo_height = fit_within(*natural_size(logo), LOGO_SIZE, LOGO_SIZE)
            add_picture(ctx, logo, raw_logo, left + inset, top + pad,
                        logo_width, logo_height)
        text, frame = textbox(ctx, text_height, left=left + inset + gutter, width=width)
        text.top = top + pad
        space_lines(frame.paragraphs[0], items, size)
        write_runs(frame.paragraphs[0], items, Look(size=size))
        ctx.cursor.place(height)
        ctx.cursor.advance(GAP)

    def to_latex(self, ctx):
        from ..renderers.latex.blocks import footnotes
        from ..renderers.latex.text import blocks as md_blocks, inline
        content = self.content
        title = inline(content["title"])
        if content.get("link"):
            title = r"\href{%s}{%s}" % (content["link"], title)
        body = md_blocks(content["text"], refs=ctx.resolve_ref) + footnotes(self, ctx)
        return "\\begin{booksidenote}{%s}\n%s\n\\end{booksidenote}" % (title, body)

    def to_transcript(self, ctx):
        import html
        from .. import model
        from ..renderers.transcript.text import escape
        content = self.content
        logo = content.get("logo") or "📖"
        title = ctx.inline(ctx.read.plain(content["title"]))
        if content.get("link"):
            url = html.escape(content["link"], quote=True)
            title = f'<a href="{url}">{title}</a>'
        # The logo may be an image path rather than a glyph; on a sheet that is one
        # more figure than the line is worth, so a path falls back to the default.
        if model.logo_is_image(logo):
            logo = "📖"
        return (
            f'<div class="tx-sidenote"><span class="tx-logo">{escape(logo)}</span>'
            f'<strong>{title}</strong> <p>{ctx.inline(ctx.read.inline(content["text"]))}</p></div>'
        )
