#!/bin/sh
# Render every diagram source in this directory into ../assets/.
#   *.py             -> hand-laid SVG, writes its own output
# Then check_bounds.py verifies no label ran off its own canvas.
# Run from anywhere; paths are resolved against this script.
set -eu
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

for py in "$here"/*.py; do
    [ -e "$py" ] || continue
    case $(basename "$py") in svgkit.py|check_bounds.py|timegrid.py|llm_pipeline.py) continue ;; esac
    python3 "$py"
done

python3 "$here"/check_bounds.py
