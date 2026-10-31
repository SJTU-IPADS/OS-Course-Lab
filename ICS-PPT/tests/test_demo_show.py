"""`p.demo(..., show=...)`: the command the slide writes, apart from the one that runs.

A command may end in a filter that only trims what the page prints (`| sed ...`).
The slide, the book and the drawer write the command without it; bash is given
the whole.
"""

import tempfile
import unittest
from pathlib import Path

from lecturekit import Lecture, demo, model
from lecturekit.renderers.latex.blocks import Ctx, emit_block
from lecturekit.renderers.viewer.blocks import render_block

RUN = "printf 'a\\n#b\\n' | sed '/^#/d'"
SHOW = "printf 'a\\n#b\\n'"


def blocks(command, **kw):
    lecture = Lecture(id="lec", title="L")

    def body(p):
        p.title("P")
        p.demo("n", command, **kw)

    lecture.page("p1", body=body)
    return lecture, next(
        b for b in lecture.build().children[0].blocks if b.kind == "demo"
    )


class DemoShowTest(unittest.TestCase):
    def test_the_slide_writes_the_shown_command(self):
        _lecture, block = blocks(RUN, show=SHOW)
        html = "".join(render_block(block))
        self.assertIn("printf", html)
        self.assertNotIn("sed", html)

    def test_the_id_is_the_commands_own(self):
        _lecture, block = blocks(RUN, show=SHOW)
        self.assertIn(demo.demo_id(RUN), "".join(render_block(block)))

    def test_the_table_runs_the_command_and_remembers_the_show(self):
        lecture, _block = blocks(RUN, show=SHOW)
        spec = demo.collect(lecture.build())[demo.demo_id(RUN)]
        self.assertEqual((spec.command, spec.show), (RUN, SHOW))

    def test_the_table_round_trips(self):
        lecture, _block = blocks(RUN, show=SHOW)
        with tempfile.TemporaryDirectory() as tmp:
            demo.write(lecture.build(), Path(tmp))
            spec = demo.read(Path(tmp))[demo.demo_id(RUN)]
        self.assertEqual((spec.command, spec.show), (RUN, SHOW))

    def test_a_demo_without_show_has_none(self):
        lecture, _block = blocks(RUN)
        self.assertIsNone(demo.collect(lecture.build())[demo.demo_id(RUN)].show)

    def test_the_book_prints_the_shown_command(self):
        _lecture, block = blocks(RUN, show=SHOW)
        ctx = Ctx(lecture_id="lec", page_id="p1", slide_width=1280, assets=None)
        tex = emit_block(block, ctx)
        self.assertIn("printf", tex)
        self.assertNotIn("sed", tex)

    def test_a_show_of_another_length_is_refused(self):
        with self.assertRaises(model.ValidationError):
            blocks("echo 1\necho 2", show="echo 1")


class DemoShowStreamTest(unittest.TestCase):
    def _events(self, command, show):
        with tempfile.TemporaryDirectory() as tmp:
            return list(demo.stream(command, cwd=Path(tmp), show=show))

    def test_the_drawer_announces_the_shown_line_and_bash_runs_the_real_one(self):
        events = self._events(RUN, SHOW)
        commands = [e.text for e in events if isinstance(e, demo.Command)]
        output = "".join(e.text for e in events if isinstance(e, demo.Chunk))
        self.assertEqual(commands, [f"$ {SHOW}"])
        self.assertEqual(output, "a\n")

    def test_each_line_is_announced_as_its_own_show(self):
        events = self._events("echo one | cat\necho two | cat", "echo one\necho two")
        commands = [e.text for e in events if isinstance(e, demo.Command)]
        self.assertEqual(commands, ["$ echo one", "$ echo two"])


if __name__ == "__main__":
    unittest.main()
