"""The same lecture, read as a book: `lecturekit book examples --compile`.

One chapter per lecture; the deck's `p.slide(...)` never reaches it, and the
`p.prose(...)` the deck never shows is the body.
"""

from lecturekit.book import Book

book = Book(title="Crash Recovery", author="lecturekit")
book.preface(
    "One page tree, two bodies. The slides in examples/showcase/pages.py are "
    "what a class sees; the prose beside them is what a reader sees."
)

book.lecture("showcase")
