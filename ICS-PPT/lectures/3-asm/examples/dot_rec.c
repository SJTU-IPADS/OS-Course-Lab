int dot_product_rec(const int *w, const int *x, int n) {
    if (n <= 0) return 0;
    return dot_product_rec(w, x, n - 1) + w[n - 1] * x[n - 1];
}

int main(void) {
    int w[4] = {1, 2, 3, 4};
    int x[4] = {5, 6, 7, 8};
    return dot_product_rec(w, x, 4);
}
