/* unhex.c -- turn a hex exploit into the raw bytes to feed the program.
 *
 *     ./unhex < exploit.txt | ./attack1
 *     ./unhex < exploit.txt > exploit.bin
 *
 * The input is pairs of hex digits separated by any whitespace. A '#' starts
 * a comment that runs to the end of the line, so an exploit can be annotated.
 * Every other pair is one output byte. Nothing is added -- no trailing
 * newline -- so the bytes are exactly what was written. Avoid the byte 0a,
 * which the program reads as the end of the prompt.
 *
 * Example input:
 *
 *     41 41 41 41 41 41 41 41   # eight bytes of padding
 *     c6 11 40 00 00 00 00 00   # the address to jump to, little-endian
 */
#include <stdio.h>
#include <ctype.h>

static int hexval(int c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

int main(void)
{
    int c, hi = -1;

    while ((c = getchar()) != EOF) {
        if (c == '#') {
            while ((c = getchar()) != EOF && c != '\n')
                ;
            if (c == EOF)
                break;
            continue;
        }
        if (isspace(c))
            continue;
        int v = hexval(c);
        if (v < 0) {
            fprintf(stderr, "unhex: not a hex digit: '%c'\n", c);
            return 1;
        }
        if (hi < 0) {
            hi = v;
        } else {
            putchar((hi << 4) | v);
            hi = -1;
        }
    }
    if (hi >= 0) {
        fprintf(stderr, "unhex: a hex digit is left over (odd number of digits)\n");
        return 1;
    }
    return 0;
}
