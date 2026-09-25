#!/bin/sh
# Checks nano-quant, nq2gguf and nq-selftest on the test files; needs no model.
#
#   sh tests/run.sh          check the three programs in the current directory
#   sh tests/run.sh DIR      check the three programs in DIR
#
# $PYTHON names the Python 3 command, python3 when it is unset.
#
# One line per item. A failed item does not stop the others; the exit status
# is the number of failed items.

set -e
D=${1:-.}
Q=$D/nano-quant
G=$D/nq2gguf
T=$D/nq-selftest
F=tests/fixtures
PY=${PYTHON:-python3}
W=$(mktemp -d)
trap 'rm -rf "$W"' EXIT

fail=0
ok()   { printf '  pass  %s\n' "$1"; }
bad()  { printf '  FAIL  %s\n' "$1"; fail=$((fail+1)); }
check() { if [ "$2" = "$3" ]; then ok "$1"; else bad "$1 (got $2, expected $3)"; fi; }

[ -f $F/tiny.safetensors ] || { echo "run 'make fixtures' first"; exit 2; }

echo "1. block formats and bits"
if $T --expect tests/expected-blocks.txt >"$W/self" 2>"$W/selferr"; then
    ok "nq-selftest matches the expected values"
else
    bad "nq-selftest differs from the expected values"
    grep -v matches "$W/self" | sed 's/^/    /'
fi
sed 's/^/    /' "$W/selferr"

echo "2. reading safetensors"
# the header length written big endian must be refused, little endian accepted
if ! $Q plan $F/tiny.safetensors >/dev/null 2>&1; then
    bad "little-endian header length refused"
elif $Q plan $F/tiny-be.safetensors >/dev/null 2>&1; then
    bad "big-endian header length accepted"
else
    ok "little-endian header length accepted, big-endian refused"
fi

echo "3. quantizing the test model"
qst=0
$Q quant $F/tiny.safetensors --recipe q4_k_m -o "$W/tiny.nq" >"$W/q" 2>/dev/null || qst=$?
check "data area md5" "$(awk '{print $1}' "$W/q")" "$(cat tests/expected-tiny-nq.md5)"

echo "4. GGUF assembly"
# a .nq is written at its full length before any tensor, so a quant that
# stopped halfway leaves a file of the right shape: assemble only a finished one
if [ $qst -ne 0 ]; then
    bad "nothing to assemble: quant stopped with status $qst"
else
    $PY tools/mkmeta.py $F/tiny-meta -o "$W/meta.kv" --name tiny >/dev/null || true
    $G "$W/tiny.nq" --meta "$W/meta.kv" -o "$W/tiny.gguf" >/dev/null 2>&1 || true
    if [ -f "$W/tiny.gguf" ] && $PY tools/ggufdump.py "$W/tiny.gguf" >"$W/dump" 2>&1; then
        ok "GGUF layout consistent"
    else
        bad "GGUF layout broken"
        [ -f "$W/dump" ] && sed 's/^/    /' "$W/dump" || true
    fi
fi

echo
if [ $fail -eq 0 ]; then
    echo "all passed"
else
    echo "$fail failed"
fi
exit $fail
