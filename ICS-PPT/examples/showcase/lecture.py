"""A small runnable lecture, used as the source of the README screenshots.

It is a real deck, not a feature checklist: seven slides out of a crash-recovery
lecture, written the way a lecture is written. It happens to touch most of the
DSL — marker strokes, a highlight chip, marked pseudocode, an animation, an
architecture diagram, a table with a callout, a bridge — because that is what a
page of a systems lecture needs.

    PYTHONPATH=.. python3 -m lecturekit.cli view examples/showcase
    scripts/screenshots.sh          # re-shoots docs/images/ from this deck
"""

from lecturekit.dsl import Lecture

import pages

lecture = Lecture(id="showcase", title="Crash Recovery", subtitle="a lecturekit example")

lecture.cover(
    "Crash Recovery",
    author="lecturekit",
    time="one tree, three targets",
)

with lecture.section("The problem", id="problem") as s:
    s.page("atomicity", body=pages.atomicity)

with lecture.section("Shadow copy", id="shadow") as s:
    s.page("shadow-code", body=pages.shadow_code)
    s.page("shadow-run", body=pages.shadow_run)
    s.bridge("The rename is atomic. The copy is not free.\nWhat if we wrote the change instead of the file?")
    s.page("logging", body=pages.logging_)

with lecture.section("Where it lives", id="stack") as s:
    s.page("layers", body=pages.layers)
    s.page("compare", body=pages.compare)

lecture.close("conclusion", body=pages.conclusion)
