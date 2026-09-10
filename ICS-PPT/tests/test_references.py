import unittest

from lecturekit import model, references


def page(pid, citations):
    return model.Page(id=pid, title=pid, citations=tuple(citations))


class CollectTest(unittest.TestCase):
    def test_page_numbers_are_deck_positions(self):
        pages = [
            page("a", []),
            page("b", [model.Citation(title="T", key="t")]),
        ]
        entries = references.collect_citations(pages)
        self.assertEqual(len(entries), 1)
        self.assertEqual(entries[0].pages, (2,))

    def test_same_key_merges_across_pages(self):
        c = model.Citation(title="T", key="t")
        pages = [page("a", [c]), page("b", []), page("c", [c])]
        entries = references.collect_citations(pages)
        self.assertEqual(len(entries), 1)
        self.assertEqual(entries[0].pages, (1, 3))

    def test_dedup_falls_back_to_title_year(self):
        # No explicit key: title+year identifies the citation.
        one = model.Citation(title="Same Paper", year="2020")
        two = model.Citation(title="Same Paper", year="2020")
        entries = references.collect_citations([page("a", [one]), page("b", [two])])
        self.assertEqual(len(entries), 1)
        self.assertEqual(entries[0].pages, (1, 2))

    def test_distinct_citations_keep_order(self):
        pages = [
            page("a", [model.Citation(title="First", key="f")]),
            page("b", [model.Citation(title="Second", key="s")]),
        ]
        entries = references.collect_citations(pages)
        self.assertEqual([e.citation.title for e in entries], ["First", "Second"])

    def test_repeat_on_same_page_counts_once(self):
        c = model.Citation(title="T", key="t")
        entries = references.collect_citations([page("a", [c, c])])
        self.assertEqual(entries[0].pages, (1,))


if __name__ == "__main__":
    unittest.main()
