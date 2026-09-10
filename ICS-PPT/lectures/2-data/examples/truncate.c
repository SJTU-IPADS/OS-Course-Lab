/* Widening keeps the value; narrowing keeps only the low bytes. */
#include <stdio.h>
#include <stdint.h>

int main(void) {
    short sx = 12345, sy = -12345;
    printf("short  12345    0x%04x -> int  0x%08x\n", (unsigned short) sx, (int) sx);
    printf("short -12345    0x%04x -> int  0x%08x\n", (unsigned short) sy, (int) sy);

    int x = 53191;
    printf("int    53191    0x%08x -> short 0x%04x = %d\n",
           x, (unsigned short) (short) x, (short) x);

    size_t params = 3212749824;          /* llama-3.2-3B, parameter count */
    size_t bytes  = params * 2;          /* every parameter as bf16 */
    printf("bytes  as size_t   %zu\n", bytes);
    printf("bytes  as int      %d\n", (int) bytes);
    return 0;
}
