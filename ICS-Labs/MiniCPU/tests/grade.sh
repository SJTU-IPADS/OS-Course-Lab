#!/bin/sh
# Run the tests of labs 1 .. N and add up the points.
#
#   sh tests/grade.sh DIR N [SRCDIR]
#
# DIR holds mini-cpu and mini-jit, SRCDIR the directories lab1 .. labN
# (default src); make check calls this.
#
# First tests/srccheck.sh checks the files of the labs; if it reports a
# problem, no test runs and the total score is 0. Some tests use random
# arguments drawn from SEED, printed on the first line; SEED=n make check
# repeats a run with the same arguments. The files of tests/, tools/ and
# guest/*.txt are made read-only while the tests run, and their checksums
# must be the same afterwards.
set -u
DIR=$1
NLABS=$2
SRC=${3:-src}
SEED=${SEED:-$(od -An -N2 -tu2 /dev/urandom | tr -d ' ')}
export SEED
echo "seed $SEED"

if ! sh tests/srccheck.sh "$SRC" "$NLABS"; then
    echo "total score 0/$((NLABS * 100)) (source check failed, no test was run)"
    exit 0
fi

SCORES=$(mktemp)
SUMS=$(mktemp)
export SCORES
# tools/ and guest/*.txt arrive with later labs; only what exists is listed
FILES=$(ls -d tests tools guest/*.txt 2>/dev/null)
find $FILES -type f | LC_ALL=C sort | xargs cksum > "$SUMS"
# their modes, restored at the end
find $FILES -type f -exec stat -c 'chmod %a %n' {} + > "$SUMS.m"
find $FILES -type f -exec chmod a-w {} +
trap 'sh "$SUMS.m"; rm -f "$SCORES" "$SUMS" "$SUMS.m"' EXIT
trap 'exit 1' INT TERM

lab=1
while [ "$lab" -le "$NLABS" ]; do
    echo "== lab $lab"
    sh "tests/lab$lab/run.sh" "$DIR"
    # a script that ended before its last line counts as 0 of 100
    if ! grep -q "^$lab " "$SCORES" 2>/dev/null; then
        echo "lab $lab score 0/100 (tests/lab$lab/run.sh did not finish)"
        echo "$lab 0 100" >> "$SCORES"
    fi
    lab=$((lab + 1))
done

if ! find $FILES -type f | LC_ALL=C sort | xargs cksum | cmp -s - "$SUMS"; then
    echo "files of tests/, tools/ or guest/ changed while the tests ran"
    echo "total score 0/$((NLABS * 100))"
    exit 0
fi
awk '{ got += $2; max += $3 } END { printf "total score %d/%d\n", got, max }' "$SCORES"
