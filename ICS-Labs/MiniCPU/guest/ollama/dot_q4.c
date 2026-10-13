/* dot_q4.c -- dot_q4 of dot_q4.s in C, one product at a time.

   guest/mini-ollama-ref is guest/mini-ollama-simd with this file in place
   of dot_q4.s. It has no instruction on ymm registers, so it runs on every
   x86-64 machine; make check takes its output on this machine as the
   expected output of guest/mini-ollama-simd under mini-cpu. Both functions
   add up the same 32 integers, so the two programs print the same text. */

/* The sum over j = 0 .. 15 of (b[j] & 15) * x[j] + (b[j] >> 4) * x[j + 16]:
   b is the 16 bytes of a Q4_0 block, x has 32 integers. */
int dot_q4(const unsigned char *b, const int *x)
{
    int sum = 0;
    for (int j = 0; j < 16; j++)
        sum += (b[j] & 15) * x[j] + (b[j] >> 4) * x[j + 16];
    return sum;
}
