/* linear_scalar and linear_simd: b + w[0] x[0] + ... + w[n-1] x[n-1].

   linear_scalar  one element per iteration, what the C compiles to
   linear_simd    eight elements per iteration on 256-bit YMM registers

   Both take (const int *w, const int *x, int b, int n) in %rdi, %rsi, %edx,
   %ecx and return the sum in %eax. guest/simd.c and guest/tinytf.c call
   them.

   The loop of linear_simd uses the four vector instructions of the AVX2
   loop in the SIMD section of lecture 3 (dot_product_avx2 in
   lectures/3-asm/examples/dot_intrin.c): vpxor, vmovdqu, vpmulld, vpaddd.
   The lecture adds the eight lane sums in C (sum_lanes); here the
   reduction is written out with vextracti128, vpaddd, vphaddd and vmovd,
   which brings the vector subset of lab 1 to seven instructions. There is
   no vzeroupper: on a real processor it removes a delay in SSE instructions
   that follow; mini-cpu has neither that delay nor the instruction. */
        .text
        .globl  linear_scalar
        .type   linear_scalar, @function
linear_scalar:
        movl    %edx, %eax              /* acc = b            */
        xorl    %r8d, %r8d              /* i = 0              */
.Lloop:
        cmpl    %ecx, %r8d              /* i < n ?            */
        jge     .Ldone
        movl    (%rdi,%r8,4), %r9d      /* w[i]               */
        imull   (%rsi,%r8,4), %r9d      /* w[i] * x[i]        */
        addl    %r9d, %eax              /* acc += that        */
        incq    %r8
        jmp     .Lloop
.Ldone:
        ret
        .size   linear_scalar, .-linear_scalar

        .globl  linear_simd
        .type   linear_simd, @function
linear_simd:
        movl    %edx, %eax              /* acc = b            */
        xorl    %r8d, %r8d              /* i = 0              */
        vpxor   %ymm2, %ymm2, %ymm2     /* eight lane sums, all zero */
.Lvec:
        movl    %ecx, %r9d
        subl    %r8d, %r9d              /* how many are left  */
        cmpl    $8, %r9d
        jl      .Lreduce                /* fewer than 8: stop vectorising */
        vmovdqu (%rdi,%r8,4), %ymm0     /* w[i .. i+7]        */
        vmovdqu (%rsi,%r8,4), %ymm1     /* x[i .. i+7]        */
        vpmulld %ymm1, %ymm0, %ymm0     /* eight products     */
        vpaddd  %ymm0, %ymm2, %ymm2     /* into the lane sums */
        addq    $8, %r8
        jmp     .Lvec
.Lreduce:                               /* eight lane sums -> one number */
        vextracti128 $1, %ymm2, %xmm1   /* the upper four lanes */
        vpaddd  %xmm1, %xmm2, %xmm2     /* four sums           */
        vphaddd %xmm2, %xmm2, %xmm2     /* two                 */
        vphaddd %xmm2, %xmm2, %xmm2     /* one                 */
        vmovd   %xmm2, %r9d
        addl    %r9d, %eax
.Ltail:                                 /* the leftover elements, one by one */
        cmpl    %ecx, %r8d
        jge     .Ldone2
        movl    (%rdi,%r8,4), %r9d
        imull   (%rsi,%r8,4), %r9d
        addl    %r9d, %eax
        incq    %r8
        jmp     .Ltail
.Ldone2:
        ret
        .size   linear_simd, .-linear_simd
        .section .note.GNU-stack,"",@progbits
