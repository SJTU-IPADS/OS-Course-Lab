/* The same right shift on a signed and on an unsigned operand.

     gcc -Og -fcf-protection=none -S shift.c -o - | sed -f asm.sed

   gcc shifts the int with sarl and the unsigned with shrl. */
int sar4(int x) {
    return x >> 4;
}

unsigned shr4(unsigned x) {
    return x >> 4;
}
