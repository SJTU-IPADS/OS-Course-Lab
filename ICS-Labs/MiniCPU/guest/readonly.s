# guest/readonly.s: a store to a page without write permission.
#
# The loader maps the code of a program without PTE_W. Reading it is
# allowed; the store below is a page fault at the address of _start, the
# third instruction of the program. The page is in the TLB by then, so the
# permission has to be checked on a TLB hit as well.
#
# Assembled without libc: gcc -nostdlib -no-pie.

        .globl  _start
        .text
_start:
        mov     _start(%rip),%eax       # a read of the code page
        mov     %eax,copy(%rip)
        movl    $0,_start(%rip)         # page fault
        mov     $60,%eax
        xor     %edi,%edi
        syscall

        .bss
copy:   .long   0
