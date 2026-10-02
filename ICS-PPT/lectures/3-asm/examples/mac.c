/* One multiply-add, compiled alone so that its machine code can be read.

     gcc -Og -fcf-protection=none -c mac.c
     objcopy -O binary -j .text mac.o /dev/stdout | od -An -tx1

   objcopy takes the bytes of the code section out of mac.o, and od prints
   them one byte at a time in hexadecimal. */
int mac(const int *w, const int *x, int sum) {
    return sum + w[0] * x[0];
}
