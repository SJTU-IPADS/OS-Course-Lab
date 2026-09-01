from lecturekit.dsl import Lecture
import pages

lecture = Lecture(id="sample", title="Sample Lecture", subtitle="Sub")
with lecture.section("Intro", id="intro") as s:
    s.page("welcome", body=pages.welcome)
