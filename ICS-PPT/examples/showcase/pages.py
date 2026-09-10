"""The page bodies. One function per page, each taking a `PageBuilder`."""

TRANSFER = """transfer(bank, a, b, amt):
    records = mmap(bank, ...)
    records[a] = records[a] - amt
    records[b] = records[b] + amt
    msync(records, ...)"""

SHADOW = """transfer(bank, a, b, amt):
    fcopy(bank, bank_temp)
    records = mmap(bank_temp, ...)
    records[a] = records[a] - amt
    records[b] = records[b] + amt
    fsync(bank_temp, ...)
    rename(bank_temp, bank)"""

LOGGING = """transfer(bank, a, b, amt):
    append(log, <begin, txid>)
    append(log, <write, a, records[a] - amt>)
    append(log, <write, b, records[b] + amt>)
    append(log, <commit, txid>)
    fsync(log)
    apply(log, bank)"""


def atomicity(p):
    p.title("Two writes, one crash")
    p.slide("""
A transfer is ==two writes==, and a crash can land ==orange:between them==.
- the debit reaches the disk
- the machine loses power
- the credit never happens
""")
    p.code("pseudo", TRANSFER, mark=[3, 4])
    p.highlight("The money is gone.", tone="orange")
    p.prose("""
A bank transfer is one operation to the person making it and two writes to the
file holding the balances. Nothing about the hardware ties those two writes
together: the disk takes them one at a time, and the power may fail between
them. What the file then holds is a state the program never intended and cannot
name — the debit applied, the credit not — and the money is simply gone.

This is the whole difficulty of crash recovery, and it does not go away by
writing more carefully. It goes away by arranging that the durable state can
only ever be *before* or *after*, never in between.
""")


def shadow_code(p):
    p.title("Shadow copy: write beside, then swap")
    p.slide("""
Never modify the file the reader is reading.
- copy it, edit the copy, and put the copy in its place
- the two ==marked== lines are the whole idea
""")
    p.code("pseudo", SHADOW, mark=[2, 7])
    p.slide("`rename` is atomic: after the crash the reader sees the old file or the new one.")
    p.prose("""
Shadow copy buys atomicity from the file system rather than building it. The
update never touches the file a reader may be holding; it lands in a copy, and
the copy takes the original's name in one indivisible step. A crash before the
rename leaves the original untouched, and the half-written copy is garbage
nobody has a name for; a crash after it leaves the finished file. There is no
third outcome, which is exactly what the previous page could not promise.

The price is in the first line: the copy is proportional to the *file*, while
the change was proportional to two records.
""")
    p.cite(
        title="Principles of Computer System Design: An Introduction",
        author="Saltzer & Kaashoek",
        year="2009",
        venue="Morgan Kaufmann, §9.3",
    )


def shadow_run(p):
    p.title("Shadow copy: write beside, then swap")
    p.slide("The reader's file is never in an intermediate state — only the shadow is.")
    p.frames(
        "assets/shadow-1.svg",
        "assets/shadow-2.svg",
        "assets/shadow-3.svg",
        width_px=880,
        caption="copy · edit · rename",
    ).footnote("A crash anywhere in the first two frames costs the copy, not the bank file.")
    p.prose("""
Read the three states in order. The bank file is copied; the copy is edited
while the original still answers every reader; the rename swaps them. Only the
last step is visible to anyone else, and it is a single file-system operation.
""")


def logging_(p):
    p.title("Logging: write the change, not the file")
    p.slide("""
Write down what you are ==green:about to do==, then do it.
- the log is append-only, so a torn record is at the end and is discarded
- ==green:commit== is the moment the transfer becomes real
""")
    p.code("pseudo", LOGGING, mark=[5], tone="green")
    p.sidenote(
        "Why this wins",
        "The log record is the size of the change; the shadow copy was the size of "
        "the whole file. That is the difference between a transfer costing two "
        "records and a transfer costing the bank.",
    )
    p.prose("""
Logging inverts the shadow copy. Instead of building the new state somewhere
safe and swapping it in, it writes the intention down first: the records that
are about to change, and a marker saying the set of them is complete. Only then
does it touch the data. Recovery replays the log, redoing a transaction whose
`commit` record is on disk and discarding one without it; the torn record a
crash leaves behind can only ever be the last one, where it is harmless.

The cost now scales with the change rather than with the file, which is why every
database on your machine is written this way.
""")


def layers(p):
    p.title("Where the guarantee lives")
    arch = p.architecture(caption="each layer promises less than the one above", flow="down")
    arch.layer("Application", ["transfer()", "SQL", ...])
    arch.layer("Recovery", ["log", "commit", "replay"])
    arch.layer("File system", ["rename", "fsync", "mmap", ...])
    arch.layer("Disk", ["sector write"])
    arch.footnote("The disk promises one sector; everything above it is built from that.")
    p.slide("The disk gives you one atomic sector. Every larger promise is software.")
    p.prose("""
It is worth naming where each guarantee comes from. The hardware offers exactly
one: a single sector write either happens or does not. The file system turns
that into an atomic rename. The recovery layer turns the atomic rename — or the
append-only log — into an atomic transaction over arbitrarily many records. And
the application spends that transaction on a meaning: a transfer.
""")


def compare(p):
    p.title("Two answers to the same question")
    p.gap("auto")
    p.table(
        headers=["", "Shadow copy", "Logging"],
        rows=[
            ["cost of one update", "the whole file", "the changed records"],
            ["cost of recovery", "nothing to do", "replay the tail"],
            ["concurrent writers", "one at a time", "many, interleaved"],
            ["where it is used", "editors, `git`", "databases, file systems"],
        ],
        align=["left", "left", "left"],
    ).annotate("this row is why\ndatabases log", at="center", dx=150, dy=-95)
    p.slide("Same guarantee, opposite bill. Pick by the shape of the update, not by taste.")
    p.prose("""
Neither technique is better in the abstract; they charge for different things. A
shadow copy is unbeatable when the update rewrites most of the object anyway and
readers must never block — which is why editors and version-control systems use
it. Logging wins the moment updates are small relative to the object, or several
writers need to be in flight at once.
""")


def conclusion(p):
    p.title("What I want you to take away")
    p.highlight("""Atomicity is not a property of the disk.
It is something you build.""")
    p.slide("""
Two techniques, one idea: make the durable state jump, never crawl.
- shadow copy buys the jump from `rename`
- logging buys it from an append-only file and one `commit` record
""")
    p.prose("""
Both designs on this page do the same thing: they arrange for the durable state
to move from *before* to *after* in one step that the hardware already
guarantees, and they keep everything expensive on the other side of that step.
Once you see that shape you will find it everywhere — in `git`'s object store,
in a file system's journal, in the way a deployment swaps a symlink.
""")
