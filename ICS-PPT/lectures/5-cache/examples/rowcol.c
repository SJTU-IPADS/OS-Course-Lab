/* Read a 4096 x 4096 matrix of int row by row and column by column.

     gcc -O1 rowcol.c -o rowcol && ./rowcol */
#include <stdio.h>
#include <time.h>

#define N 4096

static int W[N][N];                       /* 64 MB, stored one row after another */

static double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int sum_rows(int W[N][N]) {
    int sum = 0;
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            sum += W[i][j];
    return sum;
}

int sum_cols(int W[N][N]) {
    int sum = 0;
    for (int j = 0; j < N; j++)
        for (int i = 0; i < N; i++)
            sum += W[i][j];
    return sum;
}

int main(void) {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            W[i][j] = 1;
    double best_rows = 1e30, best_cols = 1e30;
    volatile int sink;
    for (int rep = 0; rep < 3; rep++) {
        double start = now();
        sink = sum_rows(W);
        double t_rows = now() - start;
        start = now();
        sink = sum_cols(W);
        double t_cols = now() - start;
        if (t_rows < best_rows) best_rows = t_rows;
        if (t_cols < best_cols) best_cols = t_cols;
    }
    (void)sink;
    printf("sum_rows  %5.2f ns per element\n", best_rows / ((double)N * N) * 1e9);
    printf("sum_cols  %5.2f ns per element\n", best_cols / ((double)N * N) * 1e9);
    return 0;
}
