from lecturekit.dsl import Lecture

lecture = Lecture(id="lec-b", title="Lecture B")


def only_page(p):
    p.title("B page")
    p.prose("B prose.")


lecture.page("b-page", body=only_page)
