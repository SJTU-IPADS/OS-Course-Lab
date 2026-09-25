import unittest

from lecturekit import Lecture, model


def build_page(body):
    lecture = Lecture(id="lec", title="L")
    lecture.page("p", body=body)
    return model.flatten_pages(lecture.build().children)[0]


class CiteDslTest(unittest.TestCase):
    def test_structured_fields(self):
        def body(p):
            p.title("T")
            p.slide("body")
            p.cite(title="Attention Is All You Need", author="Vaswani et al.",
                   year="2017", venue="NeurIPS", url="https://x.org")

        page = build_page(body)
        self.assertEqual(len(page.citations), 1)
        c = page.citations[0]
        self.assertEqual(c.title, "Attention Is All You Need")
        self.assertEqual(c.author, "Vaswani et al.")
        self.assertEqual(c.venue, "NeurIPS")

    def test_cite_is_not_a_slide_block(self):
        def body(p):
            p.title("T")
            p.slide("visible")
            p.cite(title="Hidden Paper")

        page = build_page(body)
        self.assertEqual([b.kind for b in page.blocks], ["slide"])

    def test_bibtex_string_is_parsed(self):
        def body(p):
            p.title("T")
            p.slide("body")
            p.cite("@article{vaswani2017, title={Attention Is All You Need}, "
                   "author={Vaswani, Ashish and Shazeer, Noam}, year={2017}}")

        c = build_page(body).citations[0]
        self.assertEqual(c.title, "Attention Is All You Need")
        self.assertEqual(c.author, "Vaswani et al.")
        self.assertEqual(c.year, "2017")
        self.assertEqual(c.key, "vaswani2017")

    def test_explicit_fields_override_bibtex(self):
        def body(p):
            p.title("T")
            p.slide("body")
            p.cite("@article{k, title={Long Machine Title}, author={A and B}}",
                   author="Hand Written")

        c = build_page(body).citations[0]
        self.assertEqual(c.title, "Long Machine Title")
        self.assertEqual(c.author, "Hand Written")

    def test_missing_title_is_error(self):
        def body(p):
            p.title("T")
            p.cite(author="Nobody")

        with self.assertRaises(model.ValidationError):
            build_page(body)

    def test_unparseable_bibtex_without_title_is_error(self):
        def body(p):
            p.title("T")
            p.cite("not a bibtex entry")

        with self.assertRaises(model.ValidationError):
            build_page(body)


if __name__ == "__main__":
    unittest.main()
