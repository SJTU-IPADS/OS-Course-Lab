/* libc.h -- the part of the C library mini-ollama uses.

   A guest is built with -nostdlib -nostdinc: none of the host's headers and
   none of its library. guest/mini.h has the system calls write, brk and
   exit, size_t and strlen. This file adds the integer types, the system
   calls fstat and mmap, and the declarations of libc.c and math.c. */
#ifndef LIBC_H
#define LIBC_H

#include "mini.h"                       /* size_t, write, brk, exit, strlen */

#define NULL ((void *)0)

/* The compiler knows the types of its target; these are its names for them. */
typedef __UINT8_TYPE__  uint8_t;
typedef __UINT32_TYPE__ uint32_t;
typedef __UINT64_TYPE__ uint64_t;

typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type)   __builtin_va_arg(ap, type)
#define va_end(ap)         __builtin_va_end(ap)

/* ---------------- system calls ---------------- */

/* What fstat fills in: 144 bytes on x86-64, with the size of the file at
   offset 48. The other fields are not used. */
struct stat {
    char pad0[48];
    long st_size;
    char pad1[88];
};

static inline int fstat(int fd, struct stat *st)
{
    return (int)sys3(5, fd, (long)st, 0);
}

enum { PROT_READ = 1, MAP_PRIVATE = 2 };
#define MAP_FAILED ((void *)-1)

/* Six arguments: the fourth, fifth and sixth go in r10, r8 and r9. The
   kernel returns the address, or a negative error number, which is one of
   the 4095 highest addresses. */
static inline void *mmap(void *addr, size_t len, int prot, int flags, int fd, long off)
{
    register long r10 __asm__("r10") = flags;
    register long r8 __asm__("r8") = fd;
    register long r9 __asm__("r9") = off;
    long r;
    __asm__ volatile ("syscall" : "=a"(r)
                      : "a"(9L), "D"(addr), "S"(len), "d"((long)prot),
                        "r"(r10), "r"(r8), "r"(r9)
                      : "rcx", "r11", "memory");
    return (unsigned long)r > -4096UL ? MAP_FAILED : (void *)r;
}

/* ---------------- libc.c ---------------- */

void *malloc(size_t n);
void *calloc(size_t n, size_t size);
void  free(void *p);

void *memcpy(void *dst, const void *src, size_t n);
void *memmove(void *dst, const void *src, size_t n);
void *memset(void *dst, int c, size_t n);
int   memcmp(const void *a, const void *b, size_t n);
char *strstr(const char *s, const char *sub);
int   atoi(const char *s);

/* The conversions are %s, %.*s, %d, %u, %llu and %%. */
int   snprintf(char *buf, size_t size, const char *fmt, ...);
void  die(const char *fmt, ...);        /* a message on standard error, exit status 1 */

int   putchar(int c);                   /* collects the bytes ... */
void  flush(void);                      /* ... and this writes them */

/* ---------------- math.c ---------------- */

float sqrtf(float x);
float expf(float x);
float logf(float x);
float powf(float x, float y);
float sinf(float x);
float cosf(float x);
float ldexpf(float x, int e);

#endif
