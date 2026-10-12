/* main keeps the matrix, the two vectors and a struct tensor in its frame.

     gcc -Og -fcf-protection=none -fno-stack-protector -S tensor_main.c -o - | sed -f asm.sed
     gcc -Og -o tensor_main tensor_main.c matvec_t.c dot.c && ./tensor_main; echo $?     prints 22 */
#include "tensor.h"

void matvec(int *y, const struct tensor *w, const int *x);

int main(void) {
    int W[3][4] = {{1, 2, 3, 4}, {5, 6, 7, 8}, {9, 10, 11, 12}};
    int x[4] = {5, 6, 7, 8};
    int y[3];
    struct tensor t = {3, 4, &W[0][0]};
    matvec(y, &t, x);
    return y[2];
}
