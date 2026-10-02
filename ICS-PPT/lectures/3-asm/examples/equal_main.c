/* Calls equal from equal.c with the byte 0xf0 in both arguments.
 * Build: gcc -Og -fcf-protection=none equal.c equal_main.c -o equal
 */
#include <stdio.h>

int equal(signed char a, unsigned char b);

int main(void) {
    unsigned char byte = 0xf0;
    signed char a = (signed char)byte;
    unsigned char b = byte;
    printf("a = %d (byte 0x%02x), b = %d (byte 0x%02x), a == b: %d\n",
           a, (unsigned char)a, b, b, equal(a, b));
    return 0;
}
