import tempfile
import unittest
from pathlib import Path

from lecturekit import Lecture, model
from lecturekit.renderers.latex.assets import AssetCopier
from lecturekit.renderers.latex.blocks import emit_citations
from lecturekit.renderers.latex.renderer import _chapter


class EmitCitationsTest(unittest.TestCase):
    def test_empty_is_blank(self):
        self.assertEqual(emit_citations(()), "")

    def test_lists_title_and_meta(self):
        out = emit_citations((
            model.Citation(title="Attention Is All You Need",
                           author="Vaswani et al.", year="2017", venue="NeurIPS"),
        ))
        self.assertIn(r"\section*{参考文献}", out)
        self.assertIn("Attention Is All You Need", out)
        self.assertIn("Vaswani et al.", out)
        self.assertIn("NeurIPS", out)

    def test_url_becomes_href(self):
        out = emit_citations((
            model.Citation(title="T", url="https://x.org/a"),
        ))
        self.assertIn(r"\href{https://x.org/a}", out)


class ChapterCollectionTest(unittest.TestCase):
    def _chapter(self, lecture):
        tmp = Path(tempfile.mkdtemp())
        (tmp / "src").mkdir(parents=True, exist_ok=True)
        return _chapter(lecture.build(), tmp / "src", AssetCopier(tmp / "out"))

    def test_chapter_collects_citations_into_section(self):
        lecture = Lecture(id="lec", title="L")

        def p1(p):
            p.title("A")
            p.prose("text")
            p.cite(title="Paper One", key="p1")

        def p2(p):
            p.title("B")
            p.prose("text")
            p.cite(title="Paper One", key="p1")  # duplicate, should merge
            p.cite(title="Paper Two", key="p2")

        lecture.page("a", body=p1)
        lecture.page("b", body=p2)
        out = self._chapter(lecture)
        self.assertIn(r"\section*{参考文献}", out)
        self.assertEqual(out.count("Paper One"), 1)  # deduped
        self.assertIn("Paper Two", out)

    def test_no_section_without_citations(self):
        lecture = Lecture(id="lec", title="L")
        lecture.page("a", body=lambda p: (p.title("A"), p.prose("x")))
        self.assertNotIn("参考文献", self._chapter(lecture))


if __name__ == "__main__":
    unittest.main()
