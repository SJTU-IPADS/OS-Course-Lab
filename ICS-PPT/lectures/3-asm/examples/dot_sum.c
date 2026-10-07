/* dot_sum.c: two calls to dot_product in one function. w, y and n are needed
 * after the first call returns, and the first result after the second.
 *   gcc -Og -fcf-protection=none -S dot_sum.c -o - | sed -f asm.sed
 */
int dot_product(const int *w, const int *x, int n);

int dot_sum(const int *w, const int *x, const int *y, int n) {
    return dot_product(w, x, n) + dot_product(w, y, n);
}
