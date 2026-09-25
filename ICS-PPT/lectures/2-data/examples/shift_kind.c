/* Which right shift the same >> performs is decided by the left operand's type. */
#include <stdint.h>
#include <stdio.h>

int32_t  arithmetic(int32_t  x) { return x >> 1; }   /* signed   -> sar */
uint32_t logical(uint32_t x)    { return x >> 1; }   /* unsigned -> shr */

int main(void) {
    int32_t  s = -8;
    uint32_t u = (uint32_t) s;                       /* the same 32 bits */
    printf("bits        0x%08x\n", u);
    printf("int32_t  >> 1   %11d   0x%08x   sign bit copied in\n",
           arithmetic(s), (uint32_t) arithmetic(s));
    printf("uint32_t >> 1   %11u   0x%08x   zero shifted in\n",
           logical(u), logical(u));
    return 0;
}
