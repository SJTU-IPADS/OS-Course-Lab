/* Blocked matrix multiplication with the block size given on the command
 * line, so that the cache misses that --stats counts can be compared for
 * block sizes chosen from the cache capacity.
 *
 *     mini-cpu guest/block [block size]        default 8
 *
 * The result equals that of the plain triple loop; only the access order
 * differs. */
#include "mini.h"

#define SIZE 48

static long left[SIZE][SIZE], right[SIZE][SIZE], product[SIZE][SIZE];

int guest_main(int argc, char **argv)
{
    int block_size = 8;
    if (argc > 1) {
        block_size = 0;
        for (const char *digit = argv[1]; *digit >= '0' && *digit <= '9';
             ++digit)
            block_size = block_size * 10 + (*digit - '0');
        if (block_size < 1 || block_size > SIZE) block_size = 8;
    }

    for (int row = 0; row < SIZE; ++row)
        for (int col = 0; col < SIZE; ++col) {
            left[row][col] = row + 2 * col;
            right[row][col] = 3 * row - col + 1;
            product[row][col] = 0;
        }

    for (int row_block = 0; row_block < SIZE; row_block += block_size)
        for (int col_block = 0; col_block < SIZE; col_block += block_size)
            for (int inner_block = 0; inner_block < SIZE;
                 inner_block += block_size) {
                int row_end = row_block + block_size < SIZE
                              ? row_block + block_size : SIZE;
                int col_end = col_block + block_size < SIZE
                              ? col_block + block_size : SIZE;
                int inner_end = inner_block + block_size < SIZE
                                ? inner_block + block_size : SIZE;
                for (int row = row_block; row < row_end; ++row)
                    for (int col = col_block; col < col_end; ++col) {
                        long element = product[row][col];
                        for (int inner = inner_block; inner < inner_end; ++inner)
                            element += left[row][inner] * right[inner][col];
                        product[row][col] = element;
                    }
            }

    long total = 0;
    for (int row = 0; row < SIZE; ++row)
        for (int col = 0; col < SIZE; ++col)
            total += product[row][col];
    puts("sum = ");
    putint(total);
    return 0;
}
