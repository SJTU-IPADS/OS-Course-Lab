/* The smallest guest: print a line and exit. */
#include "mini.h"

int guest_main(int argc, char **argv)
{
    (void)argv;
    puts("hello from the guest\n");
    puts("argc = ");
    putint(argc);
    return 0;
}
