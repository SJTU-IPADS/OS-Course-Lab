// Two ways to print one line, from two different libraries. That is the whole
// point of the program: `ldd` finds libstdc++ *and* libc behind it, because
// `std::cout` is implemented in the first and `printf` in the second.
#include <cstdio>
#include <iostream>

int main(void)
{
    printf("printf comes from libc\n");
    std::cout << "cout comes from libstdc++" << std::endl;
    return 0;
}
