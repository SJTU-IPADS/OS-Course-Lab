/* Grows the heap with brk, fills it, then reads it back.
   This is the only guest that calls brk, which maps new pages at run time:
   a TLB (lab 4) that keeps a stale entry for a newly mapped page shows up
   here. */
#include "mini.h"

int guest_main(int argc, char **argv)
{
    (void)argv;
    long pages = argc > 1 ? 8 : 3;
    char *lo = (char *)brk(0);              /* query the current break */
    char *hi = (char *)brk(lo + pages * 4096);
    if (hi < lo + pages * 4096) {
        puts("brk returned too little memory\n");
        return 1;
    }
    for (long i = 0; i < pages * 4096; ++i)
        lo[i] = (char)(i * 7 + 1);
    long s = 0;
    for (long i = 0; i < pages * 4096; ++i)
        s += (unsigned char)lo[i];
    puts("sum = ");
    putint(s);
    return 0;
}
