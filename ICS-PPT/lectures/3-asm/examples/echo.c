/* echo.c: read one line into an 8-byte buffer and write it back.
 * gets is the implementation of the library function shown in the lecture:
 * it receives the address of the buffer and never its size, so a line of
 * more than 7 characters is written past the end of buf, into the rest of
 * the stack frame of echo and then over the return address.
 * Build: gcc -Og -fcf-protection=none -fno-stack-protector -o echo echo.c
 */
#include <stdio.h>

/* Implementation of library function gets() */
char *gets(char *s) {
    int c;
    char *dest = s;
    while ((c = getchar()) != '\n' && c != EOF)
        *dest++ = c;                /* No bounds checking */
    if (c == EOF && dest == s)
        return NULL;                /* No characters read */
    *dest++ = '\0';                 /* Terminate string */
    return s;
}

/* Read input line and write it back */
void echo(void) {
    char buf[8];                    /* Way too small! */
    gets(buf);
    puts(buf);
}

int main(void) {
    echo();
    return 0;
}
