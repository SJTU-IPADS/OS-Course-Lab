import unittest

from lecturekit.model import (
    Block,
    Lecture,
    Page,
    Section,
    ValidationError,
    select_pages,
)


def _page(pid):
    return Page(id=pid, title=pid.title(), blocks=[Block(kind="slide", content="x")])


def _lecture():
    # Deck order (flattened, depth-first): a, b, c, d
    return Lecture(
        id="lec",
        title="Lec",
        children=[
            Section(id="one", title="One", children=[_page("a"), _page("b")]),
            Section(id="two", title="Two", children=[_page("c"), _page("d")]),
        ],
    )


def _page_ids(lecture):
    ids = []

    def walk(nodes):
        for node in nodes:
            if isinstance(node, Page):
                ids.append(node.id)
            else:
                walk(node.children)

    walk(lecture.children)
    return ids


class SelectPagesTest(unittest.TestCase):
    def test_select_by_id_prunes_other_pages(self):
        out = select_pages(_lecture(), "c")
        self.assertEqual(_page_ids(out), ["c"])

    def test_empty_sections_are_dropped(self):
        out = select_pages(_lecture(), "a")
        # Only the section that still holds a selected page survives.
        self.assertEqual([s.id for s in out.children], ["one"])

    def test_section_with_a_kept_page_survives(self):
        out = select_pages(_lecture(), "d")
        self.assertEqual([s.id for s in out.children], ["two"])

    def test_select_by_one_based_deck_index(self):
        out = select_pages(_lecture(), "3")  # third page in deck order == c
        self.assertEqual(_page_ids(out), ["c"])

    def test_select_by_index_range_inclusive(self):
        out = select_pages(_lecture(), "2-3")
        self.assertEqual(_page_ids(out), ["b", "c"])

    def test_mixed_comma_list_unions_selectors(self):
        out = select_pages(_lecture(), "1, d")
        self.assertEqual(_page_ids(out), ["a", "d"])

    def test_order_and_nesting_are_preserved(self):
        out = select_pages(_lecture(), "d,a")  # order in spec must not matter
        self.assertEqual(_page_ids(out), ["a", "d"])

    def test_unknown_id_raises(self):
        with self.assertRaises(ValidationError):
            select_pages(_lecture(), "nope")

    def test_index_out_of_range_raises(self):
        with self.assertRaises(ValidationError):
            select_pages(_lecture(), "9")

    def test_descending_range_raises(self):
        with self.assertRaises(ValidationError):
            select_pages(_lecture(), "3-1")

    def test_empty_spec_raises(self):
        with self.assertRaises(ValidationError):
            select_pages(_lecture(), "  ")

    def test_returns_a_pruned_copy_not_the_original(self):
        original = _lecture()
        select_pages(original, "a")
        self.assertEqual(_page_ids(original), ["a", "b", "c", "d"])


if __name__ == "__main__":
    unittest.main()
