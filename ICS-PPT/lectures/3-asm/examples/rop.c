/* rop.c: join two pieces of code that are already in this program.
 * The stack is not executable, and the input contains no instructions.
 *
 * vuln reads 64 bytes into a 16-byte buffer. With the flags in rop.sh the
 * compiler allocates 24 bytes, so the bytes after the first 24 overwrite the
 * return address. Those bytes are: the address of gadget_pop, the integer 42,
 * and the address of print_num. vuln's ret jumps to gadget_pop; that ret
 * jumps to print_num, which prints the 42 now in %rdi.
 *
 * A short input leaves the return address alone, so vuln returns and main
 * prints "回到 main".
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

asm(
    ".global gadget_pop\n"
    "gadget_pop:\n\t"
    "pop %rdi\n\t"
    "ret\n"
);

void print_num(long n) {
    printf("%ld\n", n);
    exit(0);
}

void vuln(void) {
    char buf[16];
    read(0, buf, 64);
}

int main(void) {
    vuln();
    puts("回到 main");
    return 0;
}
