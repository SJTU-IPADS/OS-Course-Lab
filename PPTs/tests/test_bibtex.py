import unittest

from lecturekit import bibtex


class ParseTest(unittest.TestCase):
    def test_basic_fields(self):
        fields = bibtex.parse(
            "@article{vaswani2017, title={Attention Is All You Need}, "
            "author={Ashish Vaswani}, year={2017}}"
        )
        self.assertEqual(fields["title"], "Attention Is All You Need")
        self.assertEqual(fields["year"], "2017")
        self.assertEqual(fields["key"], "vaswani2017")

    def test_quoted_values(self):
        fields = bibtex.parse('@misc{k, title = "A Title", year = "2020"}')
        self.assertEqual(fields["title"], "A Title")
        self.assertEqual(fields["year"], "2020")

    def test_bare_numeric_value(self):
        fields = bibtex.parse("@misc{k, title={T}, year = 2020}")
        self.assertEqual(fields["year"], "2020")

    def test_nested_braces_are_balanced(self):
        # The top-level comma splitter must not break inside nested braces.
        fields = bibtex.parse(
            "@inproceedings{k, title={{BERT}: Pre-training of Deep Models}, year={2019}}"
        )
        self.assertEqual(fields["title"], "BERT: Pre-training of Deep Models")
        self.assertEqual(fields["year"], "2019")

    def test_venue_from_journal(self):
        fields = bibtex.parse("@article{k, title={T}, journal={NeurIPS}}")
        self.assertEqual(fields["venue"], "NeurIPS")

    def test_venue_from_booktitle(self):
        fields = bibtex.parse("@inproceedings{k, title={T}, booktitle={OSDI}}")
        self.assertEqual(fields["venue"], "OSDI")

    def test_url_field(self):
        fields = bibtex.parse("@misc{k, title={T}, url={https://x.org/a}}")
        self.assertEqual(fields["url"], "https://x.org/a")

    def test_missing_fields_absent(self):
        fields = bibtex.parse("@misc{k, title={T}}")
        self.assertNotIn("author", fields)
        self.assertNotIn("year", fields)

    def test_malformed_returns_empty(self):
        # Not a bibtex entry at all.
        self.assertEqual(bibtex.parse("just a plain title"), {})


class AuthorBeautifyTest(unittest.TestCase):
    def test_single_author_reads_first_last(self):
        fields = bibtex.parse("@misc{k, title={T}, author={Vaswani, Ashish}}")
        self.assertEqual(fields["author"], "Ashish Vaswani")

    def test_two_authors_use_et_al(self):
        fields = bibtex.parse(
            "@misc{k, title={T}, author={Vaswani, Ashish and Shazeer, Noam}}"
        )
        self.assertEqual(fields["author"], "Vaswani et al.")

    def test_others_uses_et_al(self):
        fields = bibtex.parse(
            "@misc{k, title={T}, author={Ashish Vaswani and others}}"
        )
        self.assertEqual(fields["author"], "Vaswani et al.")


class EscapeTest(unittest.TestCase):
    def test_ampersand(self):
        fields = bibtex.parse(r"@misc{k, title={Cats \& Dogs}}")
        self.assertEqual(fields["title"], "Cats & Dogs")

    def test_dashes(self):
        fields = bibtex.parse("@misc{k, title={pages 1--10}}")
        self.assertEqual(fields["title"], "pages 1–10")

    def test_accent(self):
        fields = bibtex.parse(r'@misc{k, author={Sch{\"o}lkopf}}')
        self.assertEqual(fields["author"], "Schölkopf")


if __name__ == "__main__":
    unittest.main()
