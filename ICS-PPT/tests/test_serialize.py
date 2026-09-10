import unittest

from lecturekit import Lecture
from lecturekit.serialize import lecture_to_dict


class SerializeTest(unittest.TestCase):
    def _lecture(self):
        lecture = Lecture(id="lec", title="L", subtitle="S", ratio="4:3")

        def body(p):
            p.title("Welcome")
            p.slide("hi")
            p.notes("teacher only")

        with lecture.section("Intro", id="intro") as s:
            s.page("welcome", body=body, tags=["t1"])
        return lecture.build()

    def test_serializes_full_tree_including_unrendered_blocks(self):
        data = lecture_to_dict(self._lecture())

        self.assertEqual(data["id"], "lec")
        self.assertEqual(data["ratio"], "4:3")
        section = data["children"][0]
        self.assertEqual(section["type"], "section")
        page = section["children"][0]
        self.assertEqual(page["type"], "page")
        self.assertEqual(page["tags"], ["t1"])
        # the full AST keeps notes, which the viewer would skip
        self.assertEqual([b["kind"] for b in page["blocks"]], ["slide", "notes"])

    def test_block_only_and_except_are_sorted_lists_or_none(self):
        lecture = Lecture(id="lec", title="L")

        def body(p):
            p.title("W")
            p.slide("x", only=["pptx", "viewer"])

        lecture.page("p", body=body)
        block = lecture_to_dict(lecture.build())["children"][0]["blocks"][0]
        self.assertEqual(block["only"], ["pptx", "viewer"])
        self.assertIsNone(block["except"])
        self.assertFalse(block["disabled"])

    def test_block_disabled_round_trips(self):
        lecture = Lecture(id="lec", title="L")

        def body(p):
            p.title("W")
            p.prose("draft").disable()

        lecture.page("p", body=body)
        block = lecture_to_dict(lecture.build())["children"][0]["blocks"][0]
        self.assertTrue(block["disabled"])

    def test_page_news_is_serialized(self):
        lecture = Lecture(id="lec", title="L")

        def body(p):
            p.title("W")
            p.slide("x")
            p.news(
                "Scaling Laws for Neural Language Models",
                url="https://arxiv.org/abs/2001.08361",
                source="Kaplan et al.",
                date="2020",
                kind="paper",
                why="Read the summary and Figure 1.",
                tags=["scaling-law", "paper"],
            )

        lecture.page("p", body=body)
        page = lecture_to_dict(lecture.build())["children"][0]

        self.assertEqual(
            page["news"],
            [
                {
                    "title": "Scaling Laws for Neural Language Models",
                    "url": "https://arxiv.org/abs/2001.08361",
                    "source": "Kaplan et al.",
                    "date": "2020",
                    "kind": "paper",
                    "why": "Read the summary and Figure 1.",
                    "tags": ["scaling-law", "paper"],
                    "image": None,
                    "archived_url": None,
                }
            ],
        )

    def test_page_gap_is_serialized(self):
        lecture = Lecture(id="lec", title="L")

        def body(p):
            p.title("W")
            p.gap("auto", min_px=6, max_px=30)
            p.slide("x")

        lecture.page("p", body=body)
        page = lecture_to_dict(lecture.build())["children"][0]

        self.assertEqual(
            page["gap"],
            {"mode": "auto", "min_px": 6, "max_px": 30},
        )
