/* local.c: print the address of a local variable.
 * local is in the stack frame of main, so its address shows where the stack
 * is in this run. Run the program several times and compare.
 * Build: gcc -Og -o local local.c
 */
#include <stdio.h>

int main(void) {
    long local;
    printf("local at %p\n", (void *)&local);
    return 0;
}
