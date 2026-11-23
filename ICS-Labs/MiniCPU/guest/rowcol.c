/* Sums a matrix of int stored one row after another, by rows or by
 * columns: the functions sum_rows and sum_cols.
 *
 *     mini-cpu guest/rowcol [rows|cols]        default rows
 *
 * Both orders read the same elements and make the same additions; only
 * the order of the addresses differs. sum_rows reads the bytes of a line
 * one after another, sum_cols reads one int of a line and moves on to the
 * next row, SIZE * 4 bytes further. The matrix holds SIZE * SIZE * 4 =
 * 64 KiB, four times the 16 KiB of the default cache (--cache 64,4), so
 * no pass finds the lines of the pass before it. The sum runs PASSES
 * times, and the output is the total of the passes. */
#include "mini.h"

#define SIZE   128
#define PASSES 8

static int matrix[SIZE][SIZE];

static int sum_rows(int values[SIZE][SIZE])
{
    int sum = 0;
    for (int row = 0; row < SIZE; ++row)
        for (int col = 0; col < SIZE; ++col)
            sum += values[row][col];
    return sum;
}

static int sum_cols(int values[SIZE][SIZE])
{
    int sum = 0;
    for (int col = 0; col < SIZE; ++col)
        for (int row = 0; row < SIZE; ++row)
            sum += values[row][col];
    return sum;
}

int guest_main(int argc, char **argv)
{
    int by_cols = argc > 1 && argv[1][0] == 'c';
    for (int row = 0; row < SIZE; ++row)
        for (int col = 0; col < SIZE; ++col)
            matrix[row][col] = row + 2 * col;
    long total = 0;
    for (int pass = 0; pass < PASSES; ++pass)
        total += by_cols ? sum_cols(matrix) : sum_rows(matrix);
    puts(by_cols ? "sum_cols = " : "sum_rows = ");
    putint(total);
    return 0;
}
