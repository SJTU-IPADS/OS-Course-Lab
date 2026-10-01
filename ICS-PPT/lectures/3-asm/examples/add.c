/* y = a + b on three globals: the recap at the end of part one.

     gcc -O0 -fomit-frame-pointer -fcf-protection=none -no-pie add.c -o add
     objdump -d add | sed -n '/<add>:/,/ret/p'

   Globals rather than locals, so a, b and y sit at fixed addresses in the
   data segment and add needs no stack frame. -O0 keeps the two loads apart
   from the add; -fomit-frame-pointer drops the push/pop of %rbp; -no-pie
   gives the file the addresses it will have in memory. */
int a = 1, b = 6, y;

void add(void) {
    y = a + b;
}

int main(void) {
    add();
    return y;
}
