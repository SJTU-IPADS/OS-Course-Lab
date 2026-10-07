#!/bin/sh
# Render every diagram source in this directory into ../assets/.
#   *.dot            -> graphviz
#   *.py             -> hand-laid SVG, writes its own output
# Then check_bounds.py verifies no label ran off its own canvas.
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

# svgkit.py is a module; check_bounds.py and margins.py inspect the output
for py in "$here"/*.py; do
    [ -e "$py" ] || continue
    case $(basename "$py") in svgkit.py|check_bounds.py|margins.py) continue ;; esac
    python3 "$py"
done

python3 "$here"/check_bounds.py
