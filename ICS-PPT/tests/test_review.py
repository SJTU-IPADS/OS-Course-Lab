"""``review_section(...)``: replaying another lecture's pages as review."""

import textwrap
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from lecturekit import dev_server, model
from lecturekit.cli import load_lecture
from lecturekit.model import ValidationError
from lecturekit.renderers.viewer import StaticViewerRenderer
from lecturekit.serialize import lecture_to_dict

SOURCE_LECTURE = """
from lecturekit.dsl import Lecture

lecture = Lecture(id="src", title="Source Lecture")


def quorum(p):
    p.title("Quorums intersect")
    p.slide("any two majorities share a member")
    p.image("assets/quorum.svg", caption="两个多数派", ref="quorum-venn")
    p.cite(title="Paxos Made Simple", author="Lamport", year="2001")


def animation(p):
    p.title("Commit logging")
    p.slide("log first")
    p.frames("assets/log-1.svg", "assets/log-2.svg")


with lecture.section("Consensus", id="consensus") as s:
    s.page("quorum", body=quorum)
    s.page("commit-log", body=animation)
"""

HOST_LECTURE = """
from lecturekit.dsl import Lecture
import review

lecture = Lecture(id="host", title="Host Lecture")


def today(p):
    p.title("Today")
    p.slide("agenda")


lecture.page("today", body=today)
review.attach(lecture)
"""


def _host_review(sources: str, extra: str = "") -> str:
    """A ``review.py`` whose ``attach`` builds one review section, plus `extra` lines."""
    body = [
        "from lecturekit.dsl import review_section",
        "",
        "",
        "def attach(lecture):",
        f'    section = review_section(lecture, "回顾", {sources})',
        textwrap.indent(textwrap.dedent(extra).strip("\n"), "    ") if extra
        else "    return section",
        "",
    ]
    return "\n".join(body)


class ReviewFixture:
    """A `source` lecture and a `host` lecture that borrows from it."""

    def __init__(self, root: Path, sources: str = '{"../source": ["quorum"]}', extra: str = ""):
        self.root = root
        self.source = root / "source"
        self.host = root / "host"
        (self.source / "assets").mkdir(parents=True)
        for name in ("quorum.svg", "log-1.svg", "log-2.svg"):
            (self.source / "assets" / name).write_text(f"<svg><!--{name}--></svg>")
        (self.source / "lecture.py").write_text(SOURCE_LECTURE)

        (self.host / "assets").mkdir(parents=True)
        (self.host / "assets" / "own.svg").write_text("<svg/>")
        (self.host / "lecture.py").write_text(HOST_LECTURE)
        (self.host / "review.py").write_text(_host_review(sources, extra))

    def load(self) -> model.Lecture:
        return load_lecture(self.host)


class ReviewTest(unittest.TestCase):
    def fixture(self, **kwargs) -> ReviewFixture:
        tmp = TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        return ReviewFixture(Path(tmp.name), **kwargs)

    # --- what arrives ---

    def test_borrowed_page_keeps_its_content_under_a_namespaced_id(self):
        lecture = self.fixture().load()
        page = model.flatten_pages(lecture.children)[-1]

        self.assertEqual(page.id, "src/quorum")
        self.assertEqual(page.title, "Quorums intersect")
        # Autobolded on the way in, exactly as it was in the source lecture.
        self.assertEqual(page.blocks[0].content, "**any two majorities share a member**")

    def test_borrowed_page_is_skipped_by_the_book(self):
        lecture = self.fixture().load()
        page = model.flatten_pages(lecture.children)[-1]
        self.assertEqual(page.book, "skip")

    def test_borrowed_figure_ref_is_dropped(self):
        """A ref anchors a figure in the source chapter; two lectures' refs must not collide."""
        lecture = self.fixture().load()
        image = model.flatten_pages(lecture.children)[-1].blocks[1]
        self.assertIsNone(image.content.get("ref"))
        self.assertEqual(image.content["caption"], "两个多数派")

    def test_borrowed_image_src_is_namespaced(self):
        lecture = self.fixture().load()
        image = model.flatten_pages(lecture.children)[-1].blocks[1]
        self.assertEqual(image.content["src"], "assets/src/quorum.svg")

    def test_a_borrowed_citation_joins_this_decks_references(self):
        """The slide is on this projector, so its source belongs on this deck's list."""
        from lecturekit import references

        lecture = self.fixture().load()
        collected = references.collect_citations(model.flatten_pages(lecture.children))
        self.assertEqual([entry.citation.title for entry in collected],
                         ["Paxos Made Simple"])

    def test_the_section_lands_where_it_was_added(self):
        lecture = self.fixture().load()
        ids = [page.id for page in model.flatten_pages(lecture.children)]
        self.assertEqual(ids, ["today", "src/quorum"])

    def test_borrowed_lecture_is_recorded_for_the_renderers(self):
        fixture = self.fixture()
        lecture = fixture.load()
        self.assertEqual(len(lecture.borrowed), 1)
        entry = lecture.borrowed[0]
        self.assertEqual(entry.lecture_id, "src")
        self.assertEqual(Path(entry.directory), fixture.source.resolve())

    # --- naming pages ---

    def test_an_animation_id_brings_every_frame(self):
        lecture = self.fixture(sources='{"../source": ["commit-log"]}').load()
        ids = [page.id for page in model.flatten_pages(lecture.children)]
        self.assertEqual(ids, ["today", "src/commit-log-1", "src/commit-log-2"])

    def test_a_borrowed_animation_stays_one_slide(self):
        lecture = self.fixture(sources='{"../source": ["commit-log"]}').load()
        pages = model.flatten_pages(lecture.children)
        self.assertEqual([page.frame_group.id for page in pages[1:]],
                         ["src/commit-log", "src/commit-log"])
        self.assertEqual(model.slide_numbers(pages), [1, 2, 2])

    def test_pages_arrive_in_the_order_asked_for(self):
        lecture = self.fixture(sources='{"../source": ["commit-log", "quorum"]}').load()
        ids = [page.id for page in model.flatten_pages(lecture.children)]
        self.assertEqual(ids[-1], "src/quorum")

    def test_one_section_may_span_several_lectures(self):
        fixture = self.fixture()
        second = fixture.root / "other"
        (second / "assets").mkdir(parents=True)
        (second / "assets" / "quorum.svg").write_text("<svg/>")
        (second / "lecture.py").write_text(SOURCE_LECTURE.replace('id="src"', 'id="other"'))
        (fixture.host / "review.py").write_text(
            _host_review('{"../source": ["quorum"], "../other": ["quorum"]}')
        )

        lecture = fixture.load()
        ids = [page.id for page in model.flatten_pages(lecture.children)]
        self.assertEqual(ids, ["today", "src/quorum", "other/quorum"])
        self.assertEqual(len(lecture.borrowed), 2)

    def test_an_author_may_add_pages_after_the_borrowed_ones(self):
        extra = textwrap.dedent("""
            def bridge(p):
                p.title("Back to today")
                p.slide("so much for review")

            section.page("bridge", body=bridge)
            return section
        """)
        lecture = self.fixture(extra=extra).load()
        ids = [page.id for page in model.flatten_pages(lecture.children)]
        self.assertEqual(ids, ["today", "src/quorum", "bridge"])

    def test_a_borrowed_page_can_be_selected_by_its_namespaced_id(self):
        lecture = self.fixture().load()
        pruned = model.select_pages(lecture, "src/quorum")
        self.assertEqual([page.id for page in model.flatten_pages(pruned.children)],
                         ["src/quorum"])

    # --- refusals ---

    def test_an_unknown_page_id_names_what_is_available(self):
        fixture = self.fixture(sources='{"../source": ["nope"]}')
        with self.assertRaises(ValidationError) as caught:
            fixture.load()
        message = str(caught.exception)
        self.assertIn("nope", message)
        self.assertIn("quorum", message)
        self.assertIn("commit-log", message)

    def test_a_missing_source_directory_is_refused(self):
        fixture = self.fixture(sources='{"../gone": ["quorum"]}')
        with self.assertRaises(ValidationError) as caught:
            fixture.load()
        self.assertIn("not a directory", str(caught.exception))

    def test_a_source_that_fails_to_load_is_refused(self):
        fixture = self.fixture()
        (fixture.source / "lecture.py").write_text("raise RuntimeError('broken')")
        with self.assertRaises(ValidationError) as caught:
            fixture.load()
        self.assertIn("failed to load", str(caught.exception))

    def test_two_sources_sharing_a_lecture_id_are_refused(self):
        """The id namespaces assets, so a collision would silently overwrite figures."""
        fixture = self.fixture()
        twin = fixture.root / "twin"
        (twin / "assets").mkdir(parents=True)
        (twin / "lecture.py").write_text(SOURCE_LECTURE)
        (fixture.host / "review.py").write_text(
            _host_review('{"../source": ["quorum"], "../twin": ["quorum"]}')
        )
        with self.assertRaises(ValidationError) as caught:
            fixture.load()
        self.assertIn("share the lecture id", str(caught.exception))

    def test_borrowing_from_itself_is_refused(self):
        fixture = self.fixture()
        (fixture.source / "lecture.py").write_text(SOURCE_LECTURE.replace('id="src"', 'id="host"'))
        with self.assertRaises(ValidationError) as caught:
            fixture.load()
        self.assertIn("cannot borrow from itself", str(caught.exception))

    def test_an_empty_mapping_is_refused(self):
        fixture = self.fixture(sources="{}")
        with self.assertRaises(ValidationError) as caught:
            fixture.load()
        self.assertIn("non-empty", str(caught.exception))

    def test_a_source_naming_no_pages_is_refused(self):
        fixture = self.fixture(sources='{"../source": []}')
        with self.assertRaises(ValidationError) as caught:
            fixture.load()
        self.assertIn("names no pages", str(caught.exception))

    # --- paths ---

    def test_an_absolute_source_path_is_taken_as_given(self):
        tmp = TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        root = Path(tmp.name)
        fixture = ReviewFixture(root)
        (fixture.host / "review.py").write_text(
            _host_review(f'{{"{fixture.source}": ["quorum"]}}')
        )
        self.assertEqual(fixture.load().borrowed[0].lecture_id, "src")

    def test_a_source_sees_its_own_sibling_modules(self):
        """The source is loaded *during* the host's load, and `import pages` is a bare name.

        Without isolation the host's `pages` is already cached, so the source
        would silently build the host's page instead of its own.
        """
        fixture = self.fixture()
        for lecture_dir, marker in ((fixture.host, "host"), (fixture.source, "source")):
            (lecture_dir / "pages.py").write_text(
                f'def shared(p):\n    p.title("from {marker}")\n    p.slide("{marker}")\n'
            )
        (fixture.source / "lecture.py").write_text(
            SOURCE_LECTURE + '\nimport pages\nlecture.page("shared", body=pages.shared)\n'
        )
        (fixture.host / "lecture.py").write_text(
            HOST_LECTURE + '\nimport pages\nlecture.page("host-shared", body=pages.shared)\n'
        )
        (fixture.host / "review.py").write_text(_host_review('{"../source": ["shared"]}'))

        titles = {page.id: page.title for page in model.flatten_pages(fixture.load().children)}
        self.assertEqual(titles["src/shared"], "from source")
        self.assertEqual(titles["host-shared"], "from host")

    def test_a_relative_path_resolves_against_the_calling_file(self):
        """Not against the CWD: the path means what it means where it was written."""
        fixture = self.fixture()
        nested = fixture.host / "parts"
        nested.mkdir()
        (nested / "review_pages.py").write_text(
            _host_review('{"../../source": ["quorum"]}')
        )
        (fixture.host / "review.py").write_text(textwrap.dedent("""
            import sys
            from pathlib import Path

            sys.path.insert(0, str(Path(__file__).parent / "parts"))
            import review_pages


            def attach(lecture):
                return review_pages.attach(lecture)
        """))
        self.assertEqual(fixture.load().borrowed[0].lecture_id, "src")


class ReviewBundleTest(unittest.TestCase):
    """What the review pages need from the renderers."""

    def _render(self, **kwargs):
        tmp = TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        root = Path(tmp.name)
        fixture = ReviewFixture(root, **kwargs)
        lecture = fixture.load()
        out = root / "out"
        StaticViewerRenderer(asset_root=fixture.host).render(lecture, out)
        return lecture, out

    def test_borrowed_assets_land_under_the_source_namespace(self):
        _, out = self._render()
        self.assertIn("quorum.svg", (out / "assets" / "src" / "quorum.svg").read_text())

    def test_the_hosts_own_assets_survive_the_borrowed_copy(self):
        _, out = self._render()
        self.assertTrue((out / "assets" / "own.svg").exists())

    def test_the_bundle_serves_the_src_the_page_names(self):
        lecture, out = self._render()
        image = model.flatten_pages(lecture.children)[-1].blocks[1]
        self.assertTrue((out / image.content["src"]).exists())

    def test_the_ast_records_the_borrowed_sources(self):
        lecture, _ = self._render()
        data = lecture_to_dict(lecture)
        self.assertEqual([entry["lecture_id"] for entry in data["borrowed"]], ["src"])

    def test_resolve_asset_maps_a_borrowed_src_back_to_its_lecture(self):
        """How pptx/latex find the original file, which no bundle copy helps with."""
        borrowed = (model.Borrowed(lecture_id="src", directory="/lectures/src"),)
        self.assertEqual(
            model.resolve_asset("assets/src/quorum.svg", Path("/lectures/host"), borrowed),
            Path("/lectures/src/assets/quorum.svg"),
        )
        self.assertEqual(
            model.resolve_asset("assets/own.svg", Path("/lectures/host"), borrowed),
            Path("/lectures/host/assets/own.svg"),
        )


class ReviewWatchTest(unittest.TestCase):
    """A live reference is only live if editing the source reloads the deck."""

    def setUp(self):
        tmp = TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.root = Path(tmp.name)
        self.fixture = ReviewFixture(self.root)
        self.out = self.root / "out"

    def test_render_once_reports_the_review_sources(self):
        found = dev_server.render_once(self.fixture.host, self.out)
        self.assertEqual([Path(path) for path in found], [self.fixture.source.resolve()])

    def test_review_sources_join_the_watch_roots(self):
        paths = dev_server._watch_paths(
            self.fixture.host, self.out, (self.fixture.source,)
        )
        self.assertIn(self.fixture.source.resolve(), paths)

    def test_a_source_nested_in_the_lecture_adds_no_root(self):
        nested = self.fixture.host / "inner"
        nested.mkdir()
        paths = dev_server._watch_paths(self.fixture.host, self.out, (nested,))
        self.assertNotIn(nested.resolve(), paths)

    def test_the_filter_accepts_edits_in_a_review_source(self):
        change_filter = dev_server.LectureWatchFilter(
            self.fixture.host, self.out, (self.fixture.source,)
        )
        self.assertTrue(change_filter(None, str(self.fixture.source / "lecture.py")))
        self.assertTrue(change_filter(None, str(self.fixture.source / "assets" / "quorum.svg")))

    def test_the_filter_still_ignores_noise_in_a_review_source(self):
        change_filter = dev_server.LectureWatchFilter(
            self.fixture.host, self.out, (self.fixture.source,)
        )
        self.assertFalse(change_filter(None, str(self.fixture.source / "lecture.pyc")))
        self.assertFalse(
            change_filter(None, str(self.fixture.source / "__pycache__" / "lecture.pyc"))
        )

    def test_unrelated_directories_are_still_ignored(self):
        change_filter = dev_server.LectureWatchFilter(
            self.fixture.host, self.out, (self.fixture.source,)
        )
        self.assertFalse(change_filter(None, str(self.root / "elsewhere" / "lecture.py")))


if __name__ == "__main__":
    unittest.main()
