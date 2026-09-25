/* Print the byte representation of an object, whatever its type. */
#include <stdio.h>

typedef unsigned char *byte_pointer;      /* a pointer to plain bytes */

void show_bytes(byte_pointer start, size_t len) {
    for (size_t i = 0; i < len; i++)
        printf(" %.2x", start[i]);            /* one byte at a time */
}

int main(void) {
    int   ival = 12345;                       /* 0x00003039 */
    float fval = (float) ival;
    int  *pval = &ival;

    printf("%-14s", "int 12345");   show_bytes((byte_pointer) &ival, sizeof(ival));
    printf("\n%-14s", "float 12345"); show_bytes((byte_pointer) &fval, sizeof(fval));
    printf("\n%-14s", "&ival");       show_bytes((byte_pointer) &pval, sizeof(pval));
    printf("\n");
    return 0;
}
