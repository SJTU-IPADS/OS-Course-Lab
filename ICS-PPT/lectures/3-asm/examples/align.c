/* The size of a struct and the offset of each field, with a 1-byte field
   added in front of the others and after them.

     gcc -o align align.c && ./align */
#include <stddef.h>
#include <stdio.h>

struct tensor     { long rows, cols; const int *data; };
struct kind_first { char kind; long rows, cols; const int *data; };
struct kind_last  { long rows, cols; const int *data; char kind; };

#define SHOW(T) printf("%-10s  size %2zu   kind %2zu  rows %2zu  cols %2zu  data %2zu\n", \
                       #T, sizeof(struct T), offsetof(struct T, kind),                    \
                       offsetof(struct T, rows), offsetof(struct T, cols),                \
                       offsetof(struct T, data))

int main(void) {
    printf("%-10s  size %2zu            rows %2zu  cols %2zu  data %2zu\n", "tensor",
           sizeof(struct tensor), offsetof(struct tensor, rows),
           offsetof(struct tensor, cols), offsetof(struct tensor, data));
    SHOW(kind_first);
    SHOW(kind_last);
    return 0;
}
