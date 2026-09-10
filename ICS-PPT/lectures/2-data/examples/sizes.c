/* How many bytes each C type occupies on this machine. */
#include <stdio.h>
#include <stdint.h>

int main(void) {
    printf("char %zu  short %zu  int %zu  long %zu  void* %zu\n",
           sizeof(char), sizeof(short), sizeof(int), sizeof(long), sizeof(void *));
    printf("float %zu  double %zu  size_t %zu  int32_t %zu  int64_t %zu\n",
           sizeof(float), sizeof(double), sizeof(size_t),
           sizeof(int32_t), sizeof(int64_t));
    return 0;
}
