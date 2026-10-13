/* Blocked matrix multiplication with the block size given on the command
 * line. This is L3 of lecture 2's LAB.md: choose the block size from the
 * cache capacity, then measure whether the cache misses actually drop.
 *
 *     mini-cpu guest/block [block size]        default 8
 *
 * The result equals that of the plain triple loop; only the access order
 * differs. */
#include "mini.h"

#define N 48

static long a[N][N], b[N][N], c[N][N];

int guest_main(int argc, char **argv)
{
    int bs = 8;
    if (argc > 1) {
        bs = 0;
        for (const char *p = argv[1]; *p >= '0' && *p <= '9'; ++p)
            bs = bs * 10 + (*p - '0');
        if (bs < 1 || bs > N) bs = 8;
    }

    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j) {
            a[i][j] = i + 2 * j;
            b[i][j] = 3 * i - j + 1;
            c[i][j] = 0;
        }

    for (int ii = 0; ii < N; ii += bs)
        for (int jj = 0; jj < N; jj += bs)
            for (int kk = 0; kk < N; kk += bs) {
                int ie = ii + bs < N ? ii + bs : N;
                int je = jj + bs < N ? jj + bs : N;
                int ke = kk + bs < N ? kk + bs : N;
                for (int i = ii; i < ie; ++i)
                    for (int j = jj; j < je; ++j) {
                        long s = c[i][j];
                        for (int k = kk; k < ke; ++k)
                            s += a[i][k] * b[k][j];
                        c[i][j] = s;
                    }
            }

    long t = 0;
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            t += c[i][j];
    puts("sum = ");
    putint(t);
    return 0;
}
