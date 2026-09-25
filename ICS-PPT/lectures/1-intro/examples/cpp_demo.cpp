// Two ways to print one line, from two different libraries. That is the whole
// point of the program: `ldd` finds libstdc++ *and* libc behind it, because
// `std::cout` is implemented in the first and `printf` in the second.
// On macOS the same two layers appear as libc++ and libSystem; list them
// with `otool -L ./cpp_demo`, which plays the role of `ldd` there.
// On Windows the layers are libstdc++-6.dll and msvcrt.dll; `ldd` exists in
// MSYS2, and the native toolchain lists them with `objdump -p` or `dumpbin`.
#include <cstdio>
#include <iostream>

int main(void)
{
    printf("printf comes from libc\n");
    std::cout << "cout comes from libstdc++" << std::endl;
    return 0;
}
