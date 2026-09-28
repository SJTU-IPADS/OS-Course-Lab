int dot_product(const int *w, const int *x, int n);
int w[4096], x[4096];
volatile int sink;

int main(void) {
    for (int i = 0; i < 4096; i++) {
        w[i] = i % 256 - 128;
        x[i] = (i * 7) % 256 - 128;
    }
    for (int r = 0; r < 100000; r++) {
        __asm__ volatile("" ::: "memory");
        sink = dot_product(w, x, 4096);
    }
    return 0;
}
