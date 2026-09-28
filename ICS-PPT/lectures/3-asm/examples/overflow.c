/* overflow.c: main from main.c, with x filled from the command line.
 * fill takes the count from argc and never checks it against the 4 elements
 * of x, so from the fifth value on its writes land past the end of x, in the
 * rest of main's frame. The dot product is returned as the exit status, so
 * $? shows whether the overflow changed the result.
 * Build: gcc -Og -fcf-protection=none -fno-stack-protector overflow.c dot.c -o overflow
 */
#include <stdlib.h>

int dot_product(const int *w, const int *x, int n);

static void fill(int *x, int argc, char **argv) {
    for (int i = 1; i < argc; i++) x[i - 1] = atoi(argv[i]);  /* no bound */
}

int main(int argc, char **argv) {
    int w[4] = {1, 2, 3, 4};
    int x[4];
    fill(x, argc, argv);
    return dot_product(w, x, 4);
}
