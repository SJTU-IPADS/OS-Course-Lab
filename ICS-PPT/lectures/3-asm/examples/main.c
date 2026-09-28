int dot_product(const int *w, const int *x, int n);
int main(void) {
    int w[4] = {1, 2, 3, 4};
    int x[4] = {5, 6, 7, 8};
    return dot_product(w, x, 4);
}
