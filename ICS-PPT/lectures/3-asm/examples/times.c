/* Multiplication by a small constant.

     gcc -Og -fcf-protection=none -S times.c -o - | sed -f asm.sed

   gcc computes x * 5 with one leal, and x * 12 with a leal and a shift. */
int times5(int x) {
    return x * 5;
}

int times12(int x) {
    return x * 12;
}
