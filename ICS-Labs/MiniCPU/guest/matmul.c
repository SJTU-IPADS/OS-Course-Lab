/* Multiplies two SIZE x SIZE integer matrices. Changing SIZE or the cache's
   sets and ways changes the hit rate noticeably. */
#include "mini.h"

#define SIZE 24

static long left[SIZE][SIZE], right[SIZE][SIZE], product[SIZE][SIZE];

int guest_main(int argc, char **argv)
{
    (void)argc; (void)argv;
    for (int row = 0; row < SIZE; ++row)
        for (int col = 0; col < SIZE; ++col) {
            left[row][col] = row + 2 * col;
            right[row][col] = 3 * row - col + 1;
        }
    for (int row = 0; row < SIZE; ++row)
        for (int col = 0; col < SIZE; ++col) {
            long element = 0;
            for (int inner = 0; inner < SIZE; ++inner)
                element += left[row][inner] * right[inner][col];
            product[row][col] = element;
        }
    long total = 0;
    for (int row = 0; row < SIZE; ++row)
        for (int col = 0; col < SIZE; ++col)
            total += product[row][col];
    puts("sum = ");
    putint(total);
    return 0;
}
