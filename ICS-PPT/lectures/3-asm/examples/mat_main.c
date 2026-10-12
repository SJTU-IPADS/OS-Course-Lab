/* main keeps a 3 x 4 matrix and two vectors in its stack frame.

     gcc -Og -fcf-protection=none -fno-stack-protector -S mat_main.c -o - | sed -f asm.sed
     gcc -Og -o mat_main mat_main.c matvec.c dot.c && ./mat_main; echo $?     prints 22

   y = {70, 174, 278}; the exit status is y[2] modulo 256. */
void matvec(int *y, int W[3][4], const int *x);

int main(void) {
    int W[3][4] = {{1, 2, 3, 4}, {5, 6, 7, 8}, {9, 10, 11, 12}};
    int x[4] = {5, 6, 7, 8};
    int y[3];
    matvec(y, W, x);
    return y[2];
}
