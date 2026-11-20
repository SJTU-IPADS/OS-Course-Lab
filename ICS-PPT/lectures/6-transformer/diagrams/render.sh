#!/bin/sh
# Render every figure of this lecture into ../assets/.
# One script per figure (or per animation), named after the asset it writes.
# Modules and checks are not figures; check_bounds.py runs last and reports
# any label or box that runs off its canvas.
set -eu
cd "$(dirname "$0")"
python3 check_math.py
python3 attn_data.py >/dev/null
for py in *.py; do
    case "$py" in
        svgkit.py|tk.py|kv_grid.py|prompt_bar.py|math_data.py|attn_data.py|check_math.py|check_bounds.py) continue ;;
    esac
    python3 "$py" >/dev/null
done
python3 check_bounds.py
