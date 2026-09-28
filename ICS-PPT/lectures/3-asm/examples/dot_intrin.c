/* The AVX2 dot product written with intrinsics, and a scalar fallback
   chosen at run time by __builtin_cpu_supports.

     gcc -O2 -fcf-protection=none -S dot_intrin.c -o -   # see the vector loop
     gcc -O2 dot_intrin.c -o dot_intrin && ./dot_intrin

   Only dot_product_avx2 is compiled for AVX2 (the target attribute), so the
   program still runs on a CPU without it and takes the scalar path there. */
#include <immintrin.h>
#include <stdio.h>

__attribute__((target("avx2")))
int dot_product_avx2(const int *w, const int *x, int n) {
    __m256i vsum = _mm256_setzero_si256();   // vpxor %xmm1, %xmm1, %xmm1
    for (int i = 0; i <= n - 8; i += 8) {
        __m256i va = _mm256_loadu_si256((__m256i*)&w[i]);  // vmovdqu
        __m256i vb = _mm256_loadu_si256((__m256i*)&x[i]);  // folded into vpmulld
        __m256i vprod = _mm256_mullo_epi32(va, vb);       // vpmulld
        vsum = _mm256_add_epi32(vsum, vprod);             // vpaddd
    }
    /* Horizontal sum of the eight lanes, then the n % 8 elements left. */
    __m128i s = _mm_add_epi32(_mm256_castsi256_si128(vsum),
                              _mm256_extracti128_si256(vsum, 1));
    s = _mm_add_epi32(s, _mm_srli_si128(s, 8));
    s = _mm_add_epi32(s, _mm_srli_si128(s, 4));
    int sum = _mm_cvtsi128_si32(s);
    for (int i = n & ~7; i < n; i++)
        sum += w[i] * x[i];
    return sum;
}

int dot_product_scalar(const int *w, const int *x, int n) {
    int sum = 0;
    for (int i = 0; i < n; i++)
        sum += w[i] * x[i];
    return sum;
}

int main(void) {
    enum { N = 4100 };          /* not a multiple of 8: the tail runs too */
    static int w[N], x[N];
    for (int i = 0; i < N; i++) {
        w[i] = i % 256 - 128;
        x[i] = (i * 7) % 256 - 128;
    }
    int sum;
    if (__builtin_cpu_supports("avx2")) {
        sum = dot_product_avx2(w, x, N);
        printf("avx2   %d\n", sum);
    } else {
        sum = dot_product_scalar(w, x, N);
        printf("scalar %d\n", sum);
    }
    printf("check  %d\n", dot_product_scalar(w, x, N));
    return 0;
}
