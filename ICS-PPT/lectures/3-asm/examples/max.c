/* The same selection on signed and on unsigned operands.

     gcc -Og -fcf-protection=none -S max.c -o - | sed -f asm.sed

   Both functions compare with the same cmpl. gcc follows it with jl for
   int and with jb for unsigned. */
int max(int a, int b) {
    int v = a;
    if (a < b) v = b;
    return v;
}

unsigned max_u(unsigned a, unsigned b) {
    unsigned v = a;
    if (a < b) v = b;
    return v;
}
