"""The `link` element."""
from __future__ import annotations

from ..model import Block, element


@element
class Link(Block):
    """A hyperlink line."""
    KIND = "link"
    TARGETS = frozenset({"viewer", "pptx", "latex", "transcript"})

    def to_marp(self, ctx):
        from ..renderers.viewer.blocks import escape_html
        label = escape_html(self.content["label"])
        url = escape_html(self.content["url"])
        return [f'- <a href="{url}" target="_blank" rel="noopener noreferrer">{label}</a>', ""]

    def to_pptx(self, ctx):
        from ..renderers.pptx import theme
        from ..renderers.pptx.blocks import GAP, style_run, textbox
        from ..renderers.pptx.layout import estimate_text_height
        label = str(self.content["label"])
        url = str(self.content["url"])
        height = estimate_text_height(label, font_pt=theme.BODY_PT,
                                      width=ctx.layout.content_width)
        box, tf = textbox(ctx, height)
        p = tf.paragraphs[0]
        style_run(p.add_run(), "•  ", size_pt=theme.BODY_PT, color=theme.ACCENT1, bold=True)
        run = p.add_run()
        style_run(run, label, size_pt=theme.BODY_PT, color=theme.LINK)
        run.hyperlink.address = url
        ctx.cursor.place(height)
        ctx.cursor.advance(GAP)

    def to_latex(self, ctx):
        from ..renderers.latex.blocks import footnotes
        from ..renderers.latex.text import inline
        return r"\href{%s}{%s}%s" % (
            self.content["url"], inline(self.content["label"]), footnotes(self, ctx)
        )

    def to_transcript(self, embedder):
        import html
        from ..renderers.transcript.text import inline
        url = html.escape(self.content["url"], quote=True)
        return f'<p class="tx-link"><a href="{url}">{inline(self.content["label"])}</a></p>'
