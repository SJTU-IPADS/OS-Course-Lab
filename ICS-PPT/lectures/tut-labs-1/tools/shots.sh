#!/bin/sh
# Regenerate the terminal screenshots in ../assets/ (bomb_*.png).
#
#   sh tools/shots.sh [ONLINE_BOMB]
#
# run from the lecture directory. examples/bomb is the offline demo bomb
# (every phase variant 0, no server; see README.md). ONLINE_BOMB is a bomb
# fetched from a server started on 127.0.0.1 for the scoreboard picture; it
# defaults to $SCRATCH/kit/bomb-000000000003. PYTHON names a Python with pyte
# and Pillow installed (default: $SCRATCH/venv/bin/python).
set -eu
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
lecture=$here/..
scratch=${SCRATCH:-$lecture/../../build/scratch-bomb}
python=${PYTHON:-$scratch/venv/bin/python}
board=${1:-$scratch/kit/bomb-000000000003}
shot="$python $here/termshot.py --cols 80 --font-size 20"
bomb=$lecture/examples/bomb
answer='The stack grows toward lower addresses.'     # phase 1, variant 0

$shot --rows 20 --out "$lecture/assets/bomb_phase.png" \
    --wait "phase 1 of 8" --pause 0.5 -- "$bomb"
$shot --rows 20 --out "$lecture/assets/bomb_boom.png" \
    --wait "phase 1 of 8" --type "a wrong line" --key enter --wait BOOM --pause 2 -- "$bomb"
$shot --rows 20 --out "$lecture/assets/bomb_defused.png" \
    --wait "phase 1 of 8" --type "$answer" --key enter --wait "phase 2 of 8" --pause 0.6 \
    -- "$bomb"
$shot --rows 18 --out "$lecture/assets/bomb_board.png" \
    --wait rank --pause 1 --type q -- "$board" -s
