/* The only runtime of the guests. There is no libc; system calls are
 * inline assembly.
 *
 * The convention is that of Linux x86-64: the number in rax, the
 * arguments in rdi rsi rdx r10 r8 r9, the result in rax. The syscall
 * instruction overwrites rcx and r11, so both are in the clobber list.
 */
#ifndef MINI_H
#define MINI_H

typedef unsigned long size_t;
typedef long ssize_t;

static inline long sys1(long n, long a)
{
    long r;
    __asm__ volatile ("syscall" : "=a"(r) : "a"(n), "D"(a)
                      : "rcx", "r11", "memory");
    return r;
}

static inline long sys3(long n, long a, long b, long c)
{
    long r;
    __asm__ volatile ("syscall" : "=a"(r) : "a"(n), "D"(a), "S"(b), "d"(c)
                      : "rcx", "r11", "memory");
    return r;
}

static inline ssize_t write(int fd, const void *p, size_t n)
{
    return sys3(1, fd, (long)p, (long)n);
}

static inline int open(const char *path)      { return (int)sys3(2, (long)path, 0, 0); }
static inline ssize_t read(int fd, void *p, size_t n)
{
    return sys3(0, fd, (long)p, (long)n);
}
static inline int close(int fd)               { return (int)sys1(3, fd); }
static inline void *brk(void *p)              { return (void *)sys1(12, (long)p); }
static inline int fsync(int fd)               { return (int)sys1(74, fd); }  /* writable ramfs extension only */
static inline void exit(int code)             { sys1(60, code); __builtin_unreachable(); }

static inline size_t strlen(const char *s)
{
    size_t n = 0;
    while (s[n]) ++n;
    return n;
}

static inline void puts(const char *s) { write(1, s, strlen(s)); }

/* Decimal output; needs nothing beyond the division instruction. */
static inline void putint(long v)
{
    char buf[24];
    int i = 24;
    int neg = v < 0;
    unsigned long u = neg ? (unsigned long)-v : (unsigned long)v;
    buf[--i] = '\n';
    do { buf[--i] = (char)('0' + u % 10); u /= 10; } while (u);
    if (neg) buf[--i] = '-';
    write(1, buf + i, (size_t)(24 - i));
}

int guest_main(int argc, char **argv);

#endif
