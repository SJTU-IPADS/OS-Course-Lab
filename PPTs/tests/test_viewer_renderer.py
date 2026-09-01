import json
import tempfile
import unittest
from pathlib import Path

from lecturekit import Lecture
from lecturekit.renderers.viewer import (
    StaticViewerRenderer,
    build_marp_markdown,
    build_outline_html,
    render_marp_page,
)


def sample_lecture() -> Lecture:
    lecture = Lecture(id="lec01", title="LEC 1", subtitle="Sub")

    def opening(p):
        p.title("一句话指令背后的庞然大物")
        p.slide("- world")
        p.code("python", "print('hi')")
        p.link("OSTEP", "https://example.com")
        p.image("assets/x.svg", alt="x", caption="cap")
        p.aside("aside text")
        p.notes("teacher only")

    with lecture.section("开场", id="opening", collapsed=False) as section:
        section.page("agent-motivation", body=opening)
    return lecture


class StaticViewerRendererTest(unittest.TestCase):
    def render_to_tmp(self, tmp):
        StaticViewerRenderer().render(sample_lecture().build(), Path(tmp))

    def test_writes_static_app_files(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.render_to_tmp(tmp)
            for name in ("index.html", "viewer.css", "outline.css", "viewer.js", "lecture.json"):
                self.assertTrue(Path(tmp, name).exists(), f"missing {name}")

    def test_outline_styles_live_in_outline_css_only(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.render_to_tmp(tmp)
            outline_css = Path(tmp, "outline.css").read_text(encoding="utf-8")
            viewer_css = Path(tmp, "viewer.css").read_text(encoding="utf-8")
        self.assertIn(".outline .row", outline_css)
        self.assertIn("--icon", outline_css)
        self.assertNotIn(".outline .row", viewer_css)

    def test_lecture_json_has_nested_tree_and_linear_pages(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.render_to_tmp(tmp)
            data = json.loads(Path(tmp, "lecture.json").read_text(encoding="utf-8"))

        self.assertEqual(data["lecture"]["id"], "lec01")
        self.assertEqual(data["lecture"]["title"], "LEC 1")

        section = data["tree"][0]
        self.assertEqual(section["type"], "section")
        self.assertEqual(section["id"], "opening")
        self.assertFalse(section["collapsed"])
        self.assertEqual(section["children"][0]["type"], "page")
        self.assertEqual(section["children"][0]["id"], "agent-motivation")

        self.assertEqual(len(data["pages"]), 1)
        page = data["pages"][0]
        self.assertEqual(page["id"], "agent-motivation")
        self.assertEqual(page["title"], "一句话指令背后的庞然大物")

    def test_pages_only_contain_viewer_visible_blocks(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.render_to_tmp(tmp)
            data = json.loads(Path(tmp, "lecture.json").read_text(encoding="utf-8"))

        kinds = [b["kind"] for b in data["pages"][0]["blocks"]]
        self.assertEqual(kinds, ["slide", "code", "link", "image", "aside"])
        self.assertNotIn("notes", kinds)

    def test_index_html_embeds_lecture_json(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.render_to_tmp(tmp)
            html = Path(tmp, "index.html").read_text(encoding="utf-8")
            data = json.loads(Path(tmp, "lecture.json").read_text(encoding="utf-8"))

        self.assertIn('<script type="application/json" id="lecture-data">', html)
        self.assertIn(json.dumps(data, ensure_ascii=False), html)

    def test_writes_marp_slides_md(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.render_to_tmp(tmp)
            md = Path(tmp, "slides.md").read_text(encoding="utf-8")
        self.assertIn("marp: true", md)
        self.assertIn("math: mathjax", md)
        self.assertIn("theme: basic-office", md)
        self.assertIn("paginate: true", md)
        self.assertIn("# 一句话指令背后的庞然大物", md)

    def test_pagination_can_be_disabled(self):
        lecture = sample_lecture().build()
        self.assertIn("paginate: true", build_marp_markdown(lecture))
        self.assertIn("paginate: false", build_marp_markdown(lecture, paginate=False))
        with tempfile.TemporaryDirectory() as tmp:
            StaticViewerRenderer(paginate=False).render(lecture, Path(tmp))
            md = Path(tmp, "slides.md").read_text(encoding="utf-8")
        self.assertIn("paginate: false", md)
        self.assertNotIn("paginate: true", md)

    def test_slides_md_renders_page_title_once_from_page_metadata(self):
        lecture = Lecture(id="lec", title="L")

        def body(p):
            p.title("Page Title")
            p.slide("- point")

        with lecture.section("S", id="s") as section:
            section.page("p1", body=body)

        with tempfile.TemporaryDirectory() as tmp:
            StaticViewerRenderer().render(lecture.build(), Path(tmp))
            md = Path(tmp, "slides.md").read_text(encoding="utf-8")

        # the body must not re-emit the title; it appears once as the slide
        # heading (the outline slide also lists it, but never as an `# ` heading)
        self.assertEqual(md.count("# Page Title"), 1)

    def test_cover_page_renders_as_cover_markup_not_normal_heading(self):
        lecture = Lecture(id="lec", title="L")
        lecture.cover(
            "Elastic model serving via efficient autoscaling",
            author="Xingda Wei",
            time="July 2026",
            logo=("assets/ipads.svg", "assets/sjtu.svg"),
        )

        md = render_marp_page(lecture.build().children[0])

        self.assertIn("<!-- _class: cover -->", md)
        self.assertIn('class="lk-cover-title"', md)
        self.assertIn("Elastic model serving via efficient autoscaling", md)
        self.assertIn("Xingda Wei", md)
        self.assertIn("July 2026", md)
        self.assertIn('src="assets/ipads.svg"', md)
        self.assertIn('src="assets/sjtu.svg"', md)
        self.assertNotIn("# Elastic model serving", md)

    def test_parenthetical_title_text_stays_plain_in_slide_heading(self):
        page = sample_lecture().build().children[0].children[0]
        page = page.__class__(
            id=page.id,
            title="Scalability Challenge: Consistency（Eventual vs. Linearzability)",
            blocks=page.blocks,
        )

        md = render_marp_page(page)

        self.assertIn(
            "# Scalability Challenge: Consistency（Eventual vs. Linearzability)",
            md,
        )
        self.assertNotIn("title-parenthetical", md)

    def test_parenthetical_title_text_stays_plain_in_outline(self):
        lecture = Lecture(id="lec", title="L")

        def body(p):
            p.title("Scalability Challenge: Consistency（Eventual vs. Linearzability)")
            p.slide("hello")

        lecture.page("p1", body=body)

        html = build_outline_html(lecture.build())

        self.assertIn(
            "Scalability Challenge: Consistency（Eventual vs. Linearzability)",
            html,
        )
        self.assertNotIn("title-parenthetical", html)
        self.assertNotIn("&lt;span", html)

    def test_emph_title_markup_is_underlined_and_removed_from_slide_heading(self):
        page = sample_lecture().build().children[0].children[0]
        page = page.__class__(
            id=page.id,
            title=r"Scalability Challenge: \emph{Consistency}",
            blocks=page.blocks,
        )

        md = render_marp_page(page)

        self.assertIn(
            '# Scalability Challenge: <span class="title-parenthetical">Consistency</span>',
            md,
        )
        self.assertNotIn(r"\emph", md)

    def test_emph_title_markup_is_underlined_and_removed_from_outline(self):
        lecture = Lecture(id="lec", title="L")

        def body(p):
            p.title(r"Scalability Challenge: \emph{Consistency}")
            p.slide("hello")

        lecture.page("p1", body=body)

        html = build_outline_html(lecture.build())

        self.assertIn(
            'Scalability Challenge: <span class="title-parenthetical">Consistency</span>',
            html,
        )
        self.assertNotIn(r"\emph", html)

    def test_slides_md_has_one_slide_separator_per_extra_page(self):
        lecture = Lecture(id="lec", title="L")

        def one(p):
            p.title("One")
            p.slide("hello")

        def two(p):
            p.title("Two")
            p.slide("hello")

        with lecture.section("S", id="s") as section:
            section.page("p1", body=one)
            section.page("p2", body=two)

        with tempfile.TemporaryDirectory() as tmp:
            StaticViewerRenderer().render(lecture.build(), Path(tmp))
            md = Path(tmp, "slides.md").read_text(encoding="utf-8")

        # front-matter close + page1->page2 = 2 separators (no outline slide)
        self.assertEqual(md.count("\n---\n"), 2)

    def test_slides_md_has_no_outline_slide(self):
        lecture = Lecture(id="lec", title="Deck Title", subtitle="A subtitle")

        def one(p):
            p.title("First Page")
            p.slide("hello")

        def two(p):
            p.title("Second Page")
            p.slide("hello")

        with lecture.section("Part I", id="s1") as section:
            section.page("p1", body=one)
            section.page("p2", body=two)

        with tempfile.TemporaryDirectory() as tmp:
            StaticViewerRenderer().render(lecture.build(), Path(tmp))
            md = Path(tmp, "slides.md").read_text(encoding="utf-8")

        # the deck is pages-only: no outline slide, no inlined outline styling
        self.assertNotIn('<div class="outline">', md)
        self.assertNotIn("<style>", md)
        # the first content slide leads the body (nothing precedes its heading
        # but the front-matter)
        before_first_page = md.split("# First Page")[0]
        self.assertNotIn("outline", before_first_page)
        self.assertEqual(md.count("# First Page"), 1)

    def test_auto_gap_page_emits_gap_flow_and_spacers(self):
        lecture = Lecture(id="lec", title="L")

        def body(p):
            p.title("Gap")
            p.gap("auto", min_px=6, max_px=30)
            p.slide("one")
            p.slide("two")
            p.notes("teacher")

        lecture.page("gap", body=body)
        md = render_marp_page(lecture.build().children[0])

        self.assertIn("<!-- _class: lk-gap-auto -->", md)
        self.assertIn(
            '<div class="lk-gap-flow" style="--lk-gap-min: 6px; --lk-gap-max: 30px;">',
            md,
        )
        self.assertEqual(md.count('class="lk-gap-block"'), 2)
        self.assertEqual(md.count('class="lk-gap-spacer"'), 1)
        self.assertIn("<!--\nteacher\n-->", md)
        self.assertTrue(md.index("<!--\nteacher\n-->") > md.index("</div>"))

    def test_gap_fill_emits_an_uncapped_spacer(self):
        lecture = Lecture(id="lec", title="L")

        def body(p):
            p.title("Gap")
            p.gap("fill")
            p.slide("one")
            p.slide("two")

        lecture.page("gap", body=body)
        md = render_marp_page(lecture.build().children[0])

        self.assertIn(
            '<div class="lk-gap-flow" style="--lk-gap-min: 8px; --lk-gap-max: none;">',
            md,
        )

    def test_pages_without_auto_gap_emit_no_gap_markup(self):
        page = sample_lecture().build().children[0].children[0]
        md = render_marp_page(page)

        self.assertNotIn("lk-gap-auto", md)
        self.assertNotIn("lk-gap-flow", md)
        self.assertNotIn("lk-gap-block", md)
        self.assertNotIn("lk-gap-spacer", md)

    def test_auto_gap_reveal_uses_gap_block_as_reveal_wrapper(self):
        lecture = Lecture(id="lec", title="L")

        def body(p):
            p.title("Gap")
            p.gap("auto")
            p.slide("one")
            p.slide("two").annotate("look", at="center")

        lecture.page("gap", body=body)
        md = render_marp_page(lecture.build().children[0], reveal=True)

        self.assertIn('class="lk-gap-block reveal-block" data-reveal="0"', md)
        self.assertIn('class="lk-gap-block reveal-block" data-reveal="1"', md)
        self.assertNotIn('class="reveal-block"', md)
        self.assertIn('data-reveal="1" class="annotation', md)
        self.assertEqual(md.count('class="lk-gap-spacer"'), 1)

    def test_spacer_block_renders_fixed_height_divs(self):
        lecture = Lecture(id="lec", title="L")

        def body(p):
            p.title("Spacer")
            p.slide("one")
            p.spacer(32)
            p.slide("two")
            p.spacer(8)
            p.slide("three")

        lecture.page("sp", body=body)
        md = render_marp_page(lecture.build().children[0])

        # a page may carry several spacers, each an exact height
        self.assertIn(
            '<div class="lk-spacer" aria-hidden="true" style="height:32px"></div>', md
        )
        self.assertIn(
            '<div class="lk-spacer" aria-hidden="true" style="height:8px"></div>', md
        )
        self.assertEqual(md.count('class="lk-spacer"'), 2)

    def test_spacer_is_always_visible_in_reveal(self):
        lecture = Lecture(id="lec", title="L")

        def body(p):
            p.title("Spacer")
            p.slide("one")
            p.spacer(24)
            p.slide("two")

        lecture.page("sp", body=body)
        md = render_marp_page(lecture.build().children[0], reveal=True)

        # the two slides get reveal wrappers; the spacer is never wrapped and
        # never consumes a reveal index
        self.assertIn('data-reveal="0"', md)
        self.assertIn('data-reveal="1"', md)
        self.assertNotIn('data-reveal="2"', md)
        spacer_html = (
            '<div class="lk-spacer" aria-hidden="true" style="height:24px"></div>'
        )
        self.assertIn(spacer_html, md)
        self.assertNotIn(f'<div class="reveal-block" data-reveal', spacer_html)

    def test_highlight_renders_a_centered_toned_chip(self):
        lecture = Lecture(id="lec01", title="L")

        def body(p):
            p.title("T")
            p.highlight("Underloaded")
            p.highlight("Overloaded", tone="orange")

        lecture.page("p1", body=body)
        md = build_marp_markdown(lecture.build())
        self.assertIn(
            '<p class="lk-highlight lk-highlight--yellow">'
            "<span>Underloaded</span></p>",
            md,
        )
        self.assertIn(
            '<p class="lk-highlight lk-highlight--orange">'
            "<span>Overloaded</span></p>",
            md,
        )

    def test_highlight_keeps_lines_in_one_chip(self):
        lecture = Lecture(id="lec01", title="L")
        lecture.page(
            "p1",
            body=lambda p: (
                p.title("T"), p.highlight("  Underloaded  \n 一半的队列是空的 ")
            ),
        )
        md = build_marp_markdown(lecture.build())
        # one span, lines joined by <br>, each line stripped
        self.assertIn("<span>Underloaded<br>一半的队列是空的</span>", md)

    def test_highlight_renders_inline_markdown_and_escapes_html(self):
        lecture = Lecture(id="lec01", title="L")
        lecture.page(
            "p1",
            body=lambda p: (p.title("T"), p.highlight("发给了 **空闲的** <队列>")),
        )
        md = build_marp_markdown(lecture.build())
        self.assertIn("<strong>空闲的</strong>", md)
        self.assertIn("&lt;队列&gt;", md)

    def test_highlight_footnote_marker_sits_inside_the_line(self):
        lecture = Lecture(id="lec01", title="L")
        lecture.page(
            "p1",
            body=lambda p: (
                p.title("T"), p.highlight("Underloaded").footnote("λ < μ")
            ),
        )
        md = build_marp_markdown(lecture.build())
        # the marker is tucked before </p> so it claims no extra Marp paragraph
        self.assertIn(
            '<span>Underloaded</span> <sup class="footnote-ref">1</sup></p>', md
        )
        self.assertNotIn("</p> <sup", md)

    def test_highlight_claims_a_reveal_step(self):
        from lecturekit.renderers.viewer import _REVEAL_SKIP

        self.assertNotIn("highlight", _REVEAL_SKIP)

    def test_viewer_js_maps_pages_one_to_one(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.render_to_tmp(tmp)
            js = Path(tmp, "viewer.js").read_text(encoding="utf-8")
        # no leading outline slide: page index i -> deck slide i+1
        self.assertIn('"slides.html#" + (idx + 1)', js)
        self.assertIn("pageOrder[slide - 1]", js)

    def test_viewer_outline_shows_one_based_page_numbers(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.render_to_tmp(tmp)
            js = Path(tmp, "viewer.js").read_text(encoding="utf-8")
            css = Path(tmp, "outline.css").read_text(encoding="utf-8")

        # Two numberings: the deck position addresses a slide, the shown number
        # labels the outline row (they diverge once a deck holds an animation).
        self.assertIn("slideIndexById[p.id] = i + 1", js)
        self.assertIn("shownNumberById[p.id] = p.number || i + 1", js)
        self.assertIn('pageNumber.className = "page-number"', js)
        self.assertIn("shownNumberById[node.id]", js)
        self.assertIn(".page-number", css)

    def test_viewer_js_hosts_marp_iframe_not_handrolled_markdown(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.render_to_tmp(tmp)
            js = Path(tmp, "viewer.js").read_text(encoding="utf-8")
        self.assertIn('"slides.html#"', js)
        self.assertIn("slide-frame", js)
        self.assertIn("slide-back", js)
        self.assertNotIn("renderMarkdown", js)
        self.assertNotIn("renderBlock", js)

    def test_viewer_js_persists_reader_position_across_reloads(self):
        # A live-reload reloads the shell; the viewer must restore the reader's
        # page instead of dropping back to the outline.
        with tempfile.TemporaryDirectory() as tmp:
            self.render_to_tmp(tmp)
            js = Path(tmp, "viewer.js").read_text(encoding="utf-8")
        self.assertIn("sessionStorage", js)
        self.assertIn("restoreState", js)
        self.assertIn("beforeunload", js)

    def test_slides_md_defaults_to_16_9_size_directive(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.render_to_tmp(tmp)
            md = Path(tmp, "slides.md").read_text(encoding="utf-8")
        self.assertIn("size: 16:9", md)

    def test_slides_md_honors_lecture_ratio(self):
        lecture = Lecture(id="lec", title="L", ratio="4:3")

        def body(p):
            p.title("Page")
            p.slide("hello")

        with lecture.section("S", id="s") as section:
            section.page("p1", body=body)

        with tempfile.TemporaryDirectory() as tmp:
            StaticViewerRenderer().render(lecture.build(), Path(tmp))
            md = Path(tmp, "slides.md").read_text(encoding="utf-8")

        self.assertIn("size: 4:3", md)

    def test_copies_assets_when_present(self):
        with tempfile.TemporaryDirectory() as src, tempfile.TemporaryDirectory() as tmp:
            assets = Path(src, "assets")
            assets.mkdir()
            Path(assets, "x.svg").write_text("<svg/>", encoding="utf-8")
            StaticViewerRenderer(asset_root=Path(src)).render(
                sample_lecture().build(), Path(tmp)
            )
            self.assertTrue(Path(tmp, "assets", "x.svg").exists())

    def test_slides_md_includes_sidenote_html(self):
        lecture = Lecture(id="lec01", title="Test Lecture")

        def body(p):
            p.title("Page")
            p.sidenote("一个 LLM Request", "正文", link="https://api.deepseek.com")

        lecture.page("page", body=body)

        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            StaticViewerRenderer().render(lecture.build(), out)
            md = (out / "slides.md").read_text(encoding="utf-8")

        self.assertIn('<aside class="sidenote sidenote--single-line">', md)
        self.assertIn(
            '<a class="sidenote-title" href="https://api.deepseek.com"'
            ' target="_blank" rel="noopener noreferrer">'
            '一个 LLM Request：</a>',
            md,
        )

    def test_multiple_sidenotes_on_a_page_cycle_colors(self):
        lecture = Lecture(id="lec01", title="Test Lecture")

        def body(p):
            p.title("Page")
            p.sidenote("First", "a")
            p.sidenote("Second", "b")

        lecture.page("page", body=body)

        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            StaticViewerRenderer().render(lecture.build(), out)
            md = (out / "slides.md").read_text(encoding="utf-8")

        # first note keeps the default color, second steps to the next slot
        self.assertEqual(md.count("var(--sidenote-2)"), 1)
        self.assertNotIn("var(--sidenote-1)", md)

    def test_theme_styles_the_sidenote_box(self):
        css = Path("themes/basic-office.css").read_text(encoding="utf-8")
        self.assertIn(".sidenote {", css)
        self.assertIn("#f7d6d4", css)   # box background
        self.assertIn("#0d2332", css)   # box border
        self.assertIn(".sidenote-title", css)
        self.assertIn(".sidenote--single-line .sidenote-logo", css)
        self.assertIn("max-height: 1lh", css)

    def test_render_writes_outline_print_html(self):
        lecture = Lecture(id="lec", title="Deck Title", subtitle="A subtitle")

        def one(p):
            p.title("First Page")
            p.slide("hello")

        with lecture.section("Part I", id="s1") as section:
            section.page("p1", body=one)

        with tempfile.TemporaryDirectory() as tmp:
            StaticViewerRenderer().render(lecture.build(), Path(tmp))
            html = Path(tmp, "outline.html").read_text(encoding="utf-8")

        # deck-width page that grows to fit, with the embedded CJK font
        self.assertIn("1280px", html)                       # 16:9 default width
        self.assertIn("@font-face", html)
        self.assertIn("NotoSansSC-Regular.woff2", html)
        self.assertIn("@page", html)                        # fit-to-one-page rule
        self.assertIn("getBoundingClientRect", html)        # fractional height, measured at runtime
        # the outline tree, fully expanded, no current-page/collapsed markers
        self.assertIn('<div class="outline">', html)
        self.assertIn("Deck Title", html)
        self.assertIn("Part I", html)
        self.assertIn("A subtitle", html)
        self.assertNotIn("▶", html)                    # never collapsed
        self.assertNotIn('class="label selected"', html)
        # the outline font is pinned to a fixed px for print (outline.css's
        # vw-based clamp would resolve against a different viewport at measure
        # time and overflow onto a 2nd page); the override must follow the clamp
        # to win the cascade
        self.assertIn(".outline { font-size: 18px; }", html)
        self.assertGreater(
            html.index(".outline { font-size: 18px; }"), html.index("clamp(")
        )

    def test_outline_page_rows_link_to_slides(self):
        lecture = Lecture(id="lec", title="L")

        # Distinct titles: two sibling pages sharing one would be one outline
        # row (see FoldedTitleRunTest), and this test is about the links.
        def body(title):
            def fill(p):
                p.title(title)
                p.slide("hello")
            return fill

        with lecture.section("Part I", id="s1") as section:
            section.page("p1", body=body("Page 1"))
            section.page("p2", body=body("Page 2"))
        lecture.page("p3", body=body("Page 3"))

        with tempfile.TemporaryDirectory() as tmp:
            StaticViewerRenderer().render(lecture.build(), Path(tmp))
            html = Path(tmp, "outline.html").read_text(encoding="utf-8")

        # one slide anchor per page, indexed in deck order (sections are not links)
        self.assertEqual(html.count("https://lecturekit.invalid/slide/"), 3)
        self.assertIn('href="https://lecturekit.invalid/slide/0"', html)
        self.assertIn('href="https://lecturekit.invalid/slide/1"', html)
        self.assertIn('href="https://lecturekit.invalid/slide/2"', html)
        self.assertNotIn('href="https://lecturekit.invalid/slide/3"', html)

        # Page numbers are slide ordinals, not merged-PDF indices: the outline
        # cover itself does not shift them by one.
        self.assertEqual(html.count('class="page-number"'), 3)
        self.assertIn('<span class="page-number">1</span>', html)
        self.assertIn('<span class="page-number">2</span>', html)
        self.assertIn('<span class="page-number">3</span>', html)
        self.assertLess(
            html.index('<span class="page-number">1</span>'),
            html.index('<a class="node-link" href="https://lecturekit.invalid/slide/0"'),
        )


def _reveal_page():
    lecture = Lecture(id="lec", title="T")

    def body(p):
        p.title("Title")
        p.slide("first block").annotate("look", at="center")
        p.code("python", "print(1)")
        p.side_image("assets/x.svg")  # bg directive, must NOT be wrapped
    lecture.page("pg", body=body)
    return lecture.build().children[0]


def test_render_marp_page_no_reveal_is_unwrapped():
    md = render_marp_page(_reveal_page())
    assert "reveal-block" not in md
    assert "data-reveal" not in md


def test_render_marp_page_wraps_blocks_when_reveal():
    md = render_marp_page(_reveal_page(), reveal=True)
    # title stays a bare heading, before any wrapper
    assert md.index("# Title") < md.index('data-reveal="0"')
    # slide is step 0, code is step 1 (contiguous; side_image skipped)
    assert '<div class="reveal-block" data-reveal="0">' in md
    assert '<div class="reveal-block" data-reveal="1">' in md
    assert 'data-reveal="2"' not in md
    # the slide's annotation bubble reveals with its block (step 0)
    assert 'data-reveal="0" class="annotation' in md
    # side_image emitted but never wrapped
    assert "![bg right" in md or "![bg" in md
    assert md.count('class="reveal-block"') == 2


def test_reveal_wrapper_pads_with_blank_lines():
    md = render_marp_page(_reveal_page(), reveal=True)
    block = md.split('<div class="reveal-block" data-reveal="0">', 1)[1]
    # blank line right after the opening div (so Marp parses inner markdown)
    assert block.startswith("\n\n**first block**")
    assert "\n\n</div>" in md


def _items_page(gap: bool = False):
    lecture = Lecture(id="lec", title="T")

    def body(p):
        p.title("Recap")
        if gap:
            p.gap("auto")
        p.slide("what we covered\n- one\n- two", reveal="items")
        p.slide("plain block")
    lecture.page("pg", body=body)
    return lecture.build().children[0]


def test_reveal_items_flags_only_the_block_that_asked():
    md = render_marp_page(_items_page(), reveal=True)
    assert '<div class="reveal-block" data-reveal="0" data-reveal-items="1">' in md
    assert '<div class="reveal-block" data-reveal="1">' in md
    assert md.count("data-reveal-items") == 1


def test_reveal_items_flags_the_gap_block_wrapper_too():
    md = render_marp_page(_items_page(gap=True), reveal=True)
    assert (
        '<div class="lk-gap-block reveal-block" data-reveal="0" data-reveal-items="1">'
        in md
    )
    assert '<div class="lk-gap-block reveal-block" data-reveal="1">' in md


def test_reveal_items_flag_is_live_preview_only():
    md = render_marp_page(_items_page())
    assert "data-reveal-items" not in md
