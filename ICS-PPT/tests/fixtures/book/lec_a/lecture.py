from lecturekit.dsl import Lecture

lecture = Lecture(id="lec-a", title="Lecture A")


def intro(p):
    p.title("A page with prose")
    p.slide("bullet only on slides")
    p.prose("A paragraph of **book** text.")


def bare(p):
    p.title("A page without prose")
    p.slide("bullet")


with lecture.section("First Section") as s:
    s.page("a-intro", body=intro)
    s.page("a-bare", body=bare)
