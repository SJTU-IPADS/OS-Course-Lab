"""Translation overlays: key derivation, the TOML file, apply/check/extract."""

from __future__ import annotations

import contextlib
import io
import json
import shutil
import tempfile
import tomllib
import unittest
from pathlib import Path

from lecturekit import i18n, model
from lecturekit.dsl import Lecture as LectureBuilder


def build(body, *, page_id="intro", **kwargs) -> model.Lecture:
    """A one-page lecture whose page body is ``body``."""
    lecture = LectureBuilder(id="lec", title="标题", **kwargs)
    lecture.page(page_id, body=body)
    return lecture.build()


def keys(lecture: model.Lecture) -> list[str]:
    return [entry.key for entry in i18n.collect(lecture)]


class KeyDerivationTest(unittest.TestCase):
    def test_lecture_section_and_page_titles(self):
        lecture = LectureBuilder(id="lec", title="标题", subtitle="副标题")
        with lecture.section("小节", id="sec") as s:
            s.page("intro", body=lambda p: (p.title("页"), p.slide("正文")))
        built = lecture.build()
        self.assertEqual(
            keys(built),
            [
                "lecture.title",
                "lecture.subtitle",
                "section.sec.title",
                "intro.title",
                "intro.slide.1",
            ],
        )

    def test_blocks_are_numbered_per_kind_in_author_order(self):
        def body(p):
            p.title("T")
            p.slide("one")
            p.aside("a")
            p.slide("two")

        self.assertEqual(
            keys(build(body)),
            ["lecture.title", "intro.title", "intro.slide.1", "intro.aside.1",
             "intro.slide.2"],
        )

    def test_a_pinned_key_replaces_the_number_without_consuming_one(self):
        # The point of pinning: `slide.2` still names the third slide's
        # neighbour, so filling in a translation is not undone by a later pin.
        def body(p):
            p.title("T")
            p.slide("one")
            p.slide("two", key="why")
            p.slide("three")

        self.assertEqual(
            keys(build(body))[2:],
            ["intro.slide.1", "intro.why", "intro.slide.2"],
        )

    def test_sub_strings_hang_off_the_block_key(self):
        def body(p):
            p.title("T")
            p.slide("body").footnote("来源").annotate("看这里")
            p.image("a.svg", caption="图一", alt="替代")

        self.assertEqual(
            keys(build(body))[2:],
            [
                "intro.slide.1",
                "intro.slide.1.footnote.1",
                "intro.slide.1.annotation.1",
                "intro.image.1.caption",
                "intro.image.1.alt",
            ],
        )

    def test_a_pinned_block_keeps_its_pin_in_sub_keys(self):
        def body(p):
            p.title("T")
            p.slide("body", key="why").footnote("来源")

        self.assertEqual(keys(build(body))[2:], ["intro.why", "intro.why.footnote.1"])

    def test_table_cells_count_the_header_as_row_zero(self):
        def body(p):
            p.title("T")
            p.table([["a", "b"]], headers=["h1", "h2"])

        self.assertEqual(
            keys(build(body))[2:],
            ["intro.table.1.cell.0.0", "intro.table.1.cell.0.1",
             "intro.table.1.cell.1.0", "intro.table.1.cell.1.1"],
        )

    def test_architecture_layers_and_modules(self):
        def body(p):
            p.title("T")
            arch = p.architecture(caption="图")
            arch.layer("App", ["Shell", ...])

        self.assertEqual(
            keys(build(body))[2:],
            [
                "intro.architecture.1.caption",
                "intro.architecture.1.layer.1.title",
                "intro.architecture.1.layer.1.module.1",
            ],
        )
        # The `...` placeholder is a rendering glyph, not prose to translate.
        self.assertNotIn("intro.architecture.1.layer.1.module.2", keys(build(body)))

    def test_row_items(self):
        def body(p):
            p.title("T")
            p.row(caption="并排").image("a.svg", caption="左").image("b.svg", caption="右")

        self.assertEqual(
            keys(build(body))[2:],
            ["intro.row.1.caption", "intro.row.1.item.1.caption",
             "intro.row.1.item.2.caption"],
        )

    def test_an_animation_yields_one_set_of_keys_for_every_frame(self):
        def body(p):
            p.title("T")
            p.slide("同一段文字")
            p.frames("a.svg", "b.svg", "c.svg", caption="动画")

        built = build(body, page_id="anim")
        self.assertEqual(len(model.flatten_pages(built.children)), 3)
        self.assertEqual(
            keys(built),
            ["lecture.title", "anim.title", "anim.slide.1", "anim.image.1.caption"],
        )

    def test_a_bridge_page_keys_off_its_auto_id(self):
        lecture = LectureBuilder(id="lec", title="标题")
        lecture.page("intro", body=lambda p: (p.title("页"), p.slide("x")))
        lecture.bridge("换个话题")
        self.assertIn("bridge-1.bridge.1", keys(lecture.build()))

    def test_code_and_spacer_carry_nothing_to_translate(self):
        def body(p):
            p.title("T")
            p.code("pseudo", "if x\n    return")
            p.spacer(20)

        self.assertEqual(keys(build(body)), ["lecture.title", "intro.title"])


class BlockKeyValidationTest(unittest.TestCase):
    def test_a_key_may_not_contain_a_dot(self):
        def body(p):
            p.title("T")
            p.slide("x", key="a.b")

        with self.assertRaisesRegex(model.ValidationError, "may not contain"):
            build(body)

    def test_two_blocks_on_one_page_may_not_share_a_key(self):
        def body(p):
            p.title("T")
            p.slide("x", key="why")
            p.slide("y", key="why")

        with self.assertRaisesRegex(model.ValidationError, "Duplicate block key"):
            build(body)

    def test_an_empty_key_is_refused(self):
        def body(p):
            p.title("T")
            p.slide("x", key="  ")

        with self.assertRaisesRegex(model.ValidationError, "Empty block key"):
            build(body)

    def test_the_same_key_on_two_pages_is_fine(self):
        # Keys are page-local; the full key namespaces them by page id.
        lecture = LectureBuilder(id="lec", title="标题")
        lecture.page("a", body=lambda p: (p.title("A"), p.slide("x", key="why")))
        lecture.page("b", body=lambda p: (p.title("B"), p.slide("y", key="why")))
        self.assertEqual(keys(lecture.build())[1:], ["a.title", "a.why", "b.title", "b.why"])


class ApplyTest(unittest.TestCase):
    def setUp(self):
        def body(p):
            p.title("中文标题")
            p.slide("中文正文").footnote("来源")
            p.image("a.svg", caption="图一")

        self.lecture = build(body)

    def apply(self, table: dict, **kwargs) -> model.Lecture:
        return _apply_with(self.lecture, table, **kwargs)

    def test_entries_replace_the_baseline(self):
        applied = self.apply({"intro.title": "English title"})
        page = model.flatten_pages(applied.children)[0]
        self.assertEqual(page.title, "English title")

    def test_a_missing_entry_falls_back_and_is_recorded(self):
        applied = self.apply({"intro.title": "English title"})
        self.assertIn("intro.slide.1", applied.untranslated)
        self.assertNotIn("intro.title", applied.untranslated)
        page = model.flatten_pages(applied.children)[0]
        # The baseline as `p.slide` stored it — autobolded, not the raw literal.
        self.assertEqual(page.blocks[0].content, "**中文正文**")

    def test_an_empty_entry_counts_as_missing(self):
        applied = self.apply({"intro.slide.1": ""})
        self.assertIn("intro.slide.1", applied.untranslated)

    def test_a_fallen_back_block_is_marked_for_the_viewer(self):
        applied = self.apply({"intro.slide.1": "English body"})
        page = model.flatten_pages(applied.children)[0]
        slide, image = page.blocks
        # The slide's own text was translated but its footnote was not.
        self.assertTrue(slide.untranslated)
        self.assertTrue(image.untranslated)

    def test_a_fully_translated_block_is_not_marked(self):
        applied = self.apply({
            "intro.slide.1": "English body",
            "intro.slide.1.footnote.1": "Source",
        })
        slide = model.flatten_pages(applied.children)[0].blocks[0]
        self.assertFalse(slide.untranslated)

    def test_a_slide_translation_gets_the_slide_authoring_rules(self):
        # A translator writing slide text writes slide text: flush-left prose
        # bolds and `==keyword==` expands, exactly as in `p.slide(...)`.
        applied = self.apply({"intro.slide.1": "English headline\n- a ==key== point"})
        self.assertEqual(
            model.flatten_pages(applied.children)[0].blocks[0].content,
            "**English headline**\n- a <mark>key</mark> point",
        )

    def test_a_slide_the_author_kept_unbolded_stays_unbolded(self):
        # `p.slide(..., autobold=False)` is the author's choice about the block,
        # not about one language of it.
        def body(p):
            p.title("中文标题")
            p.slide("中文正文", autobold=False)

        applied = _apply_with(build(body), {"intro.slide.1": "English headline"})
        self.assertEqual(
            model.flatten_pages(applied.children)[0].blocks[0].content,
            "English headline",
        )

    def test_a_translation_copied_from_the_src_comment_is_unchanged(self):
        # `# src:` quotes the baseline after the same pass, so mirroring it must
        # not double-bold or re-expand.
        applied = self.apply({"intro.slide.1": "**Already bold**"})
        self.assertEqual(
            model.flatten_pages(applied.children)[0].blocks[0].content,
            "**Already bold**",
        )

    def test_the_applied_language_is_recorded(self):
        self.assertEqual(self.apply({}).lang, "en")

    def test_strict_refuses_a_lecture_with_untranslated_strings(self):
        with self.assertRaisesRegex(model.ValidationError, "untranslated"):
            self.apply({}, strict=True)

    def test_strict_passes_when_everything_is_translated(self):
        full = {key: f"en:{key}" for key in keys(self.lecture)}
        applied = self.apply(full, strict=True)
        self.assertEqual(applied.untranslated, ())

    def test_the_baseline_lecture_is_untouched(self):
        self.apply({"intro.title": "English title"})
        page = model.flatten_pages(self.lecture.children)[0]
        self.assertEqual(page.title, "中文标题")
        self.assertIsNone(self.lecture.lang)

    def test_nested_content_is_rebuilt_rather_than_mutated(self):
        def body(p):
            p.title("T")
            p.table([["甲", "乙"]], headers=["一", "二"])

        lecture = build(body)
        applied = _apply_with(lecture, {"intro.table.1.cell.1.0": "first"})
        self.assertEqual(
            model.flatten_pages(applied.children)[0].blocks[0].content["rows"],
            [["first", "乙"]],
        )
        self.assertEqual(
            model.flatten_pages(lecture.children)[0].blocks[0].content["rows"],
            [["甲", "乙"]],
        )

    def test_an_animation_translates_every_frame_from_one_key(self):
        def body(p):
            p.title("T")
            p.slide("中文")
            p.frames("a.svg", "b.svg")

        applied = _apply_with(build(body, page_id="anim"), {"anim.slide.1": "English"})
        pages = model.flatten_pages(applied.children)
        self.assertEqual([page.blocks[0].content for page in pages],
                         ["**English**", "**English**"])
        self.assertEqual(applied.untranslated.count("anim.title"), 1)


def _apply_with(lecture: model.Lecture, table: dict, **kwargs) -> model.Lecture:
    """`i18n.apply` over a throwaway ``i18n/en.toml`` holding ``table``."""
    with tempfile.TemporaryDirectory() as tmp:
        write_overlay(Path(tmp), "en", table)
        return i18n.apply(lecture, Path(tmp), "en", **kwargs)


def write_overlay(directory: Path, lang: str, table: dict[str, str]) -> Path:
    path = i18n.overlay_path(directory, lang)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        "\n".join(
            f"[{i18n.toml_string(key)}]\ntext = {i18n.toml_string(text)}\n"
            for key, text in table.items()
        ),
        encoding="utf-8",
    )
    return path


class TomlEmitterTest(unittest.TestCase):
    def round_trip(self, text: str) -> str:
        emitted = i18n.toml_string(text)
        return i18n.normalize(tomllib.loads(f"k = {emitted}")["k"])

    def test_preserves_marks_math_and_backslashes(self):
        for text in ("==mark== and **bold**", "$$ x = 1 $$", r"a \sum b", "100% & <tag>"):
            with self.subTest(text=text):
                self.assertEqual(self.round_trip(text), text)

    def test_preserves_a_leading_space(self):
        # The one-space indent is autobold's escape hatch; losing it in the
        # round trip would bold a line the author deliberately left plain.
        self.assertEqual(self.round_trip(" 这行不加粗"), " 这行不加粗")

    def test_preserves_multi_line_text(self):
        text = "第一行\n- 第二行\n 第三行"
        self.assertEqual(self.round_trip(text), text)

    def test_handles_a_body_containing_the_literal_delimiter(self):
        self.assertEqual(self.round_trip("has ''' inside"), "has ''' inside")

    def test_handles_a_quote_on_one_line(self):
        self.assertEqual(self.round_trip("don't"), "don't")

    def test_an_empty_string_stays_empty(self):
        self.assertEqual(self.round_trip(""), "")


class ExtractTest(unittest.TestCase):
    def entries(self, **pairs) -> list[i18n.Entry]:
        return [i18n.Entry(key, text) for key, text in pairs.items()]

    def parse(self, toml_text: str) -> dict:
        return tomllib.loads(toml_text)

    def test_a_new_key_arrives_untranslated(self):
        data = self.parse(i18n.extract(self.entries(a="中文"), {}))
        self.assertEqual(data["a"]["text"], "")
        self.assertEqual(data["a"]["src_hash"], i18n.source_hash("中文"))

    def test_an_existing_translation_is_kept(self):
        existing = {"a": i18n.OverlayEntry(i18n.source_hash("中文"), "English")}
        data = self.parse(i18n.extract(self.entries(a="中文"), existing))
        self.assertEqual(data["a"]["text"], "English")

    def test_a_changed_baseline_keeps_the_text_and_is_flagged(self):
        existing = {"a": i18n.OverlayEntry("deadbeef", "English")}
        out = i18n.extract(self.entries(a="中文"), existing)
        self.assertIn("# CHANGED: was deadbeef", out)
        data = self.parse(out)
        self.assertEqual(data["a"]["text"], "English")
        self.assertEqual(data["a"]["src_hash"], i18n.source_hash("中文"))

    def test_an_orphan_is_kept_under_a_banner_rather_than_deleted(self):
        existing = {"gone": i18n.OverlayEntry("deadbeef", "English")}
        out = i18n.extract(self.entries(a="中文"), existing)
        self.assertIn("# orphaned", out)
        self.assertEqual(self.parse(out)["gone"]["text"], "English")

    def test_round_trips_through_the_loader(self):
        entries = self.entries(a="中文\n 第二行", b="==mark==")
        existing = {"a": i18n.OverlayEntry(i18n.source_hash("中文\n 第二行"), "en\n line")}
        loaded = i18n.parse_overlay(i18n.extract(entries, existing))
        self.assertEqual(loaded["a"].text, "en\n line")
        self.assertEqual(loaded["b"].text, "")


class CheckTest(unittest.TestCase):
    def setUp(self):
        def body(p):
            p.title("中文标题")
            p.slide("中文正文")

        self.lecture = build(body)

    def test_reports_missing_changed_and_orphaned(self):
        table = {
            "lecture.title": i18n.OverlayEntry(i18n.source_hash("标题"), "Title"),
            "intro.title": i18n.OverlayEntry("deadbeef", "Heading"),
            "stale.key": i18n.OverlayEntry("deadbeef", "Gone"),
        }
        report = i18n.check(self.lecture, table)
        self.assertEqual(report.missing, ("intro.slide.1",))
        self.assertEqual(report.changed, ("intro.title",))
        self.assertEqual(report.orphaned, ("stale.key",))
        self.assertFalse(report.clean)

    def test_clean_when_every_key_matches(self):
        table = {
            entry.key: i18n.OverlayEntry(i18n.source_hash(entry.text), "x")
            for entry in i18n.collect(self.lecture)
        }
        self.assertTrue(i18n.check(self.lecture, table).clean)


class UiStringTest(unittest.TestCase):
    def test_known_languages(self):
        self.assertEqual(i18n.ui("zh", "references"), "参考文献")
        self.assertEqual(i18n.ui("en", "references"), "References")

    def test_no_language_keeps_todays_behaviour(self):
        self.assertEqual(i18n.ui(None, "outline"), "大纲")

    def test_an_unknown_language_falls_back_to_chinese_chrome(self):
        # An overlay for `ja` still translates the lecture; only the chrome
        # falls back, which must not block the render.
        self.assertEqual(i18n.ui("ja", "figure"), "图")

    def test_the_table_is_complete_for_every_language(self):
        for lang, table in i18n.UI_STRINGS.items():
            with self.subTest(lang=lang):
                self.assertEqual(set(table), set(i18n.UI_STRINGS["zh"]))


class OverlayFileTest(unittest.TestCase):
    def test_a_missing_file_is_an_error_not_an_empty_overlay(self):
        with self.assertRaisesRegex(model.ValidationError, "no en overlay"):
            i18n.load_overlay(Path("/nonexistent-lecture"), "en")

    def test_a_missing_file_is_tolerated_for_a_review_source(self):
        self.assertEqual(i18n.try_load_overlay(Path("/nonexistent-lecture"), "en"), {})

    def test_a_broken_file_names_itself(self):
        with self.assertRaisesRegex(model.ValidationError, "cannot parse overlay"):
            i18n.parse_overlay("[unclosed\n", Path("x/en.toml"))

    def test_normalize_only_strips_trailing_newlines(self):
        self.assertEqual(i18n.normalize("\n a \n\n"), "\n a ")


class BookOverlayTest(unittest.TestCase):
    def test_collect_book_names_the_front_matter(self):
        from lecturekit.book import load_book

        book = load_book(Path("tests/fixtures/book"))
        self.assertEqual(
            [entry.key for entry in i18n.collect_book(book)],
            ["book.title", "book.preface"],
        )


if __name__ == "__main__":
    unittest.main()


class ViewerTest(unittest.TestCase):
    """What the deck does with a translated lecture (viewer only, by design)."""

    def lecture(self, table: dict) -> model.Lecture:
        def body(p):
            p.title("中文标题")
            p.slide("中文正文")
            p.slide("另一段")

        return _apply_with(build(body), table)

    def test_an_untranslated_block_is_washed(self):
        from lecturekit.renderers.viewer import render_marp_page

        applied = self.lecture({"intro.slide.1": "English body"})
        markup = render_marp_page(model.flatten_pages(applied.children)[0])
        self.assertIn('<div class="lk-untranslated">', markup)
        # One wash: the translated block is not marked.
        self.assertEqual(markup.count('class="lk-untranslated"'), 1)

    def test_a_baseline_lecture_is_never_washed(self):
        from lecturekit.renderers.viewer import render_marp_page

        lecture = build(lambda p: (p.title("T"), p.slide("x")))
        markup = render_marp_page(model.flatten_pages(lecture.children)[0])
        self.assertNotIn("lk-untranslated", markup)

    def test_the_chrome_strings_ride_in_lecture_json(self):
        from lecturekit.renderers.viewer import build_data

        data = build_data(self.lecture({}))
        self.assertEqual(data["ui"]["outline"], "Outline")

    def test_the_references_page_follows_the_language(self):
        from lecturekit.renderers.viewer import with_references_page

        def body(p):
            p.title("T")
            p.slide("x")
            p.cite(title="A paper", year="2020")

        applied = _apply_with(build(body), {})
        pages = model.flatten_pages(with_references_page(applied).children)
        self.assertEqual(pages[-1].title, "References")


class LatexTest(unittest.TestCase):
    def test_the_figure_prefix_follows_the_language(self):
        from lecturekit.renderers.latex.blocks import Ctx

        ctx = Ctx(lecture_id="lec", page_id="p", slide_width=1280, assets=None)
        self.assertIn("图~", ctx.resolve_ref("fig"))
        self.assertIn("Figure~", replace_lang(ctx, "en").resolve_ref("fig"))

    def test_chapter_end_headings_follow_the_language(self):
        from lecturekit.renderers.latex.blocks import emit_citations, emit_news

        citations = (model.Citation(title="A paper"),)
        news = (model.NewsItem(title="A post", url="http://x"),)
        self.assertIn("参考文献", emit_citations(citations))
        self.assertIn("References", emit_citations(citations, "en"))
        self.assertIn("延伸阅读", emit_news(news))
        self.assertIn("Further reading", emit_news(news, "en"))


def replace_lang(ctx, lang):
    import dataclasses

    return dataclasses.replace(ctx, lang=lang)


class CliTest(unittest.TestCase):
    """The `--lang` flag and the `i18n` subcommands, end to end."""

    SOURCE = "tests/fixtures/sample"

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = Path(self.tmp.name, "lec")
        shutil.copytree(self.SOURCE, self.dir)
        self.addCleanup(self.tmp.cleanup)

    def run_cli(self, argv: list[str]) -> tuple[int, str]:
        from lecturekit.cli import main

        buffer = io.StringIO()
        with contextlib.redirect_stdout(buffer), contextlib.redirect_stderr(buffer):
            code = main(argv)
        return code, buffer.getvalue()

    def test_extract_then_check_then_apply(self):
        code, out = self.run_cli(["i18n", "extract", str(self.dir), "--lang", "en"])
        self.assertEqual(code, 0)
        self.assertIn("en.toml", out)
        self.assertTrue(i18n.overlay_path(self.dir, "en").exists())

        # Nothing is translated yet, so `check` fails.
        code, out = self.run_cli(["i18n", "check", str(self.dir), "--lang", "en"])
        self.assertEqual(code, 1)
        self.assertIn("missing", out)

        entries = i18n.collect(_load(self.dir))
        write_overlay(self.dir, "en", {e.key: f"en:{e.key}" for e in entries})
        code, out = self.run_cli(["i18n", "check", str(self.dir), "--lang", "en"])
        self.assertEqual(code, 0)
        self.assertIn("0 missing", out)

        code, out = self.run_cli(["inspect", str(self.dir), "--lang", "en"])
        self.assertEqual(code, 0)
        self.assertIn("en:lecture.title", out)

    def test_check_allows_a_changed_baseline_on_request(self):
        entries = i18n.collect(_load(self.dir))
        path = i18n.overlay_path(self.dir, "en")
        path.parent.mkdir(parents=True)
        path.write_text(i18n.extract(
            entries,
            {e.key: i18n.OverlayEntry("deadbeef", f"en:{e.key}") for e in entries},
        ), encoding="utf-8")
        # `extract` refreshed the hashes, so re-stale them to model an edit.
        path.write_text(
            path.read_text().replace("src_hash = '", "src_hash = 'x"), encoding="utf-8"
        )
        code, out = self.run_cli(["i18n", "check", str(self.dir), "--lang", "en"])
        self.assertEqual(code, 1)
        self.assertIn("changed", out)
        code, _ = self.run_cli(
            ["i18n", "check", str(self.dir), "--lang", "en", "--allow-changed"]
        )
        self.assertEqual(code, 0)

    def test_a_missing_overlay_is_refused_rather_than_ignored(self):
        code, out = self.run_cli(["inspect", str(self.dir), "--lang", "en"])
        self.assertEqual(code, 1)
        self.assertIn("no en overlay", out)

    def test_the_default_output_dir_carries_the_language(self):
        from lecturekit.cli import default_out_dir

        lecture = _load(self.dir)
        self.assertEqual(default_out_dir(lecture, "viewer"), Path("build/sample-viewer"))
        self.assertEqual(
            default_out_dir(lecture, "viewer", "en"), Path("build/sample-en-viewer")
        )

    def test_strict_refuses_to_render_a_half_translated_deck(self):
        write_overlay(self.dir, "en", {"lecture.title": "Title"})
        out_dir = Path(self.tmp.name, "out")
        code, out = self.run_cli([
            "render", str(self.dir), "--lang", "en", "--strict",
            "--no-build", "--out", str(out_dir),
        ])
        self.assertEqual(code, 1)
        self.assertIn("untranslated", out)


def _load(directory: Path) -> model.Lecture:
    from lecturekit.cli import load_lecture

    return load_lecture(directory)


class DevServerTest(unittest.TestCase):
    """A watch session renders with the overlay, and reloads when it changes."""

    def test_render_once_applies_the_overlay(self):
        from lecturekit import dev_server

        with tempfile.TemporaryDirectory() as tmp:
            lecture_dir = Path(tmp, "lec")
            shutil.copytree("tests/fixtures/sample", lecture_dir)
            write_overlay(lecture_dir, "en", {"lecture.title": "Sample (EN)"})
            out = Path(tmp, "out")
            dev_server.render_once(lecture_dir, out, lang="en")
            data = json.loads(Path(out, "lecture.json").read_text())
            self.assertEqual(data["lecture"]["title"], "Sample (EN)")
            self.assertEqual(data["ui"]["outline"], "Outline")

    def test_an_overlay_edit_is_a_watched_source_change(self):
        from lecturekit.dev_server import LectureWatchFilter

        with tempfile.TemporaryDirectory() as tmp:
            lecture_dir = Path(tmp, "lec")
            lecture_dir.mkdir()
            change_filter = LectureWatchFilter(lecture_dir, Path(tmp, "out"))
            overlay = i18n.overlay_path(lecture_dir, "en")
            self.assertTrue(change_filter(None, str(overlay)))
