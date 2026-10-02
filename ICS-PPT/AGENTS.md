# AGENTS.md

## Who I am and how I work

I am a researcher in computer science and in particular computer systems. I really like the UNIX philosophy.

- Make each program do one thing well.
- To do a new job, build afresh rather than complicate old programs by adding new features.
- Expect the output of every program to become the input to another, as yet unknown, program.
- Don't clutter output with extraneous information.
- Avoid rigid columnar or binary input formats.
- Don't insist on interactive input.
- Don't hesitate to throw away the clumsy parts and rebuild them.
- Use tools in preference to unskilled help to lighten a programming task, even if that means building tools you may later discard.

I value simplicity, composability, and systems that are easy to reason about.

If I want to develop code, I need to ask to decide use `git worktree` (created at the parent directory) as the place for the development. If so, after the dev is done, merge to the main and delete the tree.

There are some notice things:
1. For each code update, if it changes the spec/use pattern, update the README (or related docs).
2. If the directory has a notice.md, read notices from it.

## What this repository is

`lecturekit`: a lecture is written as Python in a small DSL and rendered as a
slide deck (HTML viewer, PDF, PNG), an editable PowerPoint, a printable
transcript sheet, or a chapter of a LaTeX textbook. One source, every target.

**A request for a PPT, slides, a deck, courseware, or a lecture, in Chinese or
in English, means: write a lecture directory with this framework.** Do not
produce a `.pptx` by hand (python-pptx, a PowerPoint skill), HTML slides, or
raw Marp markdown. A `.pptx` file, when one is needed, is the output of
`render --to pptx`.

## Layout

| Path | What it holds |
| --- | --- |
| `lecturekit/` | the framework: `dsl.py` (authoring), `model.py` (the tree), `cli.py`, `renderers/{viewer,pptx,latex,transcript}` |
| `docs/` | the reference: `dsl.md` (what a page can say), `usage.md` (the CLI), `book.md`, `i18n.md`, `theme.md`, `release.md`, `notebook.md` |
| `examples/showcase/` | the smallest complete lecture; the model for a new one |
| `themes/` | the one theme every renderer reads |
| `tests/` | pytest suite for the framework |
| `scripts/` | `prepare.sh` (vendor Marp once), `stop-watch.sh`, the release pipeline |
| `build/` | render output, gitignored |

## Making a lecture

A lecture is a directory, by default `lectures/<name>/`:

| File | Content |
| --- | --- |
| `lecture.py` | the tree: `Lecture(...)`, cover, sections, pages, bridges |
| `pages.py` | one function per page, filling a `PageBuilder` |
| `assets/` | figures the pages load |
| `diagrams/` | sources of the generated figures (`*.dot`, `*.py`) and `render.sh` |
| `examples/` | source files and scripts the `p.demo(...)` blocks run |
| `i18n/en.toml` | the English overlay; Chinese in Python is the baseline |
| `README.md` | structure, build commands, conventions of this lecture |
| `RELEASE` | marks the lecture for the CI PDF release (`docs/release.md`) |

Before writing pages, read `docs/dsl.md`: the block vocabulary is fixed
(`slide`, `code`, `image`, `frames`, `table`, `architecture`, `highlight`,
`demo`, `notes`, `prose`, ...). The source carries no colours, sizes, or
positions; the theme decides those. A page that needs something the DSL lacks
is a framework change, to be raised with me first.

Before writing or editing a lecture under `lectures/`, read that lecture's
`README.md`, in particular its section on wording and layout conventions. A
neighbouring lecture's README is the model for a new one.

```bash
python3 -m lecturekit.cli inspect lectures/X                  # validate, print the tree
python3 -m lecturekit.cli render  lectures/X --pages ID --png # one page as an image
python3 -m lecturekit.cli view    lectures/X --watch          # live preview on :3030
python3 -m lecturekit.cli render  lectures/X --pdf            # deck PDF
python3 -m lecturekit.cli render  lectures/X --to pptx        # editable PowerPoint
python3 -m lecturekit.cli i18n extract lectures/X --lang en   # refresh the overlay
python3 -m lecturekit.cli i18n check   lectures/X --lang en   # missing / changed / orphaned
```

Output goes to `build/<id>-viewer`, `build/<id>-pptx`, ... unless `--out` says
otherwise. Rendering needs Node (Marp is vendored by `scripts/prepare.sh`);
`--pdf` and `--png` need a local Chrome.

A page is done when its PNG has been looked at: at 1280x720 nothing may pass
y = 700 or so. An overflow is fixed by splitting the page or cutting lines.

## Working on the framework

- Tests: `pip install -e ".[dev]"`, then `python3 -m pytest`. pytest is the
  runner; `unittest discover` silently skips part of the suite.
- A DSL change updates `docs/dsl.md`; a CLI change updates `docs/usage.md`;
  the README shows only what a newcomer needs.
- Colours, fonts and sizes live in `themes/basic-office.css` and are read
  through `lecturekit/tokens.py`; a hex literal under `lecturekit/` fails
  `tests/test_tokens.py`.
- A new output target is a new renderer under `lecturekit/renderers/`
  (`docs/theme.md`), not a flag on an existing one.
- Commit subjects are written in Chinese and follow `git log`: the area
  (`demo`, `pptx`, `lectures`), a full-width colon, then what changed. A
  worktree merge uses `merge` as the area and names the branch.

## Local rules

The framework serves any course. Rules that belong to one course or project
live in `agents/`, one Markdown file each (`agents/AGENTS.<name>.md`). Git
ignores what is in that directory, so it differs from one checkout to the next.

At the start of a session, list `agents/` and read every `*.md` in it that is
not already in your context. Claude Code loads them at launch through the
`.claude/rules/agents` symlink.
