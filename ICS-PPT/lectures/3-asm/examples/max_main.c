/* Calls max and max_u from max.c with the bit patterns 0xfffffff0 and
 * 0x000000f0.
 * Build: gcc -Og -fcf-protection=none max.c max_main.c -o max
 */
#include <stdio.h>

int max(int a, int b);
unsigned max_u(unsigned a, unsigned b);

int main(void) {
    unsigned x = 0xfffffff0u, y = 0x000000f0u;
    int r = max((int)x, (int)y);
    unsigned u = max_u(x, y);
    printf("max  (%d, %d) = %d\n", (int)x, (int)y, r);
    printf("max_u(%u, %u) = %u\n", x, y, u);
    return 0;
}
