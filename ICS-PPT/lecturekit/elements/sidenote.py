"""The `sidenote` element."""
from __future__ import annotations

from ..model import Block, element


@element
class Sidenote(Block):
    """A note in the margin column."""
    KIND = "sidenote"
    TARGETS = frozenset({"viewer", "pptx", "latex", "transcript"})

    def to_marp(self, ctx):
        from ..renderers.viewer.blocks import (
            SIDENOTE_WHEEL_SIZE, escape_html, inline_md, sidenote_classes, sidenote_logo_html,
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
        from pptx.enum.shapes import MSO_SHAPE
        from pptx.util import Emu, Pt
        from .. import model
        from ..renderers.pptx import theme
        from ..renderers.pptx.blocks import (
            GAP,
            LOGO_SIZE,
            _line,
            _shape,
            apply_runs,
            resolve_image,
            set_line_spacing,
            solid,
            style_run,
        )
        from ..renderers.pptx.layout import estimate_text_height, fit_within
        from ..renderers.pptx.text import parse_inline
        note = self.content
        raw_logo = note.get("logo")
        title = f"{note['title']}："
        body = str(note["text"])
        fill = theme.SIDENOTE_WHEEL[ctx.sidenote_index % len(theme.SIDENOTE_WHEEL)]
        ctx.sidenote_index += 1

        # An image logo embeds as a picture in a left gutter (the theme floats it);
        # an emoji/glyph logo stays an inline text run. A path that won't resolve
        # falls back to the default book glyph rather than leaking the path as text.
        logo_path = resolve_image(raw_logo, ctx) if model.logo_is_image(raw_logo) else None
        logo_glyph = "" if logo_path is not None else (raw_logo or "📖")

        height = estimate_text_height(
            logo_glyph + title + body, font_pt=theme.SIDENOTE_PT,
            width=ctx.layout.content_width,
        ) + Emu(Pt(16))
        box_top = ctx.cursor.top
        shape = _shape(ctx, MSO_SHAPE.ROUNDED_RECTANGLE, ctx.layout.content_left,
                       box_top, ctx.layout.content_width, height)
        solid(shape, fill)
        _line(shape, theme.SIDENOTE_BORDER, 1.0)
        tf = shape.text_frame
        tf.word_wrap = True
        if logo_path is not None:
            tf.margin_left = LOGO_SIZE + Pt(16)
        p = tf.paragraphs[0]
        set_line_spacing(p, theme.SIDENOTE_PT)
        if logo_glyph:
            style_run(p.add_run(), logo_glyph + " ", size_pt=theme.SIDENOTE_PT, color=theme.FG)
        title_run = p.add_run()
        style_run(title_run, title, size_pt=theme.SIDENOTE_PT, color=theme.ACCENT1, bold=True)
        if note.get("link"):
            title_run.hyperlink.address = note["link"]
        apply_runs(p, parse_inline(body), size_pt=theme.SIDENOTE_PT, color=theme.FG)

        if logo_path is not None:
            pic = ctx.slide.shapes.add_picture(
                str(logo_path), ctx.layout.content_left + Pt(10), box_top + Pt(8),
                width=LOGO_SIZE,
            )
            # keep the logo inside the box even for a tall source image
            if pic.height > height - Pt(16):
                pic.width, pic.height = fit_within(pic.width, pic.height, LOGO_SIZE, height - Pt(16))

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

    def to_transcript(self, embedder):
        import html
        from .. import model
        from ..renderers.transcript.text import escape, inline, markdown
        content = self.content
        logo = content.get("logo") or "📖"
        title = inline(content["title"])
        if content.get("link"):
            url = html.escape(content["link"], quote=True)
            title = f'<a href="{url}">{title}</a>'
        # The logo may be an image path rather than a glyph; on a sheet that is one
        # more figure than the line is worth, so a path falls back to the default.
        if model.logo_is_image(logo):
            logo = "📖"
        return (
            f'<div class="tx-sidenote"><span class="tx-logo">{escape(str(logo))}</span>'
            f'<strong>{title}</strong> {markdown(content["text"])}</div>'
        )
