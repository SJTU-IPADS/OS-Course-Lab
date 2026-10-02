#!/usr/bin/env bash
# List the lectures to release, one PDF per line: the lecture's directory, then
# a tab and a language when that PDF is a translation.
#
#   scripts/released.sh [DIR...]      # searches DIR (default: .) for RELEASE files
#
# A directory is released when it holds a file named RELEASE beside its
# lecture.py. The file names the translation overlays to release as well, as
# words separated by white space (`en` for i18n/en.toml); `#` starts a comment.
# An empty RELEASE releases the lecture as written in Python, and only that.
#
# The output is what scripts/build-pdfs.sh reads.
set -euo pipefail

status=0
while IFS= read -r marker; do
  dir=${marker%/RELEASE}
  dir=${dir#./}
  if [ ! -f "$dir/lecture.py" ]; then
    echo "released: $marker has no lecture.py beside it" >&2
    status=1
    continue
  fi
  printf '%s\n' "$dir"
  sed 's/#.*//' "$marker" | tr -s '[:space:]' '\n' | while IFS= read -r lang; do
    [ -z "$lang" ] || printf '%s\t%s\n' "$dir" "$lang"
  done
done < <(
  find "${@:-.}" \( -name .git -o -name node_modules -o -name build \) -prune \
    -o -type f -name RELEASE -print | LC_ALL=C sort
)
exit "$status"
