/* Calls sar4 and shr4 from shift.c with the bit pattern 0xfffffff0.
 * Build: gcc -Og -fcf-protection=none shift.c shift_main.c -o shift
 */
#include <stdio.h>

int sar4(int x);
unsigned shr4(unsigned x);

int main(void) {
    unsigned bits = 0xfffffff0u;
    int r = sar4((int)bits);
    unsigned u = shr4(bits);
    printf("sar4(0x%08x) = 0x%08x = %d\n", bits, (unsigned)r, r);
    printf("shr4(0x%08x) = 0x%08x = %u\n", bits, u, u);
    return 0;
}
