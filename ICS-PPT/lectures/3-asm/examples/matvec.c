/* A 3 x 4 matrix of weights, stored one row after another.

     gcc -Og -fcf-protection=none -S matvec.c -o - | sed -f asm.sed */
int dot_product(const int *w, const int *x, int n);

/* One element: row r, column j. */
int elem(int W[3][4], long r, long j) {
    return W[r][j];
}

/* y = W x: y[r] is the dot product of row r of W and x. */
void matvec(int *y, int W[3][4], const int *x) {
    for (long r = 0; r < 3; r++)
        y[r] = dot_product(W[r], x, 4);
}
