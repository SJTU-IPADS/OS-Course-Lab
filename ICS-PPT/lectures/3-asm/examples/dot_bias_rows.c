/* dot_bias called rows times: b is used after every call.

     gcc -Og -fcf-protection=none -S dot_bias_rows.c -o - | sed -f asm.sed

   w, x, n, b, rows, total and r all live across the call; six of them get
   the six callee-saved registers and the seventh goes to the stack. */
int dot_product(const int *w, const int *x, int n);

int dot_bias_rows(const int *w, const int *x, int n, int b, int rows) {
    int total = 0;
    for (int r = 0; r < rows; r++)
        total += dot_product(w + r * n, x, n) + b;
    return total;
}
