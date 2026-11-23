/* dot_q4.c -- dot_q4 of dot_q4.s in C, one product at a time.

   guest/mini-ollama-ref is guest/mini-ollama-simd with this file in place
   of dot_q4.s. It has no instruction on ymm registers, so it runs on every
   x86-64 machine; make check takes its output on this machine as the
   expected output of guest/mini-ollama-simd under mini-cpu. Both functions
   add up the same 32 integers, so the two programs print the same text. */

/* The sum over j = 0 .. 15 of
   (packed[j] & 15) * input[j] + (packed[j] >> 4) * input[j + 16]:
   packed is the 16 bytes of a Q4_0 block, input has 32 integers. */
int dot_q4(const unsigned char *packed, const int *input)
{
    int sum = 0;
    for (int byte = 0; byte < 16; byte++)
        sum += (packed[byte] & 15) * input[byte] +
               (packed[byte] >> 4) * input[byte + 16];
    return sum;
}
