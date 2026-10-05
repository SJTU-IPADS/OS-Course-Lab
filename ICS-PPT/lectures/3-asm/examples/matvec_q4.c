/* y = W x with int4 weights: the step that dominates LLM token generation.

   W has 4096 columns, 2 KiB per row. Byte j of a row holds weight j in its
   low 4 bits and weight j + 2048 in its high 4 bits; a stored value q means
   the weight q - 8. x holds 4096 int8 activations.

     make matvec_scalar matvec_avx2
     ./matvec_scalar            1 thread, W = 512 MiB
     ./matvec_avx2 8            8 threads
     ./matvec_avx2 8 8          8 threads, W = 8 MiB (fits in L3)

   Prints the milliseconds per product, the operations per second in GFLOPS
   (a multiply-add counts as 2 operations, as in a roofline) and the weight
   bytes read per second. Built with -mavx2 it uses the AVX2 kernel,
   otherwise the scalar one. */
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#ifdef __AVX2__
#include <immintrin.h>
#endif

enum { COLS = 4096, HALF = COLS / 2 };

static int xsum;                        /* sum of x, for the -8 in each weight */
int *y;                                 /* global, so the stores are kept */

#ifndef __AVX2__
static int dot_q4(const unsigned char *w, const signed char *x) {
    int sum = 0;
    for (int j = 0; j < HALF; j++) {
        int lo = (w[j] & 15) - 8, hi = (w[j] >> 4) - 8;
        sum += lo * x[j] + hi * x[j + HALF];
    }
    return sum;
}
#else
/* 64 weights per step. vpmaddubsw multiplies unsigned by signed bytes and
   adds neighbouring products into 16 bits; vpmaddwd widens to 32 bits. The
   stored values q are used as they are, so the -8 is paid once per row. */
static int dot_q4(const unsigned char *w, const signed char *x) {
    __m256i low4 = _mm256_set1_epi8(15), ones = _mm256_set1_epi16(1);
    __m256i acc = _mm256_setzero_si256();
    for (int j = 0; j < HALF; j += 32) {
        __m256i b = _mm256_loadu_si256((const __m256i *)(w + j));
        __m256i lo = _mm256_and_si256(b, low4);
        __m256i hi = _mm256_and_si256(_mm256_srli_epi16(b, 4), low4);
        __m256i xlo = _mm256_loadu_si256((const __m256i *)(x + j));
        __m256i xhi = _mm256_loadu_si256((const __m256i *)(x + j + HALF));
        __m256i p = _mm256_add_epi16(_mm256_maddubs_epi16(lo, xlo),
                                     _mm256_maddubs_epi16(hi, xhi));
        acc = _mm256_add_epi32(acc, _mm256_madd_epi16(p, ones));
    }
    __m128i s = _mm_add_epi32(_mm256_castsi256_si128(acc),
                              _mm256_extracti128_si256(acc, 1));
    s = _mm_add_epi32(s, _mm_shuffle_epi32(s, 0x4e));
    s = _mm_add_epi32(s, _mm_shuffle_epi32(s, 0xb1));
    return _mm_cvtsi128_si32(s) - 8 * xsum;
}
#endif

static void matvec(const unsigned char *w, const signed char *x, size_t rows,
                   int threads) {
    #pragma omp parallel for num_threads(threads)
    for (size_t r = 0; r < rows; r++)
        y[r] = dot_q4(w + r * HALF, x);
}

int main(int argc, char **argv) {
    static signed char x[COLS];
    int threads = argc > 1 ? atoi(argv[1]) : 1;
    size_t bytes = (size_t)(argc > 2 ? atoi(argv[2]) : 512) << 20;
    size_t rows = bytes / HALF;
    unsigned char *w = malloc(bytes);
    y = malloc(rows * sizeof *y);
    if (!w || !y) { fprintf(stderr, "out of memory\n"); return 1; }

    unsigned s = 1;                             /* any bytes will do */
    for (size_t i = 0; i < bytes; i++) {
        s = s * 1103515245 + 12345;
        w[i] = s >> 24;
    }
    for (int i = 0; i < COLS; i++) {
        s = s * 1103515245 + 12345;
        x[i] = s >> 24;
        xsum += x[i];
    }

    matvec(w, x, rows, threads);                /* warm up */
    int reps = 0;
    double t0 = omp_get_wtime(), t;
    do {
        matvec(w, x, rows, threads);
        reps++;
    } while ((t = omp_get_wtime() - t0) < 1.0);
    t /= reps;
    /* each byte of W holds two weights: two multiply-adds, 4 operations */
    printf("threads %d  ms %.3f  GFLOPS %.2f  GB/s %.2f\n",
           threads, t * 1e3, 4.0 * bytes / t / 1e9, bytes / t / 1e9);
    return 0;
}
