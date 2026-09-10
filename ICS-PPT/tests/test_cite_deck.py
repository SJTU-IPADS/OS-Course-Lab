import unittest

from lecturekit import Lecture, model
from lecturekit.renderers.viewer import build_data, build_marp_markdown
from lecturekit.renderers.viewer import with_references_page


def lecture_with_cites():
    lecture = Lecture(id="lec", title="L")

    def p1(p):
        p.title("Intro")
        p.slide("hi")
        p.cite(title="Attention Is All You Need", author="Vaswani et al.",
               year="2017", venue="NeurIPS", url="https://x.org", key="v17")

    def p2(p):
        p.title("More")
        p.slide("more")
        p.cite(title="Attention Is All You Need", key="v17")  # same ref again
        p.cite(title="Scaling Laws", author="Kaplan et al.", year="2020", key="k20")

    lecture.page("p1", body=p1)
    lecture.page("p2", body=p2)
    return lecture.build()


class ReferencesPageTest(unittest.TestCase):
    def test_page_appended_when_citations_exist(self):
        model_lec = with_references_page(lecture_with_cites())
        pages = model.flatten_pages(model_lec.children)
        self.assertEqual(pages[-1].id, "__references__")
        self.assertEqual(pages[-1].title, "参考文献")

    def test_no_page_when_no_citations(self):
        lecture = Lecture(id="lec", title="L")
        lecture.page("p", body=lambda p: (p.title("T"), p.slide("x")))
        model_lec = with_references_page(lecture.build())
        self.assertNotIn("__references__", [p.id for p in model.flatten_pages(model_lec.children)])

    def test_markdown_lists_entries_with_page_backrefs(self):
        md = build_marp_markdown(lecture_with_cites())
        self.assertIn("参考文献", md)
        self.assertIn("Attention Is All You Need", md)
        self.assertIn("Scaling Laws", md)
        # First ref cited on pages 1 and 2, second only on page 2.
        self.assertIn("P1, P2", md)
        self.assertIn("P2", md)

    def test_markdown_marks_references_page_for_small_type(self):
        md = build_marp_markdown(lecture_with_cites())
        self.assertIn("<!-- _class: lk-references -->", md)

    def test_build_data_includes_references_page(self):
        data = build_data(lecture_with_cites())
        self.assertEqual(data["pages"][-1]["id"], "__references__")

    def test_reserved_id_conflict_raises(self):
        lecture = Lecture(id="lec", title="L")

        def body(p):
            p.title("T")
            p.slide("x")
            p.cite(title="Paper", key="k")

        lecture.page("__references__", body=body)
        with self.assertRaises(model.ValidationError):
            with_references_page(lecture.build())


if __name__ == "__main__":
    unittest.main()
