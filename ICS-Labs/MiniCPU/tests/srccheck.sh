#!/bin/sh
# Check the object files of src/lab1 .. src/labN before the tests run.
#
#   sh tests/srccheck.sh SRCDIR N
#
# Every .c file is compiled with the options of the Makefile and read with
# nm, readelf and objdump. A lab passes when
#   - the functions and variables it uses from outside its own files are
#     those of its line in tests/symbols.txt, or of libc;
#   - it defines no name of libc (which would replace the libc function
#     for the whole program) and no name starting with __;
#   - its object files have no sections other than code, data and debug
#     information (no .init_array: nothing runs before main), and code
#     only in .text sections;
#   - its code has no system call instruction.
# Prints one line per problem; the exit status is 1 if there is any.
set -u
export LC_ALL=C
SRC=$1
N=$2
CC=${CC:-gcc}
CFLAGS="-std=c11 -O1 -g -Wall -Wextra -Iinclude -Imodel -D_GNU_SOURCE -DLAB=$N"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
bad=0
problem() { echo "source check: $*"; bad=1; }

# allowed GROUP...: the symbols of these groups of tests/symbols.txt, one per line
allowed() {
    for g in "$@"; do sed -n "s/^$g://p" tests/symbols.txt; done | tr ' ' '\n' | sed '/^$/d'
}

# the names libc and libm define: a lab must not define them
for l in libc.so.6 libm.so.6; do
    p=$($CC -print-file-name=$l)
    nm -D --defined-only "$p" 2>/dev/null
done | awk '{ sub(/@.*/, "", $NF); print $NF }' | sort -u > "$TMP/libc"

k=1
while [ "$k" -le "$N" ]; do
    d=$SRC/lab$k
    rm -f "$TMP"/*.o
    for f in "$d"/*.c; do
        [ -f "$f" ] || continue
        b=$(basename "$f" .c)
        $CC $CFLAGS -c -o "$TMP/$b.o" "$f" 2> "$TMP/cc" ||
            { problem "lab$k/$b.c does not compile"; head -n 3 "$TMP/cc"; }
    done
    set -- "$TMP"/*.o
    if [ -f "$1" ]; then
        groups="libc lab$k"
        [ "$k" = 6 ] && groups="$groups libc-io"
        nm --defined-only "$@" | awk 'NF == 3 && $2 ~ /[A-Z]/ { print $3 }' | sort -u > "$TMP/def"
        { allowed $groups; cat "$TMP/def"; } | sort -u > "$TMP/ok"
        for o in "$@"; do
            b=$(basename "$o" .o)
            nm -u "$o" | awk '{ print $NF }' | sort -u | comm -23 - "$TMP/ok" |
                while read -r s; do echo "source check: lab$k/$b.c uses $s, which lab $k may not use (tests/symbols.txt)"; done
            nm --defined-only "$o" | awk 'NF == 3 && $2 ~ /[A-Z]/ { print $3 }' | sort -u > "$TMP/d1"
            comm -12 "$TMP/d1" "$TMP/libc" |
                while read -r s; do echo "source check: lab$k/$b.c defines $s, a function or variable of libc"; done
            grep '^__' "$TMP/d1" |
                while read -r s; do echo "source check: lab$k/$b.c defines $s; names starting with __ are reserved"; done
            readelf -SW "$o" | sed -n 's/^ *\[ *[0-9]*\] \([^ ]*\) .*/\1/p' |
                grep -vE '^(\.text|\.data|\.bss|\.rodata|\.tbss|\.tdata)(\..*)?$|^\.rela?\.(text|data|rodata|debug_.*|eh_frame|tdata)(\..*)?$|^\.debug_|^\.comment$|^\.note\.GNU-stack$|^\.note\.gnu\.property$|^\.eh_frame$|^\.group$|^\.symtab$|^\.strtab$|^\.shstrtab$|^$' |
                while read -r s; do echo "source check: lab$k/$b.c has the section $s"; done
            objdump -h "$o" | awk '$1 ~ /^[0-9]+$/ { s = $2; next }
                                   /CODE/ && s !~ /^\.text/ { print s }' |
                while read -r s; do echo "source check: lab$k/$b.c has code in the section $s"; done
            objdump -d "$o" | grep -E '[[:space:]](syscall|sysenter|int +\$0x80)([[:space:]]|$)' |
                head -n 1 | while read -r s; do echo "source check: lab$k/$b.c contains a system call instruction"; done
        done > "$TMP/why"
        [ -s "$TMP/why" ] && { cat "$TMP/why"; bad=1; }
    fi
    k=$((k + 1))
done
exit $bad
