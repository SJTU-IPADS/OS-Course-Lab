/* Recurses below the mapped stack. The loader maps 64 stack pages (256 KiB)
   in advance, so the machine stops with an address translation fault once
   the recursion passes them; the demand paging extension lets the stack
   keep growing down. */
#include "mini.h"

/* Each level puts a 1 KiB array on the stack, so the stack grows quickly
   and the compiler cannot turn the recursion into a loop. */
static long down(long depth)
{
    volatile char pad[1024];
    pad[0] = (char)depth;
    if (depth == 0)
        return 0;
    return pad[0] + down(depth - 1) - (char)depth;
}

int guest_main(int argc, char **argv)
{
    (void)argv;
    long depth = argc > 1 ? 4000 : 400;
    puts("depth = ");
    putint(depth);
    down(depth);
    puts("back in guest_main\n");
    return 0;
}
