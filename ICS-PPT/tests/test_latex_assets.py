import tempfile
import unittest
from pathlib import Path

from lecturekit.renderers.latex.assets import AssetCopier


class AssetCopierTest(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp())
        self.src = self.tmp / "src"
        (self.src / "assets").mkdir(parents=True)
        (self.src / "assets" / "fig.png").write_bytes(b"png")
        (self.src / "other").mkdir()
        (self.src / "other" / "fig.png").write_bytes(b"other")
        self.out = self.tmp / "out"

    def test_copies_under_the_lecture_id_and_returns_a_relative_path(self):
        copier = AssetCopier(self.out)
        rel = copier.copy("lec-a", self.src, "assets/fig.png")
        self.assertEqual(rel, "assets/lec-a/fig.png")
        self.assertEqual((self.out / rel).read_bytes(), b"png")

    def test_same_source_twice_reuses_one_copy(self):
        copier = AssetCopier(self.out)
        first = copier.copy("lec-a", self.src, "assets/fig.png")
        again = copier.copy("lec-a", self.src, "assets/fig.png")
        self.assertEqual(first, again)

    def test_basename_collision_within_a_lecture_is_suffixed(self):
        copier = AssetCopier(self.out)
        copier.copy("lec-a", self.src, "assets/fig.png")
        rel = copier.copy("lec-a", self.src, "other/fig.png")
        self.assertEqual(rel, "assets/lec-a/fig-2.png")
        self.assertEqual((self.out / rel).read_bytes(), b"other")

    def test_same_basename_in_two_lectures_does_not_collide(self):
        copier = AssetCopier(self.out)
        a = copier.copy("lec-a", self.src, "assets/fig.png")
        b = copier.copy("lec-b", self.src, "assets/fig.png")
        self.assertEqual((a, b), ("assets/lec-a/fig.png", "assets/lec-b/fig.png"))

    def test_missing_source_raises(self):
        with self.assertRaises(FileNotFoundError):
            AssetCopier(self.out).copy("lec-a", self.src, "assets/nope.png")


class PreambleTest(unittest.TestCase):
    def preamble(self, **kw):
        from lecturekit.book import BookModel
        from lecturekit.renderers.latex.preamble import document_preamble

        defaults = dict(
            title="T", author=None, subtitle=None, preface=None,
            lectures=(), asset_roots={},
        )
        return document_preamble(BookModel(**{**defaults, **kw}))

    def test_declares_ctexbook_and_the_packages_the_blocks_need(self):
        out = self.preamble()
        self.assertIn(r"\documentclass", out)
        self.assertIn("ctexbook", out)
        for package in ("graphicx", "booktabs", "listings", "hyperref", "tcolorbox"):
            self.assertIn(package, out)

    def test_defines_the_booktodo_macro(self):
        self.assertIn(r"\newcommand{\booktodo}", self.preamble())

    def test_title_and_author_are_escaped(self):
        out = self.preamble(title="100% Systems", author="A_B")
        self.assertIn(r"\title{100\% Systems}", out)
        self.assertIn(r"\author{A\_B}", out)

    def test_makefile_invokes_latexmk_with_xelatex(self):
        from lecturekit.renderers.latex.preamble import MAKEFILE

        self.assertIn("latexmk -xelatex", MAKEFILE)
        self.assertIn("\n\t", MAKEFILE)  # a real tab starts the recipe
