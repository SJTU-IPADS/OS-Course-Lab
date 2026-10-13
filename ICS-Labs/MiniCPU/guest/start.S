/* Entry of every guest. The loader lays out the stack as the System V ABI
 * does:
 *     (%rsp) = argc, 8(%rsp) = argv[0], ...
 * argc and argv are passed to guest_main, and its return value to exit. */
        .text
        .globl _start
_start:
        movq    (%rsp), %rdi            /* argc */
        leaq    8(%rsp), %rsi           /* argv */
        andq    $-16, %rsp
        call    guest_main
        movl    %eax, %edi
        movl    $60, %eax               /* exit */
        syscall
        hlt
