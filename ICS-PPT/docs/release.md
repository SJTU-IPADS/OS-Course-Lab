# Releasing lectures as PDFs

CI builds the PDF of every lecture marked for release on every push, and on a
tag push it publishes them as one zip. GitHub (`.github/workflows/lectures.yml`)
and GitLab (`.gitlab-ci.yml`) do the same thing. Both configurations are thin:
the work is done by three scripts in `scripts/`, which run the same way on your
own machine.

## Marking a lecture

A lecture is released when its directory holds a file named `RELEASE` beside
its `lecture.py`:

```
lectures/2-data/
├── lecture.py
├── pages.py
├── RELEASE          ← this lecture goes into the release
└── i18n/en.toml
```

An empty `RELEASE` (or one holding only comments) releases the lecture as
written in Python, and only that. To release a translation as well, name its
[i18n](i18n.md) language in the file; words are separated by white space, and
`#` starts a comment:

```
# CI releases this lecture as a PDF (docs/release.md).
en
```

Deleting the file takes the lecture out of the next release. Nothing else lists
the lectures, so a new one is released by adding the file to it, and a
directory without `lecture.py` beside the file is an error, not a skip.

## What a release holds

One PDF per lecture and language, named after the directory rather than the
title, so the names are stable and plain ASCII: `lectures/2-data` gives
`2-data.pdf`, and its English translation `2-data.en.pdf`. Each is the deck's
PDF: the outline page, then one page per slide (as `render --pdf` makes it; see
[usage.md](usage.md)).

The zip is `lectures-<version>.zip`, holding a single folder of the same name
with the PDFs in it. The version is the tag on a tag push and the short commit
hash otherwise.

## The pipeline

```bash
scripts/released.sh                            # the lectures to release, one PDF per line
scripts/released.sh | scripts/build-pdfs.sh OUT   # build them into OUT, print each path
scripts/release-zip.sh dist/lectures-test.zip  # the two above, then zip; what CI runs
```

- `released.sh [DIR…]` finds the `RELEASE` files under `DIR` (default `.`,
  skipping `.git`, `node_modules` and `build`) and prints the lecture's
  directory, or the directory, a tab and a language for a translation.
- `build-pdfs.sh OUT` reads those lines, renders each with
  `python3 -m lecturekit.cli render DIR [--lang L] --pdf`, and moves the PDF to
  `OUT/<name>.pdf`. A lecture that fails is reported and the rest still build;
  the exit status says whether any failed.
- `release-zip.sh OUT.zip` runs the two from the repository root and writes the
  zip only if every lecture built: a release is all of them or none. When no
  `RELEASE` file marks a lecture, it says so on standard error, writes no zip
  and exits with status 0.

Run on your own machine, they need what `render --pdf` needs: marp-cli
(`scripts/prepare.sh`) and a Chrome or Chromium.

## CI

Every push runs one build job in `python:3.12-slim-trixie`, a Debian image with
Python. `scripts/ci-setup.sh` installs Chromium, Node and npm, the DejaVu and
Cascadia Code fonts, lecturekit (`pip install -e .`) and marp-cli
(`scripts/prepare.sh`); `scripts/release-zip.sh` then builds the zip, which the
job keeps as an artifact for a week. In 2026-09 the fourteen lectures took about
a minute and a quarter to render after setup and made a 35 MB zip.

A container runs as root, where Chrome refuses to start without
`--no-sandbox`; lecturekit adds that flag when it runs as root, and so does
marp-cli.

A tag push also publishes a release named after the tag:

```bash
git tag v2026.10
git push origin v2026.10
```

Use a tag made of letters, digits, `.`, `_`, `-` and `+`. GitLab's package
registry takes nothing else as a version, and the zip is named after it.

A repository with no `RELEASE` file has nothing to build, and CI treats a push
to it as done: no setup, no artifact, and no release for a tag. On GitHub the
build job lists the marked lectures with `scripts/released.sh` first and skips
its remaining steps when the list is empty; the release job runs only when the
list was not. On GitLab both jobs carry `rules: exists: ["**/RELEASE"]`, so
neither is created and no pipeline runs.

### GitHub

The release job attaches the zip to a GitHub release with `gh release create`,
authenticated by the workflow's own token, for which the job asks `contents:
write`. A branch push's zip is on the workflow run's page, under *Artifacts*.
Re-running a tag's workflow fails at the release step while that release
exists; delete the release first.

### GitLab

The build job runs on a runner with the Docker executor. On a tag push it
uploads the zip to the project's generic package registry, and the release job
publishes a release whose asset link points there. The zip goes to the registry
rather than being linked as a job artifact because artifacts expire, and because
a self-managed GitLab caps them at 100 MB by default.

What a private GitLab has to provide:

- **Package registry** enabled for the project (*Settings → General →
  Visibility, project features, permissions*).
- **Network** from the runners to Docker Hub for the image, a Debian mirror for
  `apt-get`, PyPI for lecturekit's dependencies and the npm registry for
  marp-cli. Where Docker Hub is out of reach, set the CI/CD variable
  `LECTURE_IMAGE` to a mirror of `python:3.12-slim-trixie`, or to any Debian
  trixie image with Python 3.12 or later. The release job uses
  `registry.gitlab.com/gitlab-org/release-cli`, which likewise may need
  mirroring.
- **Runner tags**, if the instance's runners only take tagged jobs: add a
  `tags:` list to both jobs in `.gitlab-ci.yml`.
