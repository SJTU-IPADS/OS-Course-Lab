#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

void* malloc(size_t size) {
    static void* (*real_malloc)(size_t) = NULL;
    if (!real_malloc) {
        real_malloc = dlsym(RTLD_NEXT, "malloc");
    }
    void* ptr = real_malloc(size);
    fprintf(stderr, "[INTERCEPT] malloc(%zu) = %p\n", size, ptr);
    return ptr;
}

void free(void* ptr) {
    static void (*real_free)(void*) = NULL;
    if (!real_free) {
        real_free = dlsym(RTLD_NEXT, "free");
    }
    fprintf(stderr, "[INTERCEPT] free(%p)\n", ptr);
    real_free(ptr);
}

void* realloc(void* ptr, size_t size) {
    static void* (*real_realloc)(void*, size_t) = NULL;
    if (!real_realloc) {
        real_realloc = dlsym(RTLD_NEXT, "realloc");
    }
    void* new_ptr = real_realloc(ptr, size);
    fprintf(stderr, "[INTERCEPT] realloc(%p, %zu) = %p\n", ptr, size, new_ptr);
    return new_ptr;
}
