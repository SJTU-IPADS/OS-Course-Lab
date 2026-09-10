import unittest

from lecturekit.model import Block, Page
from lecturekit.renderers.viewer import render_marp_page


class PresenterNotesTest(unittest.TestCase):
    """A ``notes`` block becomes a Marp presenter note: an HTML comment shown in
    the speaker view (press ``p``) but never on the audience-facing slide."""

    def test_notes_block_emits_presenter_comment(self):
        page = Page(id="p", title="T", blocks=[
            Block(kind="slide", content="visible to all"),
            Block(kind="notes", content="teacher only"),
        ])
        md = render_marp_page(page)
        self.assertIn("<!--\nteacher only\n-->", md)
        self.assertIn("visible to all", md)

    def test_page_without_notes_has_no_comment(self):
        page = Page(id="p", title="T", blocks=[Block(kind="slide", content="hi")])
        self.assertNotIn("<!--", render_marp_page(page))

    def test_notes_cannot_close_the_comment_early(self):
        page = Page(id="p", title="T", blocks=[
            Block(kind="notes", content="mind the --> here"),
        ])
        md = render_marp_page(page)
        self.assertNotIn("--> here", md)      # the raw closer is defused
        self.assertIn("--&gt; here", md)
        self.assertTrue(md.rstrip().endswith("-->"))

    def test_note_is_separated_from_preceding_html_block(self):
        """A note following an HTML-block block (here a footnoted slide, whose
        footnotes emit a trailing ``<aside>``) must be preceded by a blank line,
        or markdown-it folds the ``<!--`` into that HTML block and the note
        leaks onto the slide."""
        page = Page(id="p", title="T", blocks=[
            Block(kind="slide", content="body", footnotes=["src"]),
            Block(kind="notes", content="para one\n\npara two"),
        ])
        md = render_marp_page(page)
        self.assertIn("\n\n<!--\npara one\n\npara two\n-->", md)

    def test_notes_respects_except_override(self):
        page = Page(id="p", title="T", blocks=[
            Block(kind="notes", content="hidden here", except_={"viewer"}),
        ])
        self.assertNotIn("hidden here", render_marp_page(page))


if __name__ == "__main__":
    unittest.main()
