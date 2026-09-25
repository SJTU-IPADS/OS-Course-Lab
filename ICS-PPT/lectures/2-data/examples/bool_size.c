/* How a boolean value is stored, in C and in C++. */
#include <stdio.h>
#ifndef __cplusplus
#include <stdbool.h>            /* C99 adds built-in _Bool; C17 exposes bool here */
#define LANG "C  "
#else
#define LANG "C++"
#endif

int main(void) {
    bool b = 17;                /* any nonzero value converts to 1 */
    printf("%s  sizeof(bool) %zu  value %d  byte %02x\n",
           LANG, sizeof(bool), (int) b, *(unsigned char *) &b);
    return 0;
}
