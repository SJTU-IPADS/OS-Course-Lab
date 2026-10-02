/* One byte, read as two types, then compared.

     gcc -Og -fcf-protection=none -S equal.c -o - | sed -f asm.sed

   C converts both operands of == to int first. gcc widens the signed char
   with movsbl and the unsigned char with movzbl. */
int equal(signed char a, unsigned char b) {
    return a == b;
}
