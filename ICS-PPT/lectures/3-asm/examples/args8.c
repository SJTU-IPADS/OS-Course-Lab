/* args8.c: a call with eight arguments. The first six travel in registers,
 * the seventh and the eighth on the stack.
 *   gcc -Og -fcf-protection=none -S args8.c -o - | sed -f asm.sed
 */
long last2(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8) {
    return a7 - a8;
}

long use8(void) {
    return last2(1, 2, 3, 4, 5, 6, 7, 8);
}
