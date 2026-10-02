/* Calls times5 and times12 from times.c with x = 7.
 * Build: gcc -Og -fcf-protection=none times.c times_main.c -o times
 */
#include <stdio.h>

int times5(int x);
int times12(int x);

int main(void) {
    printf("times5(7) = %d, times12(7) = %d\n", times5(7), times12(7));
    return 0;
}
