/* The dot product split across threads with OpenMP.

     gcc -O2 -mavx2 -fopenmp dot_omp.c -o dot_omp
     OMP_NUM_THREADS=8 ./dot_omp

   Without -fopenmp the pragma is ignored and the loop runs on one thread. */
#include <stdio.h>
#ifdef _OPENMP
#include <omp.h>
#endif

/* |w[i]*x[i]| <= 64, so the sum of 2^24 products fits in an int. */
enum { N = 1 << 24 };
static int w[N], x[N];

int dot_product_omp(const int *w, const int *x, int n) {
    int sum = 0;
    #pragma omp parallel for reduction(+:sum)
    for (int i = 0; i < n; i++) {
        sum += w[i] * x[i];
    }
    return sum;
}

int main(void) {
    for (int i = 0; i < N; i++) {
        w[i] = i % 16 - 8;
        x[i] = (i * 7) % 16 - 8;
    }
    int threads = 1;
#ifdef _OPENMP
    threads = omp_get_max_threads();
#endif
    int sum = dot_product_omp(w, x, N);
    int check = 0;
    for (int i = 0; i < N; i++)
        check += w[i] * x[i];
    printf("threads %d\nsum     %d\ncheck   %d\n", threads, sum, check);
    return 0;
}
