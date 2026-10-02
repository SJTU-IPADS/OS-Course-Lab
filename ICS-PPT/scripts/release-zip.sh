#!/usr/bin/env bash
# Build the PDF of every released lecture and pack them into one zip — what CI
# attaches to a release, and what this machine makes the same way.
#
#   scripts/release-zip.sh dist/lectures-v2026.10.zip
#
# Run from the repository root. The zip holds one folder, named after the zip,
# with scripts/build-pdfs.sh's PDFs in it; the lectures are the ones
# scripts/released.sh finds. Any lecture that fails to build fails the whole
# run, and no zip is written: a release is all of them or none. When no RELEASE
# file marks a lecture there is nothing to do: the script says so, writes no
# zip and succeeds.
set -euo pipefail

zip=${1:?usage: release-zip.sh OUT.zip}
name=$(basename "$zip" .zip)
stage=$(mktemp -d)
trap 'rm -rf "$stage"' EXIT

list=$(scripts/released.sh)
if [ -z "$list" ]; then
  echo "release-zip: no RELEASE file marks a lecture; nothing to build" >&2
  exit 0
fi
count=$(printf '%s\n' "$list" | scripts/build-pdfs.sh "$stage/$name" | wc -l | tr -d ' ')

mkdir -p "$(dirname "$zip")"
rm -f "$zip"
python3 -m zipfile -c "$zip" "$stage/$name"
echo "release-zip: $zip ($count PDFs)" >&2
