/* stack_exec.c: jump to one instruction stored in a local array.
 * code holds the encoding of ret (0xc3) and is in the stack frame of main.
 * The call jumps to it. With the default build the stack is not executable
 * and the process receives SIGSEGV; linked with -z execstack the stack is
 * executable, ret returns to main and the program exits with status 0.
 * Build: gcc -Og -o stack_exec stack_exec.c
 *        gcc -Og -z execstack -o stack_exec stack_exec.c
 */
int main(void) {
    unsigned char code[] = {0xc3};          /* ret */
    ((void (*)(void))code)();
    return 0;
}
