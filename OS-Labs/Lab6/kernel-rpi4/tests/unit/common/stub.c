#include <stdlib.h>

void *kmalloc(size_t size)
{
        return calloc(1, size);
}

void *kzalloc(size_t size)
{
        return calloc(1, size);
}

void kfree(void *ptr)
{
        free(ptr);
}