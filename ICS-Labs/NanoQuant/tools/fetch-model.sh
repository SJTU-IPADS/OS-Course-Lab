#!/bin/sh
# Download the model files listed in model/SHA256SUMS from the Hugging Face
# repository Qwen/Qwen3-VL-2B-Instruct, and check each one against its digest.
#
#   sh tools/fetch-model.sh          download into model/
#   sh tools/fetch-model.sh DIR      download into DIR
#
# The files come from $HF_ENDPOINT, https://hf-mirror.com when it is unset.
# To download from Hugging Face itself:
#   HF_ENDPOINT=https://huggingface.co sh tools/fetch-model.sh
#
# A file already in place with the right digest is not downloaded again, so
# the script also checks files obtained some other way. An interrupted
# download stays in NAME.part and resumes on the next run.
#
# One line per file on stdout, NAME: OK or NAME: FAILED; the exit status is
# the number of files that failed.

REPO=Qwen/Qwen3-VL-2B-Instruct
REV=main
BASE=${HF_ENDPOINT:-https://hf-mirror.com}
BASE=${BASE%/}
DIR=${1:-model}
SUMS=$(dirname "$0")/../model/SHA256SUMS

if command -v sha256sum >/dev/null 2>&1; then
    digest() { sha256sum "$1" | cut -d ' ' -f 1; }
else
    digest() { shasum -a 256 "$1" | cut -d ' ' -f 1; }     # macOS
fi

command -v curl >/dev/null 2>&1 || { echo "fetch-model: curl is needed" >&2; exit 2; }
[ -f "$SUMS" ] || { echo "fetch-model: $SUMS is missing" >&2; exit 2; }
mkdir -p "$DIR" || exit 2

fail=0
while read -r want name; do
    out=$DIR/$name
    if [ -f "$out" ] && [ "$(digest "$out")" = "$want" ]; then
        echo "$name: OK"
        continue
    fi
    echo "fetch-model: $name from $BASE" >&2
    curl -fL --retry 3 -C - --progress-bar -o "$out.part" \
        "$BASE/$REPO/resolve/$REV/$name" </dev/null
    st=$?
    if [ $st -ne 0 ]; then
        echo "fetch-model: curl stopped with status $st; run again to retry," \
             "a partial download resumes from $out.part" >&2
        echo "$name: FAILED"
        fail=$((fail + 1))
        continue
    fi
    got=$(digest "$out.part")
    if [ "$got" != "$want" ]; then
        rm -f "$out.part"        # complete but wrong: resuming it cannot help
        echo "fetch-model: $name has sha256 $got, expected $want" >&2
        echo "$name: FAILED"
        fail=$((fail + 1))
        continue
    fi
    mv "$out.part" "$out"
    echo "$name: OK"
done <"$SUMS"
exit $fail
