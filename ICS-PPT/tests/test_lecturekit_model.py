import unittest

from lecturekit import Lecture, Page
from lecturekit.model import (
    BLOCK_KINDS,
    Block,
    NewsItem,
    Page as ModelPage,
    ValidationError,
    select_blocks,
)


class SelectBlocksTest(unittest.TestCase):
    def _page(self, *blocks):
        return ModelPage(id="p", title="T", blocks=list(blocks))

    def test_block_kinds_covers_the_full_authoring_vocabulary(self):
        self.assertEqual(
            BLOCK_KINDS,
            frozenset({
                "cover", "slide", "notes", "prose", "code", "link",
                "image", "side_image", "sidenote", "demo", "aside",
                "table", "architecture", "row", "spacer", "highlight",
                "bridge",
            }),
        )

    def test_select_keeps_only_kinds_the_renderer_handles(self):
        page = self._page(
            Block(kind="slide", content="s"),
            Block(kind="notes", content="n"),
        )
        kinds = {"slide"}
        self.assertEqual(
            [b.kind for b in select_blocks(page, "viewer", kinds)], ["slide"]
        )

    def test_select_honors_only(self):
        page = self._page(Block(kind="slide", content="s", only={"pptx"}))
        self.assertEqual(select_blocks(page, "viewer", {"slide"}), [])

    def test_select_honors_except(self):
        page = self._page(Block(kind="slide", content="s", except_={"viewer"}))
        self.assertEqual(select_blocks(page, "viewer", {"slide"}), [])


class LectureKitModelTest(unittest.TestCase):
    def test_top_level_page_type_names_authoring_page_builder(self):
        self.assertEqual(Page.__name__, "PageBuilder")
        self.assertTrue(hasattr(Page(), "slide"))
        self.assertTrue(hasattr(Page(), "image"))

    def test_declares_nested_sections_pages_and_visibility(self):
        lecture = Lecture(id="lec01", title="Test Lecture", subtitle="Sub")

        def body(p):
            p.title("Welcome")
            p.slide("visible")
            p.notes("teacher")
            p.aside("terminal only", only=["terminal"])

        with lecture.section("Intro", id="intro") as section:
            section.page("welcome", body=body)

        model = lecture.build()
        page = model.children[0].children[0]

        self.assertEqual(model.children[0].id, "intro")
        self.assertEqual(page.title, "Welcome")
        self.assertEqual(page.blocks[0].kind, "slide")
        self.assertEqual(
            [b.content for b in select_blocks(page, "viewer", {"slide"})], ["**visible**"]
        )

    def test_viewer_target_shows_classroom_blocks_but_not_notes(self):
        lecture = Lecture(id="lec01", title="Test Lecture")

        def body(p):
            p.title("Welcome")
            p.slide("slide text")
            p.code("python", "print('hi')")
            p.link("OSTEP", "https://example.com")
            p.image("assets/x.svg", alt="x")
            p.aside("aside text")
            p.notes("teacher only")
            p.demo("run", "echo hi")

        lecture.page("welcome", body=body)
        page = lecture.build().children[0]

        viewer_kinds = {"slide", "code", "link", "image", "side_image", "sidenote", "aside"}
        kinds = [b.kind for b in select_blocks(page, "viewer", viewer_kinds)]
        self.assertEqual(kinds, ["slide", "code", "link", "image", "aside"])

    def test_image_block_records_size_via_kwargs(self):
        lecture = Lecture(id="lec01", title="Test Lecture")

        def body(p):
            p.title("Welcome")
            p.image("assets/x.svg", alt="a", width_pct=60, height_px=200)

        lecture.page("welcome", body=body)
        block = lecture.build().children[0].blocks[0]

        self.assertEqual(block.content["width"], "60%")
        self.assertEqual(block.content["height"], "200px")

    def test_image_block_records_size_via_setters(self):
        lecture = Lecture(id="lec01", title="Test Lecture")

        def body(p):
            p.title("Welcome")
            p.image("assets/x.svg").width_px(480).height_pct(50)

        lecture.page("welcome", body=body)
        block = lecture.build().children[0].blocks[0]

        self.assertEqual(block.content["width"], "480px")
        self.assertEqual(block.content["height"], "50%")

    def test_image_size_setters_chain_with_footnote(self):
        lecture = Lecture(id="lec01", title="Test Lecture")

        def body(p):
            p.title("Welcome")
            p.image("assets/x.svg").width_px(480).footnote("src")

        lecture.page("welcome", body=body)
        block = lecture.build().children[0].blocks[0]

        self.assertEqual(block.content["width"], "480px")
        self.assertEqual(block.footnotes, ("src",))

    def test_image_rejects_both_units_for_one_dimension(self):
        lecture = Lecture(id="lec01", title="Test Lecture")

        def body(p):
            p.title("Welcome")
            p.image("assets/x.svg", width_px=480, width_pct=60)

        lecture.page("welcome", body=body)
        with self.assertRaises(ValidationError):
            lecture.build()

    def test_image_defaults_unframed_and_centered(self):
        lecture = Lecture(id="lec01", title="Test Lecture")
        lecture.page("welcome", body=lambda p: (p.title("W"), p.image("assets/x.svg")))
        block = lecture.build().children[0].blocks[0]

        self.assertIs(block.content["framed"], False)
        self.assertEqual(block.content["caption_align"], "center")

    def test_image_records_framing_and_alignment_via_kwargs(self):
        lecture = Lecture(id="lec01", title="Test Lecture")

        def body(p):
            p.title("W")
            p.image("assets/x.svg", framed=True, caption="c", caption_align="left")

        lecture.page("welcome", body=body)
        block = lecture.build().children[0].blocks[0]

        self.assertIs(block.content["framed"], True)
        self.assertEqual(block.content["caption_align"], "left")

    def test_image_records_framing_and_alignment_via_setters(self):
        lecture = Lecture(id="lec01", title="Test Lecture")

        def body(p):
            p.title("W")
            p.image("assets/x.svg", caption="c").framed().caption_align("right")

        lecture.page("welcome", body=body)
        block = lecture.build().children[0].blocks[0]

        self.assertIs(block.content["framed"], True)
        self.assertEqual(block.content["caption_align"], "right")

    def test_image_rejects_invalid_caption_align(self):
        lecture = Lecture(id="lec01", title="Test Lecture")
        lecture.page(
            "welcome",
            body=lambda p: (p.title("W"), p.image("assets/x.svg", caption_align="middle")),
        )
        with self.assertRaises(ValidationError):
            lecture.build()

    def test_page_body_error_names_page_and_source_line(self):
        lecture = Lecture(id="lec01", title="Test Lecture")

        def broken(p):
            p.title("W")
            p.image()  # missing required 'src'

        lecture.page("welcome", body=broken)
        with self.assertRaises(ValidationError) as ctx:
            lecture.build()
        message = str(ctx.exception)
        # names the page, the author function, the source file/line, and keeps
        # the original error text so the mistake is locatable at a glance.
        self.assertIn("welcome", message)
        self.assertIn("in broken()", message)
        self.assertIn("test_lecturekit_model.py", message)
        self.assertIn("src", message)
        # the underlying exception is chained for a full traceback when wanted.
        self.assertIsInstance(ctx.exception.__cause__, TypeError)

    def test_validation_rejects_duplicate_ids(self):
        lecture = Lecture(id="lec01", title="Test Lecture")
        lecture.page("same", body=lambda p: (p.title("One"), p.slide("one")))
        lecture.page("same", body=lambda p: (p.title("Two"), p.slide("two")))

        with self.assertRaises(ValidationError):
            lecture.build()

    def test_body_without_title_is_rejected(self):
        lecture = Lecture(id="lec01", title="Test Lecture")
        lecture.page("welcome", body=lambda p: p.slide("no title here"))

        with self.assertRaises(ValidationError):
            lecture.build()

    def test_body_with_two_titles_is_rejected(self):
        lecture = Lecture(id="lec01", title="Test Lecture")

        def body(p):
            p.title("First")
            p.title("Second")
            p.slide("body")

        lecture.page("welcome", body=body)

        with self.assertRaises(ValidationError):
            lecture.build()

    def test_page_gap_auto_records_metadata(self):
        lecture = Lecture(id="lec01", title="Test Lecture")

        def body(p):
            p.title("Welcome")
            p.gap("auto", min_px=6, max_px=30)
            p.slide("one")

        lecture.page("welcome", body=body)
        page = lecture.build().children[0]

        self.assertEqual(page.gap.mode, "auto")
        self.assertEqual(page.gap.min_px, 6)
        self.assertEqual(page.gap.max_px, 30)

    def test_page_gap_bare_call_takes_the_default_cap(self):
        page = self._gap_page(lambda p: p.gap())

        self.assertEqual(page.gap.mode, "auto")
        self.assertEqual(page.gap.min_px, 8)
        self.assertEqual(page.gap.max_px, 52)

    def test_page_gap_positional_sets_the_cap(self):
        page = self._gap_page(lambda p: p.gap(24))

        self.assertEqual(page.gap.min_px, 8)
        self.assertEqual(page.gap.max_px, 24)

    def test_page_gap_fill_lifts_the_cap(self):
        page = self._gap_page(lambda p: p.gap("fill"))

        self.assertEqual(page.gap.min_px, 8)
        self.assertIsNone(page.gap.max_px)

    def test_page_gap_positional_cap_pulls_the_floor_down_with_it(self):
        page = self._gap_page(lambda p: p.gap(4))

        self.assertEqual(page.gap.min_px, 4)
        self.assertEqual(page.gap.max_px, 4)

    def test_page_gap_explicit_floor_survives_a_small_cap(self):
        page = self._gap_page(lambda p: p.gap(24, min_px=12))

        self.assertEqual(page.gap.min_px, 12)
        self.assertEqual(page.gap.max_px, 24)

    def test_page_gap_legacy_spelling_keeps_its_own_default_cap(self):
        page = self._gap_page(lambda p: p.gap("auto"))

        self.assertEqual(page.gap.min_px, 8)
        self.assertEqual(page.gap.max_px, 28)

    def test_page_gap_rejects_a_cap_given_twice(self):
        for call in (lambda p: p.gap(24, max_px=30),
                     lambda p: p.gap("fill", max_px=30)):
            with self.subTest(call=call):
                with self.assertRaises(ValidationError):
                    self._gap_page(call)

    def test_page_gap_rejects_a_non_integer_cap(self):
        with self.assertRaises(ValidationError):
            self._gap_page(lambda p: p.gap(3.5))

    def _gap_page(self, call):
        lecture = Lecture(id="lec01", title="Test Lecture")
        lecture.page(
            "welcome",
            body=lambda p: (p.title("W"), call(p), p.slide("body")),
        )
        return lecture.build().children[0]

    def test_page_gap_defaults_to_none(self):
        lecture = Lecture(id="lec01", title="Test Lecture")
        lecture.page("welcome", body=lambda p: (p.title("W"), p.slide("body")))

        self.assertIsNone(lecture.build().children[0].gap)

    def test_page_gap_rejects_unknown_mode(self):
        lecture = Lecture(id="lec01", title="Test Lecture")
        lecture.page(
            "welcome",
            body=lambda p: (p.title("W"), p.gap("fixed"), p.slide("body")),
        )

        with self.assertRaises(ValidationError):
            lecture.build()

    def test_page_gap_rejects_negative_values(self):
        lecture = Lecture(id="lec01", title="Test Lecture")
        lecture.page(
            "welcome",
            body=lambda p: (
                p.title("W"),
                p.gap("auto", min_px=-1),
                p.slide("body"),
            ),
        )

        with self.assertRaises(ValidationError):
            lecture.build()

    def test_page_gap_rejects_bool_values(self):
        lecture = Lecture(id="lec01", title="Test Lecture")
        lecture.page(
            "welcome",
            body=lambda p: (
                p.title("W"),
                p.gap("auto", min_px=True),
                p.slide("body"),
            ),
        )

        with self.assertRaises(ValidationError):
            lecture.build()

    def test_page_gap_rejects_min_greater_than_max(self):
        lecture = Lecture(id="lec01", title="Test Lecture")
        lecture.page(
            "welcome",
            body=lambda p: (
                p.title("W"),
                p.gap("auto", min_px=40, max_px=20),
                p.slide("body"),
            ),
        )

        with self.assertRaises(ValidationError):
            lecture.build()

    def test_page_gap_rejects_duplicate_calls(self):
        lecture = Lecture(id="lec01", title="Test Lecture")

        def body(p):
            p.title("W")
            p.gap("auto")
            p.gap("auto")
            p.slide("body")

        lecture.page("welcome", body=body)

        with self.assertRaises(ValidationError):
            lecture.build()

    def test_spacer_records_height_and_allows_several_per_page(self):
        lecture = Lecture(id="lec01", title="Test Lecture")

        def body(p):
            p.title("W")
            p.slide("a")
            p.spacer(24)
            p.slide("b")
            p.spacer(8)

        lecture.page("welcome", body=body)
        blocks = lecture.build().children[0].blocks
        spacers = [b for b in blocks if b.kind == "spacer"]
        self.assertEqual([b.content["px"] for b in spacers], [24, 8])

    def test_spacer_rejects_non_positive_height(self):
        for px in (0, -5):
            lecture = Lecture(id="lec01", title="Test Lecture")
            lecture.page(
                "welcome",
                body=lambda p, px=px: (p.title("W"), p.spacer(px)),
            )
            with self.assertRaises(ValidationError):
                lecture.build()

    def test_spacer_rejects_bool_height(self):
        lecture = Lecture(id="lec01", title="Test Lecture")
        lecture.page(
            "welcome",
            body=lambda p: (p.title("W"), p.spacer(True)),
        )
        with self.assertRaises(ValidationError):
            lecture.build()

    def test_highlight_records_text_and_defaults_to_yellow(self):
        lecture = Lecture(id="lec01", title="Test Lecture")

        def body(p):
            p.title("W")
            p.highlight("Underloaded")
            p.highlight("Overloaded", tone="orange")

        lecture.page("welcome", body=body)
        blocks = lecture.build().children[0].blocks
        highlights = [b for b in blocks if b.kind == "highlight"]
        self.assertEqual(
            [(b.content["text"], b.content["tone"]) for b in highlights],
            [("Underloaded", "yellow"), ("Overloaded", "orange")],
        )

    def test_highlight_keeps_text_verbatim_for_multiline(self):
        lecture = Lecture(id="lec01", title="Test Lecture")
        lecture.page(
            "welcome",
            body=lambda p: (p.title("W"), p.highlight("a\nb")),
        )
        blocks = lecture.build().children[0].blocks
        self.assertEqual(blocks[0].content["text"], "a\nb")

    def test_highlight_rejects_empty_text(self):
        for text in ("", "   \n  "):
            lecture = Lecture(id="lec01", title="Test Lecture")
            lecture.page(
                "welcome",
                body=lambda p, text=text: (p.title("W"), p.highlight(text)),
            )
            with self.assertRaises(ValidationError):
                lecture.build()

    def test_highlight_rejects_unknown_tone(self):
        lecture = Lecture(id="lec01", title="Test Lecture")
        lecture.page(
            "welcome",
            body=lambda p: (p.title("W"), p.highlight("x", tone="magenta")),
        )
        with self.assertRaises(ValidationError):
            lecture.build()

    def test_lecture_defaults_to_16_9_ratio(self):
        lecture = Lecture(id="lec01", title="Test Lecture")
        lecture.page("welcome", body=lambda p: (p.title("W"), p.slide("body")))

        self.assertEqual(lecture.build().ratio, "16:9")

    def test_lecture_ratio_can_be_set(self):
        lecture = Lecture(id="lec01", title="Test Lecture", ratio="4:3")
        lecture.page("welcome", body=lambda p: (p.title("W"), p.slide("body")))

        self.assertEqual(lecture.build().ratio, "4:3")

    def test_unknown_lecture_ratio_is_rejected(self):
        lecture = Lecture(id="lec01", title="Test Lecture", ratio="21:9")
        lecture.page("welcome", body=lambda p: (p.title("W"), p.slide("body")))

        with self.assertRaises(ValidationError):
            lecture.build()

    def test_sidenote_block_records_fields(self):
        lecture = Lecture(id="lec01", title="Test Lecture")

        def body(p):
            p.title("Welcome")
            p.sidenote("一个 LLM Request", "在地球上的任何一个地方",
                       link="https://api.deepseek.com", logo="assets/laptop.png")

        lecture.page("welcome", body=body)
        block = lecture.build().children[0].blocks[0]

        self.assertEqual(block.kind, "sidenote")
        self.assertEqual(block.content, {
            "title": "一个 LLM Request",
            "text": "在地球上的任何一个地方",
            "link": "https://api.deepseek.com",
            "logo": "assets/laptop.png",
        })

    def test_sidenote_link_and_logo_default_to_none(self):
        lecture = Lecture(id="lec01", title="Test Lecture")

        def body(p):
            p.title("Welcome")
            p.sidenote("T", "B")

        lecture.page("welcome", body=body)
        block = lecture.build().children[0].blocks[0]

        self.assertIsNone(block.content["link"])
        self.assertIsNone(block.content["logo"])

    def test_sidenote_selected_for_viewer_and_overrides_apply(self):
        lecture = Lecture(id="lec01", title="Test Lecture")

        def body(p):
            p.title("Welcome")
            p.sidenote("T", "B")

        lecture.page("welcome", body=body)
        page = lecture.build().children[0]

        self.assertEqual(
            [b.kind for b in select_blocks(page, "viewer", {"sidenote"})], ["sidenote"]
        )
        # a kind the renderer does not handle is skipped
        self.assertEqual(select_blocks(page, "viewer", set()), [])

    def _table_block(self, **kwargs):
        lecture = Lecture(id="lec01", title="Test Lecture")

        def body(p):
            p.title("Welcome")
            p.table(**kwargs)

        lecture.page("welcome", body=body)
        return lecture.build().children[0].blocks[0]

    def test_table_block_records_headers_rows_and_align(self):
        block = self._table_block(
            headers=["机制", "开销"],
            rows=[["进程", "高"], ["线程", "低"]],
            align=["left", "right"],
        )
        self.assertEqual(block.kind, "table")
        self.assertEqual(block.content, {
            "headers": ["机制", "开销"],
            "rows": [["进程", "高"], ["线程", "低"]],
            "align": ["left", "right"],
        })

    def test_table_align_defaults_to_none(self):
        block = self._table_block(headers=["a", "b"], rows=[["1", "2"]])
        self.assertIsNone(block.content["align"])

    def test_table_rejects_ragged_row(self):
        with self.assertRaises(ValidationError):
            self._table_block(headers=["a", "b"], rows=[["1"]])

    def test_table_rejects_align_of_wrong_length(self):
        with self.assertRaises(ValidationError):
            self._table_block(headers=["a", "b"], rows=[["1", "2"]], align=["left"])

    def test_table_rejects_unknown_align_value(self):
        with self.assertRaises(ValidationError):
            self._table_block(
                headers=["a", "b"], rows=[["1", "2"]], align=["left", "middle"]
            )


class NewsTest(unittest.TestCase):
    def _page_with_news(self):
        lecture = Lecture(id="lec01", title="Test Lecture")

        def body(p):
            p.title("Welcome")
            p.slide("body")
            p.news(
                "Scaling Laws for Neural Language Models",
                url="https://arxiv.org/abs/2001.08361",
                source="Kaplan et al.",
                date="2020",
                kind="paper",
                why="Read the summary and Figure 1.",
                tags=["paper", "scaling-law"],
                image="assets/scaling-law-loss-curves.png",
                archived_url="https://web.archive.org/example",
            )
            p.news("Systems article", url="https://example.com/sys", tags=["systems"])

        lecture.page("welcome", body=body)
        return lecture.build().children[0]

    def test_news_items_are_page_metadata_in_author_order(self):
        page = self._page_with_news()

        self.assertEqual(page.blocks[0].kind, "slide")
        self.assertEqual(
            page.news,
            (
                NewsItem(
                    title="Scaling Laws for Neural Language Models",
                    url="https://arxiv.org/abs/2001.08361",
                    source="Kaplan et al.",
                    date="2020",
                    kind="paper",
                    why="Read the summary and Figure 1.",
                    tags=("paper", "scaling-law"),
                    image="assets/scaling-law-loss-curves.png",
                    archived_url="https://web.archive.org/example",
                ),
                NewsItem(
                    title="Systems article",
                    url="https://example.com/sys",
                    tags=("systems",),
                ),
            ),
        )

    def test_news_defaults_to_empty_tuple(self):
        lecture = Lecture(id="lec01", title="Test Lecture")
        lecture.page("welcome", body=lambda p: (p.title("Welcome"), p.slide("body")))

        self.assertEqual(lecture.build().children[0].news, ())

    def test_news_rejects_invalid_kind(self):
        lecture = Lecture(id="lec01", title="Test Lecture")

        def body(p):
            p.title("Welcome")
            p.slide("body")
            p.news("Title", url="https://example.com", kind="tweet")

        lecture.page("welcome", body=body)
        with self.assertRaises(ValidationError):
            lecture.build()

    def test_news_rejects_empty_required_and_tag_fields(self):
        bad_bodies = [
            lambda p: p.news("", url="https://example.com"),
            lambda p: p.news("Title", url=" "),
            lambda p: p.news("Title", url="https://example.com", tags=["ok", " "]),
        ]
        for bad_body in bad_bodies:
            with self.subTest(bad_body=bad_body):
                lecture = Lecture(id="lec01", title="Test Lecture")

                def body(p, bad_body=bad_body):
                    p.title("Welcome")
                    p.slide("body")
                    bad_body(p)

                lecture.page("welcome", body=body)
                with self.assertRaises(ValidationError):
                    lecture.build()

    def test_news_rejects_empty_optional_string_fields(self):
        optional_fields = [
            {"source": ""},
            {"date": " "},
            {"why": ""},
            {"image": " "},
            {"archived_url": ""},
        ]
        for kwargs in optional_fields:
            with self.subTest(kwargs=kwargs):
                lecture = Lecture(id="lec01", title="Test Lecture")

                def body(p, kwargs=kwargs):
                    p.title("Welcome")
                    p.slide("body")
                    p.news("Title", url="https://example.com", **kwargs)

                lecture.page("welcome", body=body)
                with self.assertRaises(ValidationError):
                    lecture.build()


class CloseTest(unittest.TestCase):
    def _closing(self, p):
        p.title("That's a wrap")
        p.slide("Processes give isolation; threads give sharing.")

    def test_close_builds_a_single_top_level_page(self):
        lecture = Lecture(id="lec01", title="Test Lecture")
        lecture.page("intro", body=lambda p: (p.title("I"), p.slide("hi")))
        lecture.close("conclusion", body=self._closing)

        children = lecture.build().children
        # appended in call order: the conclusion lands last
        page = children[-1]
        self.assertIsInstance(page, ModelPage)
        self.assertEqual(page.id, "conclusion")
        self.assertEqual(page.title, "That's a wrap")
        self.assertEqual(page.blocks[0].kind, "slide")

    def test_close_title_comes_from_the_body(self):
        lecture = Lecture(id="lec01", title="Test Lecture")
        lecture.close("conclusion", body=self._closing)
        self.assertEqual(lecture.build().children[-1].title, "That's a wrap")

    def test_close_passes_tags_and_annotation_flag_to_the_page(self):
        lecture = Lecture(id="lec01", title="Test Lecture")
        lecture.close(
            "conclusion",
            body=lambda p: (p.title("C"), p.image("x.svg").annotate("n")),
            tags=["recap"],
            annotation=False,
        )
        page = lecture.build().children[-1]
        self.assertEqual(page.tags, {"recap"})
        self.assertFalse(page.show_annotations)

    def test_close_page_id_collision_is_rejected(self):
        lecture = Lecture(id="lec01", title="Test Lecture")
        lecture.page("conclusion", body=lambda p: (p.title("X"), p.slide("x")))
        lecture.close("conclusion", body=self._closing)
        with self.assertRaises(ValidationError):
            lecture.build()


class CoverTest(unittest.TestCase):
    def test_cover_builds_a_top_level_page_with_optional_metadata(self):
        lecture = Lecture(id="lec01", title="Test Lecture")
        lecture.cover(
            "Elastic model serving via efficient autoscaling",
            author="Xingda Wei",
            time="July 2026",
            logo={"left": "assets/ipads.svg", "right": "assets/sjtu.svg"},
        )

        page = lecture.build().children[0]

        self.assertIsInstance(page, ModelPage)
        self.assertEqual(page.id, "cover")
        self.assertEqual(page.title, "Elastic model serving via efficient autoscaling")
        self.assertEqual(page.blocks[0].kind, "cover")
        self.assertEqual(
            page.blocks[0].content,
            {
                "author": "Xingda Wei",
                "time": "July 2026",
                "logo": {"left": "assets/ipads.svg", "right": "assets/sjtu.svg"},
            },
        )

    def test_cover_allows_title_only_and_custom_id(self):
        lecture = Lecture(id="lec01", title="Test Lecture")
        lecture.cover("Opening", id="front")

        page = lecture.build().children[0]

        self.assertEqual(page.id, "front")
        self.assertEqual(page.title, "Opening")
        self.assertEqual(
            page.blocks[0].content,
            {"author": None, "time": None, "logo": None},
        )


class AnnotateTest(unittest.TestCase):
    def _block(self, annotate):
        """Build one block, applying ``annotate`` to its BlockHandle."""
        lecture = Lecture(id="lec01", title="Test Lecture")

        def body(p):
            p.title("W")
            annotate(p.image("assets/x.svg"))

        lecture.page("w", body=body)
        return lecture.build().children[0].blocks[0]

    def test_annotate_records_text_and_position(self):
        block = self._block(lambda h: h.annotate("看这里", at="top-right", dx=-10))
        self.assertEqual(len(block.annotations), 1)
        note = block.annotations[0]
        self.assertEqual(note.text, "看这里")
        self.assertEqual(note.at, "top-right")
        self.assertEqual(note.dx, -10)
        self.assertEqual(note.dy, 0)

    def test_annotate_defaults_and_accumulates_and_chains_with_footnote(self):
        block = self._block(
            lambda h: h.annotate("A").annotate("B", at="bottom").footnote("src")
        )
        self.assertEqual([n.text for n in block.annotations], ["A", "B"])
        self.assertEqual(block.annotations[0].at, "top-right")  # default anchor
        self.assertEqual(block.annotations[1].at, "bottom")
        self.assertEqual(block.footnotes, ("src",))

    def test_block_defaults_to_no_annotations(self):
        block = self._block(lambda h: h)
        self.assertEqual(block.annotations, ())

    def test_annotate_rejects_empty_text(self):
        with self.assertRaises(ValidationError):
            self._block(lambda h: h.annotate("   "))

    def test_annotate_rejects_unknown_anchor(self):
        with self.assertRaises(ValidationError):
            self._block(lambda h: h.annotate("x", at="middle"))

    def test_page_annotation_flag_controls_show_annotations(self):
        lecture = Lecture(id="lec01", title="Test Lecture")
        body = lambda p: (p.title("W"), p.image("assets/x.svg").annotate("n"))
        lecture.page("on", body=body)
        lecture.page("off", body=body, annotation=False)
        pages = lecture.build().children
        self.assertTrue(pages[0].show_annotations)
        self.assertFalse(pages[1].show_annotations)


class AnnotateRenderTest(unittest.TestCase):
    def _markdown(self, annotation=True):
        from lecturekit.renderers.viewer import build_marp_markdown

        lecture = Lecture(id="lec01", title="Test Lecture")
        lecture.page(
            "w",
            body=lambda p: (
                p.title("W"),
                p.image("assets/x.svg").annotate("看这里", at="top-right", dx=-10),
            ),
            annotation=annotation,
        )
        return build_marp_markdown(lecture.build())

    def test_bubble_renders_with_class_text_and_position(self):
        md = self._markdown(annotation=True)
        # The bubble carries a deck-unique class; its position lives in a
        # generated <style> rule (Marp strips inline-style positioning).
        self.assertIn('class="annotation ann-w-0"', md)
        self.assertIn("看这里", md)
        self.assertIn("<style>", md)
        # top-right on a 1280x720 (16:9) slide: left = 1280 + dx(-10) = 1270px,
        # top = 0px; the bubble translates back onto the slide by its own size.
        self.assertIn("section .ann-w-0 {", md)
        self.assertIn("left:1270px; top:0px", md)
        self.assertIn("transform: translate(-100%, 0%)", md)

    def test_page_with_annotation_off_suppresses_bubble(self):
        md = self._markdown(annotation=False)
        self.assertNotIn('class="annotation', md)
        self.assertNotIn("看这里", md)
        self.assertNotIn("ann-w-0", md)

    def test_bubble_inline_markdown_is_rendered_and_escaped(self):
        from lecturekit.renderers.viewer.blocks import annotation_markup
        from lecturekit.model import Annotation

        aside, rule = annotation_markup(
            Annotation(text="**bold** <script>"), "p-0", 1280, 720
        )
        self.assertIn("<strong>bold</strong>", aside)
        self.assertIn("&lt;script&gt;", aside)
        self.assertIn("section .ann-p-0 {", rule)


class AnnotateSerializeTest(unittest.TestCase):
    def test_serialize_carries_annotations_and_show_flag(self):
        from lecturekit.serialize import lecture_to_dict

        lecture = Lecture(id="lec01", title="Test Lecture")
        lecture.page(
            "w",
            body=lambda p: (
                p.title("W"),
                p.image("x.svg").annotate("看这里", at="bottom", dx=5),
            ),
            annotation=False,
        )
        page = lecture_to_dict(lecture.build())["children"][0]
        self.assertFalse(page["show_annotations"])
        self.assertEqual(
            page["blocks"][0]["annotations"],
            [{"text": "看这里", "at": "bottom", "dx": 5, "dy": 0}],
        )


class RevealItemsTest(unittest.TestCase):
    """`p.slide(..., reveal="items")` — the live preview's finer reveal step."""

    def _block(self, **kwargs):
        lecture = Lecture(id="lec", title="L")
        lecture.page(
            "p",
            body=lambda p: (p.title("T"), p.slide("- one\n- two", **kwargs)),
        )
        return lecture.build().children[0].blocks[0]

    def test_a_slide_reveals_whole_by_default(self):
        self.assertEqual(self._block().reveal, "block")

    def test_items_mode_is_recorded_on_the_block(self):
        self.assertEqual(self._block(reveal="items").reveal, "items")

    def test_unknown_mode_is_refused(self):
        with self.assertRaises(ValidationError):
            self._block(reveal="lines")

    def test_items_without_a_list_is_refused(self):
        lecture = Lecture(id="lec", title="L")
        lecture.page(
            "p",
            body=lambda p: (p.title("T"), p.slide("no bullets here", reveal="items")),
        )
        with self.assertRaises(ValidationError):
            lecture.build()

    def test_serialize_carries_the_mode(self):
        from lecturekit.serialize import lecture_to_dict

        lecture = Lecture(id="lec01", title="L")
        lecture.page(
            "p",
            body=lambda p: (p.title("T"), p.slide("- one", reveal="items")),
        )
        page = lecture_to_dict(lecture.build())["children"][0]
        self.assertEqual(page["blocks"][0]["reveal"], "items")
