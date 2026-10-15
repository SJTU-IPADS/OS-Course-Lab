/* dot_q4: the integer sum of one Q4_0 block, eight products at a time.

       int dot_q4(const unsigned char *b, const int *x);

   b in %rdi points at the 16 data bytes of the block, x in %rsi at 32
   integers. Byte j holds two stored values of 4 bits: weight j in its low
   half, weight j + 16 in its high half. The result in %eax is

       the sum over j = 0 .. 15 of (b[j] & 15) * x[j] + (b[j] >> 4) * x[j + 16]

   The 16 bytes are taken in two halves of 8. vpmovzxbd widens 8 bytes to
   eight 32-bit integers; vpand and vpsrld cut each into its low and its
   high 4 bits; vpmulld multiplies both by their eight numbers of x, and
   vpaddd collects the products in eight lane sums. The eight lane sums
   become one number as in linear_simd of guest/linear.s. matvec_simd.c
   calls this function once for every block of every row.

   vzeroupper is left out. On a real processor it removes a delay in the
   SSE instructions that follow; mini-cpu has no such delay and no such
   instruction. */
        .section .rodata
        .balign 32
.Lfifteen:
        .long   15, 15, 15, 15, 15, 15, 15, 15  /* the low 4 bits of each lane */

        .text
        .globl  dot_q4
        .type   dot_q4, @function
dot_q4:
        vpmovzxbd (%rdi), %ymm0                 /* b[0 .. 7] as eight integers   */
        vpand   .Lfifteen(%rip), %ymm0, %ymm1   /* their low 4 bits              */
        vpsrld  $4, %ymm0, %ymm0                /* their high 4 bits             */
        vpmulld (%rsi), %ymm1, %ymm1            /* low  * x[0 .. 7]              */
        vpmulld 64(%rsi), %ymm0, %ymm0          /* high * x[16 .. 23]            */
        vpaddd  %ymm0, %ymm1, %ymm2             /* eight lane sums               */
        vpmovzxbd 8(%rdi), %ymm0                /* b[8 .. 15] as eight integers  */
        vpand   .Lfifteen(%rip), %ymm0, %ymm1   /* their low 4 bits              */
        vpsrld  $4, %ymm0, %ymm0                /* their high 4 bits             */
        vpmulld 32(%rsi), %ymm1, %ymm1          /* low  * x[8 .. 15]             */
        vpmulld 96(%rsi), %ymm0, %ymm0          /* high * x[24 .. 31]            */
        vpaddd  %ymm1, %ymm2, %ymm2             /* into the lane sums            */
        vpaddd  %ymm0, %ymm2, %ymm2             /* into the lane sums            */
        vextracti128 $1, %ymm2, %xmm1           /* the upper four lanes          */
        vpaddd  %xmm1, %xmm2, %xmm2             /* four sums                     */
        vphaddd %xmm2, %xmm2, %xmm2             /* two                           */
        vphaddd %xmm2, %xmm2, %xmm2             /* one                           */
        vmovd   %xmm2, %eax                     /* the result                    */
        ret
        .size   dot_q4, .-dot_q4
        .section .note.GNU-stack,"",@progbits
