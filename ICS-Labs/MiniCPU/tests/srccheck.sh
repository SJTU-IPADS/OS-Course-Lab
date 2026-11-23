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
NLABS=$2
CC=${CC:-gcc}
CFLAGS="-std=c11 -O1 -g -Wall -Wextra -Iinclude -Imodel -D_GNU_SOURCE -DLAB=$NLABS"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
bad=0
problem() { echo "source check: $*"; bad=1; }

# allowed GROUP...: the symbols of these groups of tests/symbols.txt, one per line
allowed() {
    for group in "$@"; do sed -n "s/^$group://p" tests/symbols.txt; done | tr ' ' '\n' | sed '/^$/d'
}

# the names libc and libm define: a lab must not define them
for lib in libc.so.6 libm.so.6; do
    libfile=$($CC -print-file-name=$lib)
    nm -D --defined-only "$libfile" 2>/dev/null
done | awk '{ sub(/@.*/, "", $NF); print $NF }' | sort -u > "$TMP/libc"

lab=1
while [ "$lab" -le "$NLABS" ]; do
    dir=$SRC/lab$lab
    rm -f "$TMP"/*.o
    for file in "$dir"/*.c; do
        [ -f "$file" ] || continue
        name=$(basename "$file" .c)
        $CC $CFLAGS -c -o "$TMP/$name.o" "$file" 2> "$TMP/cc" ||
            { problem "lab$lab/$name.c does not compile"; head -n 3 "$TMP/cc"; }
    done
    set -- "$TMP"/*.o
    if [ -f "$1" ]; then
        groups="libc lab$lab"
        [ "$lab" = 5 ] && groups="$groups libc-io"
        nm --defined-only "$@" | awk 'NF == 3 && $2 ~ /[A-Z]/ { print $3 }' | sort -u > "$TMP/def"
        { allowed $groups; cat "$TMP/def"; } | sort -u > "$TMP/ok"
        for object in "$@"; do
            name=$(basename "$object" .o)
            nm -u "$object" | awk '{ print $NF }' | sort -u | comm -23 - "$TMP/ok" |
                while read -r sym; do echo "source check: lab$lab/$name.c uses $sym, which lab $lab may not use (tests/symbols.txt)"; done
            nm --defined-only "$object" | awk 'NF == 3 && $2 ~ /[A-Z]/ { print $3 }' | sort -u > "$TMP/d1"
            comm -12 "$TMP/d1" "$TMP/libc" |
                while read -r sym; do echo "source check: lab$lab/$name.c defines $sym, a function or variable of libc"; done
            grep '^__' "$TMP/d1" |
                while read -r sym; do echo "source check: lab$lab/$name.c defines $sym; names starting with __ are reserved"; done
            readelf -SW "$object" | sed -n 's/^ *\[ *[0-9]*\] \([^ ]*\) .*/\1/p' |
                grep -vE '^(\.text|\.data|\.bss|\.rodata|\.tbss|\.tdata)(\..*)?$|^\.rela?\.(text|data|rodata|debug_.*|eh_frame|tdata)(\..*)?$|^\.debug_|^\.comment$|^\.note\.GNU-stack$|^\.note\.gnu\.property$|^\.eh_frame$|^\.group$|^\.symtab$|^\.strtab$|^\.shstrtab$|^$' |
                while read -r section; do echo "source check: lab$lab/$name.c has the section $section"; done
            objdump -h "$object" | awk '$1 ~ /^[0-9]+$/ { section = $2; next }
                                        /CODE/ && section !~ /^\.text/ { print section }' |
                while read -r section; do echo "source check: lab$lab/$name.c has code in the section $section"; done
            objdump -d "$object" | grep -E '[[:space:]](syscall|sysenter|int +\$0x80)([[:space:]]|$)' |
                head -n 1 | while read -r line; do echo "source check: lab$lab/$name.c contains a system call instruction"; done
        done > "$TMP/why"
        [ -s "$TMP/why" ] && { cat "$TMP/why"; bad=1; }
    fi
    lab=$((lab + 1))
done
exit $bad
