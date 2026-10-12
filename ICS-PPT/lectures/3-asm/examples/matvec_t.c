/* matvec for a matrix of any size: the size and the address come in a struct.

     gcc -Og -fcf-protection=none -S matvec_t.c -o - | sed -f asm.sed */
#include "tensor.h"

int dot_product(const int *w, const int *x, int n);

/* The address of row r. */
const int *row(const struct tensor *w, long r) {
    return w->data + r * w->cols;
}

void matvec(int *y, const struct tensor *w, const int *x) {
    for (long r = 0; r < w->rows; r++)
        y[r] = dot_product(w->data + r * w->cols, x, w->cols);
}
