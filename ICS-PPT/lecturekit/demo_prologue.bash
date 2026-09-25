# Read by bash (through BASH_ENV) before a demo's own command, by
# lecturekit/demo.py. Before each top-level command it writes an OSC 7717
# marker naming the command's line; the server takes the markers out of the
# output and shows the command itself in their place, so the drawer reads as a
# terminal transcript. As a file rather than a line in front of the command, so
# that $LINENO and bash's own error messages count the author's lines.

# Only this shell reads it: a bash script the demo runs is not a demo.
unset BASH_ENV

# A copy of stdout, so that `{ ...; } > file` and `$(...)` never capture a
# marker meant for the drawer.
exec {__lk_fd}>&1
__lk_l=0

# `( ... )` subshells report too.
set -T

# Only at the top level: a function body or a sourced file counts lines of its
# own. Only when the line moves forward: a pipeline asks once per part. Inside
# an `if`, so `set -e` never sees the test fail; bash keeps `$?` across the
# trap, so `false; echo $?` still prints 1. On one line, because a newline
# inside the trap would move $LINENO while it runs.
trap 'if [[ -z ${FUNCNAME-}${BASH_SOURCE-} ]] && (( LINENO > __lk_l )); then __lk_l=$LINENO; printf "\033]7717;%d\007" "$LINENO" >&$__lk_fd 2>/dev/null || :; fi' DEBUG
