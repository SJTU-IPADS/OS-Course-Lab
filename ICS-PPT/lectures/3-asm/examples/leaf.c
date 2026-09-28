/* A leaf function whose locals live below %rsp, in the red zone.

     gcc -Og -fcf-protection=none -S leaf.c -o -

   volatile forces x and y into memory; the function calls nothing, so
   gcc stores them under %rsp without moving it. */
int leaf_example(int a, int b) {
    volatile int x = a, y = b;
    return x + y;
}
