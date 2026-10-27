/* dot_product that skips the zero elements of w.

     gcc -Og -fcf-protection=none -S dot_skip.c -o -

   With -Og the if stays a conditional jump: testl %ecx, %ecx and je .L3
   jump over the multiply-add when w[i] is 0. */
int dot_skip(int *w, int *x, int n) {
    int sum = 0;
    for (int i = 0; i < n; i++)
        if (w[i] != 0)  // compiled to je
            sum += w[i] * x[i];
    return sum;
}
