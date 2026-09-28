int dot_product(const int *w, const int *x, int n);
int dot_bias(const int *w, const int *x, int n, int b) {
    return dot_product(w, x, n) + b;
}
