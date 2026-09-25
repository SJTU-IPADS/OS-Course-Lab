import io
import tempfile
import unittest
from contextlib import redirect_stdout
from pathlib import Path
from unittest.mock import patch

from lecturekit.cli import main

BOOK = "tests/fixtures/book"


class BookCliTest(unittest.TestCase):
    def test_book_writes_a_latex_tree(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertEqual(main(["book", BOOK, "--out", tmp]), 0)
            self.assertTrue(Path(tmp, "book.tex").exists())
            self.assertTrue(Path(tmp, "chapters", "lec-a.tex").exists())
            self.assertTrue(Path(tmp, "Makefile").exists())

    def test_stats_prints_coverage_and_renders_nothing(self):
        with tempfile.TemporaryDirectory() as tmp:
            buf = io.StringIO()
            with redirect_stdout(buf):
                self.assertEqual(main(["book", BOOK, "--stats", "--out", tmp]), 0)
            out = buf.getvalue()
            self.assertIn("lec-a", out)
            self.assertIn("1/2", out)
            self.assertIn("total", out)
            self.assertFalse(Path(tmp, "book.tex").exists())

    def test_lectures_selects_a_subset(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertEqual(
                main(["book", BOOK, "--out", tmp, "--lectures", "lec-b"]), 0
            )
            self.assertTrue(Path(tmp, "chapters", "lec-b.tex").exists())
            self.assertFalse(Path(tmp, "chapters", "lec-a.tex").exists())

    def test_unknown_lecture_id_is_an_error(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertEqual(
                main(["book", BOOK, "--out", tmp, "--lectures", "nope"]), 1
            )

    def test_compile_runs_latexmk_in_the_output_dir(self):
        with tempfile.TemporaryDirectory() as tmp:
            with patch("lecturekit.cli.subprocess.run") as run:
                self.assertEqual(main(["book", BOOK, "--out", tmp, "--compile"]), 0)
            run.assert_called_once()
            command = run.call_args[0][0]
            self.assertIn("latexmk", command)
            self.assertIn("-xelatex", command)
            self.assertEqual(run.call_args[1]["cwd"], Path(tmp))

    def test_no_compile_never_shells_out(self):
        with tempfile.TemporaryDirectory() as tmp:
            with patch("lecturekit.cli.subprocess.run") as run:
                main(["book", BOOK, "--out", tmp])
            run.assert_not_called()

    def test_missing_book_py_exits_nonzero(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertEqual(main(["book", tmp, "--out", tmp]), 1)
