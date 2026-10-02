#!/usr/bin/env bash
# Build one PDF per line of standard input into OUT, and print each one's path.
#
#   scripts/released.sh | scripts/build-pdfs.sh OUT
#
# A line is a lecture directory, or a directory, a tab and an i18n language, as
# scripts/released.sh prints them. The PDF is named after the directory, not the
# title, so the names are stable and plain ASCII: lectures/2-data gives
# 2-data.pdf, and with `en` 2-data.en.pdf. The render's own chatter goes to
# standard error. A lecture that fails is reported and the rest still build;
# the exit status says whether any failed.
set -uo pipefail

out=${1:?usage: build-pdfs.sh OUT < list}
mkdir -p "$out"
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

status=0
while IFS=$'\t' read -r dir lang; do
  [ -n "$dir" ] || continue
  name=$(basename "$dir")${lang:+.$lang}
  if [ -e "$out/$name.pdf" ]; then
    echo "build-pdfs: two lectures would both be $out/$name.pdf" >&2
    status=1
    continue
  fi
  bundle=$work/$name
  echo "build-pdfs: $dir${lang:+ ($lang)}" >&2
  if python3 -m lecturekit.cli render "$dir" ${lang:+--lang "$lang"} \
      --out "$bundle" --pdf >&2; then
    pdf=$(find "$bundle" -maxdepth 1 -name '*.pdf' | head -n 1)
    if [ -n "$pdf" ] && mv "$pdf" "$out/$name.pdf"; then
      printf '%s\n' "$out/$name.pdf"
      continue
    fi
  fi
  echo "build-pdfs: $dir${lang:+ ($lang)} failed" >&2
  status=1
done
exit "$status"
