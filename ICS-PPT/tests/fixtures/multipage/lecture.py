from lecturekit.dsl import Lecture
import content

# Deck order (flattened): alpha, beta, gamma, delta
lecture = Lecture(id="multi", title="Multi", subtitle="Sub")
with lecture.section("One", id="one") as s:
    s.page("alpha", body=content.alpha)
    s.page("beta", body=content.beta)
with lecture.section("Two", id="two") as s:
    s.page("gamma", body=content.gamma)
    s.page("delta", body=content.delta)
