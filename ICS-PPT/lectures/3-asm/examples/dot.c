int dot_product(const int *w, const int *x, int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) {
        sum += w[i] * x[i];
    }
    return sum;
}
