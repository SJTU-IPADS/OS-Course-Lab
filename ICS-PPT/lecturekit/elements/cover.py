"""The `cover` element."""
from __future__ import annotations

from ..model import Block, element
from .. import model


@element
class Cover(Block):
    """The cover page: title, author, time, logo."""
    KIND = "cover"
    TARGETS = frozenset({"viewer", "pptx"})
    IN_CONTAINER = False
    REVEAL_STEP = False

    def check_content(self, page_id: str) -> None:
        content = self.content
        for field_name in ("author", "time"):
            value = content.get(field_name)
            if value is not None and (not isinstance(value, str) or not value.strip()):
                raise model.ValidationError(
                    f"Empty cover {field_name} on {page_id}"
                )
        logo = content.get("logo")
        if logo is None:
            return
        if isinstance(logo, str):
            if not logo.strip():
                raise model.ValidationError(f"Empty cover logo on {page_id}")
            return
        if not isinstance(logo, dict):
            raise model.ValidationError(
                f"Cover logo on {page_id} must be a string or left/right mapping"
            )
        for key in logo:
            if key not in ("left", "right"):
                raise model.ValidationError(
                    f"Cover logo on {page_id} has unknown slot {key!r}"
                )
        for key in ("left", "right"):
            value = logo.get(key)
            if value is not None and (not isinstance(value, str) or not value.strip()):
                raise model.ValidationError(
                    f"Empty cover logo {key} on {page_id}"
                )

    def to_marp(self, ctx):
        return []

    def to_pptx(self, ctx):
        """The cover owns the whole slide, title included."""
        from pptx.enum.text import MSO_AUTO_SIZE, PP_ALIGN
        from ..renderers.pptx import theme
        from ..renderers.pptx.blocks import (
            COVER_ACCENT,
            COVER_META_PT,
            COVER_TIME_PT,
            COVER_TITLE_PT,
            draw_cover_logos,
        )
        from ..renderers.pptx.draw import Look, space_lines, write_runs
        from ..renderers.pptx.text import runs
        cover = self.content
        draw_cover_logos(cover.get("logo"), ctx)

        width = int(ctx.layout.width * 0.78)
        left = (ctx.layout.width - width) // 2

        def write(top, height, auto_size, lines):
            box = ctx.slide.shapes.add_textbox(left, int(ctx.layout.height * top),
                                               width, int(ctx.layout.height * height))
            frame = box.text_frame
            frame.word_wrap = True
            frame.auto_size = auto_size
            frame.margin_left = frame.margin_right = 0
            frame.margin_top = frame.margin_bottom = 0
            for number, (items, look) in enumerate(lines):
                paragraph = frame.add_paragraph() if number else frame.paragraphs[0]
                paragraph.alignment = PP_ALIGN.CENTER
                space_lines(paragraph, items, look.size)
                write_runs(paragraph, items, look)

        write(0.24, 0.26, MSO_AUTO_SIZE.TEXT_TO_FIT_SHAPE, [
            (runs(ctx.read.cover_title(ctx.title)),
             Look(size=COVER_TITLE_PT, color=COVER_ACCENT)),
        ])
        looks = {
            "author": Look(size=COVER_META_PT, color=theme.FG, bold=True),
            "time": Look(size=COVER_TIME_PT, color=theme.DK2),
        }
        write(0.57, 0.2, MSO_AUTO_SIZE.SHAPE_TO_FIT_TEXT, [
            (runs(ctx.read.plain(cover[key])), look)
            for key, look in looks.items() if cover.get(key)
        ])
