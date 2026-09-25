#!/bin/sh
# Render every diagram source in this directory into ../assets/.
#   *.dot            -> graphviz
#   *.py             -> hand-laid SVG, writes its own output
# Run from anywhere; paths are resolved against this script.
set -eu
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
assets=$here/../assets

for dot in "$here"/*.dot; do
    [ -e "$dot" ] || continue
    name=$(basename "$dot" .dot)
    dot -Tsvg "$dot" -o "$assets/$name.svg"
    echo "$assets/$name.svg"
done

for py in "$here"/*.py; do
    [ -e "$py" ] || continue
    python3 "$py"
done
