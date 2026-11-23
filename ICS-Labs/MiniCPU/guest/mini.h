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

static inline long sys1(long number, long arg1)
{
    long result;
    __asm__ volatile ("syscall" : "=a"(result) : "a"(number), "D"(arg1)
                      : "rcx", "r11", "memory");
    return result;
}

static inline long sys3(long number, long arg1, long arg2, long arg3)
{
    long result;
    __asm__ volatile ("syscall" : "=a"(result)
                      : "a"(number), "D"(arg1), "S"(arg2), "d"(arg3)
                      : "rcx", "r11", "memory");
    return result;
}

static inline ssize_t write(int fd, const void *buf, size_t count)
{
    return sys3(1, fd, (long)buf, (long)count);
}

static inline int open(const char *path)      { return (int)sys3(2, (long)path, 0, 0); }
static inline ssize_t read(int fd, void *buf, size_t count)
{
    return sys3(0, fd, (long)buf, (long)count);
}
static inline int close(int fd)               { return (int)sys1(3, fd); }
static inline void *brk(void *addr)           { return (void *)sys1(12, (long)addr); }
static inline int fsync(int fd)               { return (int)sys1(74, fd); }  /* writable ramfs extension only */
static inline void exit(int code)             { sys1(60, code); __builtin_unreachable(); }

static inline size_t strlen(const char *str)
{
    size_t len = 0;
    while (str[len]) ++len;
    return len;
}

static inline void puts(const char *str) { write(1, str, strlen(str)); }

/* Decimal output; needs nothing beyond the division instruction. */
static inline void putint(long value)
{
    char buf[24];
    int start = 24;
    int neg = value < 0;
    unsigned long magnitude = neg ? (unsigned long)-value
                                  : (unsigned long)value;
    buf[--start] = '\n';
    do {
        buf[--start] = (char)('0' + magnitude % 10);
        magnitude /= 10;
    } while (magnitude);
    if (neg) buf[--start] = '-';
    write(1, buf + start, (size_t)(24 - start));
}

int guest_main(int argc, char **argv);

#endif
