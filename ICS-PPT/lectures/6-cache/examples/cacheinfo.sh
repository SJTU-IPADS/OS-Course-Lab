#!/bin/sh
# Print the caches of cpu0: level, type, size, ways, line size, sets.
for dir in /sys/devices/system/cpu/cpu0/cache/index*; do
    printf '%s  %-12s %6s  %3s-way  line %s B  %5s sets\n' \
        "L$(cat $dir/level)" "$(cat $dir/type)" "$(cat $dir/size)" \
        "$(cat $dir/ways_of_associativity)" "$(cat $dir/coherency_line_size)" \
        "$(cat $dir/number_of_sets)"
done
