/* C = A B with the three loops in each of the six possible orders.

   Every version does the same n^3 multiply-adds; only the order of the loops
   differs. The program prints the time per execution of the innermost loop body.

     gcc -O1 matmul_order.c -o matmul_order && ./matmul_order 768 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef double (*matrix)[];

static double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static int n;

#define A(i, k) a[(long)(i) * n + (k)]
#define B(k, j) b[(long)(k) * n + (j)]
#define C(i, j) c[(long)(i) * n + (j)]

void ijk(double *a, double *b, double *c) {
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            double sum = 0.0;
            for (int k = 0; k < n; k++)
                sum += A(i, k) * B(k, j);
            C(i, j) = sum;
        }
}

void jik(double *a, double *b, double *c) {
    for (int j = 0; j < n; j++)
        for (int i = 0; i < n; i++) {
            double sum = 0.0;
            for (int k = 0; k < n; k++)
                sum += A(i, k) * B(k, j);
            C(i, j) = sum;
        }
}

void kij(double *a, double *b, double *c) {
    for (int k = 0; k < n; k++)
        for (int i = 0; i < n; i++) {
            double r = A(i, k);
            for (int j = 0; j < n; j++)
                C(i, j) += r * B(k, j);
        }
}

void ikj(double *a, double *b, double *c) {
    for (int i = 0; i < n; i++)
        for (int k = 0; k < n; k++) {
            double r = A(i, k);
            for (int j = 0; j < n; j++)
                C(i, j) += r * B(k, j);
        }
}

void jki(double *a, double *b, double *c) {
    for (int j = 0; j < n; j++)
        for (int k = 0; k < n; k++) {
            double r = B(k, j);
            for (int i = 0; i < n; i++)
                C(i, j) += A(i, k) * r;
        }
}

void kji(double *a, double *b, double *c) {
    for (int k = 0; k < n; k++)
        for (int j = 0; j < n; j++) {
            double r = B(k, j);
            for (int i = 0; i < n; i++)
                C(i, j) += A(i, k) * r;
        }
}

int main(int argc, char **argv) {
    n = argc > 1 ? atoi(argv[1]) : 768;
    double *a = malloc((size_t)n * n * sizeof(double));
    double *b = malloc((size_t)n * n * sizeof(double));
    double *c = malloc((size_t)n * n * sizeof(double));
    for (long i = 0; i < (long)n * n; i++) { a[i] = 1.0; b[i] = 2.0; }
    void (*version[])(double *, double *, double *) = {ijk, jik, kij, ikj, jki, kji};
    const char *name[] = {"ijk", "jik", "kij", "ikj", "jki", "kji"};
    printf("n = %d\n", n);
    for (int v = 0; v < 6; v++) {
        for (long i = 0; i < (long)n * n; i++) c[i] = 0.0;
        double start = now();
        version[v](a, b, c);
        double seconds = now() - start;
        printf("%s  %5.2f ns per inner iteration  %6.2f s\n", name[v],
               seconds / ((double)n * n * n) * 1e9, seconds);
    }
    return 0;
}
