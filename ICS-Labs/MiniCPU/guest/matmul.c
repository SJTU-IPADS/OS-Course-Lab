/* Multiplies two N x N integer matrices. Changing N or the cache's sets and
   ways changes the hit rate noticeably. */
#include "mini.h"

#define N 24

static long a[N][N], b[N][N], c[N][N];

int guest_main(int argc, char **argv)
{
    (void)argc; (void)argv;
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j) {
            a[i][j] = i + 2 * j;
            b[i][j] = 3 * i - j + 1;
        }
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j) {
            long s = 0;
            for (int k = 0; k < N; ++k)
                s += a[i][k] * b[k][j];
            c[i][j] = s;
        }
    long t = 0;
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            t += c[i][j];
    puts("sum = ");
    putint(t);
    return 0;
}
