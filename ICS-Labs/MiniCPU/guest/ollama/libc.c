/* libc.c -- memory, strings and output for mini-ollama.

   Only what the program calls, each in its shortest form. The loops over
   bytes stay loops: the Makefile passes
   -fno-tree-loop-distribute-patterns, without which the compiler would
   turn the loop of memcpy into a call of memcpy. */
#include "libc.h"

/* ---------------- memory ---------------- */

enum { HEAP_STEP = 1 << 20 };           /* the break moves by multiples of this */

static char *heap, *heap_end;           /* the free bytes: heap .. heap_end */

/* n bytes at an address that is a multiple of 16, or NULL. The heap starts
   at the end of the program's data and grows through brk; blocks are handed
   out one after another and never taken back. */
void *malloc(size_t n)
{
    if (!heap_end) {
        heap_end = brk(0);              /* the current break */
        heap = heap_end + (-(size_t)heap_end & 15);
    }
    if (n > (size_t)-1 / 2)
        return NULL;
    n = (n + 15) & ~(size_t)15;
    if (heap + n > heap_end) {
        size_t grow = (size_t)(heap + n - heap_end);
        grow = (grow + HEAP_STEP - 1) / HEAP_STEP * HEAP_STEP;
        if ((char *)brk(heap_end + grow) < heap_end + grow)
            return NULL;                /* the break did not move */
        heap_end += grow;
    }
    void *p = heap;
    heap += n;
    return p;
}

/* The memory brk adds is zero and no block is handed out twice, so there
   is nothing to clear. */
void *calloc(size_t n, size_t size)
{
    if (size && n > (size_t)-1 / size)
        return NULL;
    return malloc(n * size);
}

void free(void *p)
{
    (void)p;
}

/* ---------------- bytes and strings ---------------- */

void *memcpy(void *dst, const void *src, size_t n)
{
    char *d = dst;
    const char *s = src;
    for (size_t i = 0; i < n; i++)
        d[i] = s[i];
    return dst;
}

/* As memcpy, for blocks that overlap: when dst lies above src, the last
   byte is copied first. */
void *memmove(void *dst, const void *src, size_t n)
{
    char *d = dst;
    const char *s = src;
    if (d <= s)
        return memcpy(dst, src, n);
    while (n > 0) {
        n--;
        d[n] = s[n];
    }
    return dst;
}

void *memset(void *dst, int c, size_t n)
{
    char *d = dst;
    for (size_t i = 0; i < n; i++)
        d[i] = (char)c;
    return dst;
}

/* Negative, zero or positive as a[0..n) comes before, equals or comes
   after b[0..n). */
int memcmp(const void *a, const void *b, size_t n)
{
    const unsigned char *p = a, *q = b;
    for (size_t i = 0; i < n; i++)
        if (p[i] != q[i])
            return p[i] - q[i];
    return 0;
}

/* Where sub first occurs in s, or NULL. The comparison stops at the first
   byte that differs, which is at the latest the end of s. */
char *strstr(const char *s, const char *sub)
{
    size_t n = strlen(sub);
    for (; *s; s++)
        if (memcmp(s, sub, n) == 0)
            return (char *)s;
    return n ? NULL : (char *)s;
}

int atoi(const char *s)
{
    unsigned v = 0;
    int neg = 0;
    while (*s == ' ')
        s++;
    if (*s == '-' || *s == '+')
        neg = *s++ == '-';
    for (; *s >= '0' && *s <= '9'; s++)
        v = v * 10 + (unsigned)(*s - '0');
    return neg ? -(int)v : (int)v;
}

/* ---------------- formatted text ---------------- */

struct text {
    char *p, *end;                      /* the next byte, the end of the room */
};

static void put(struct text *t, const char *s, size_t n)
{
    for (size_t i = 0; i < n && t->p < t->end; i++)
        *t->p++ = s[i];
}

static void put_number(struct text *t, unsigned long long v, int neg)
{
    char digit[24];
    int i = sizeof digit;
    do {
        digit[--i] = (char)('0' + v % 10);
        v /= 10;
    } while (v);
    if (neg)
        digit[--i] = '-';
    put(t, digit + i, sizeof digit - (size_t)i);
}

/* fmt into buf[0..size) with the arguments in place of the conversions
   %s, %.*s, %d, %u and %llu; text that does not fit is cut off. The result
   is terminated; its length is returned. size is at least 1. */
static int format(char *buf, size_t size, const char *fmt, va_list ap)
{
    struct text t = { buf, buf + size - 1 };

    for (; *fmt; fmt++) {
        if (fmt[0] != '%') {
            put(&t, fmt, 1);
        } else if (fmt[1] == 's') {
            const char *s = va_arg(ap, const char *);
            put(&t, s, strlen(s));
            fmt += 1;
        } else if (fmt[1] == '.' && fmt[2] == '*' && fmt[3] == 's') {
            int n = va_arg(ap, int);    /* the length comes first */
            const char *s = va_arg(ap, const char *);
            put(&t, s, n > 0 ? (size_t)n : 0);
            fmt += 3;
        } else if (fmt[1] == 'd') {
            int v = va_arg(ap, int);
            put_number(&t, v < 0 ? -(unsigned long long)v : (unsigned long long)v, v < 0);
            fmt += 1;
        } else if (fmt[1] == 'u') {
            put_number(&t, va_arg(ap, unsigned), 0);
            fmt += 1;
        } else if (fmt[1] == 'l' && fmt[2] == 'l' && fmt[3] == 'u') {
            put_number(&t, va_arg(ap, unsigned long long), 0);
            fmt += 3;
        } else if (fmt[1] == '%') {
            put(&t, fmt, 1);
            fmt += 1;
        } else {
            put(&t, fmt, 1);            /* not a conversion: as it is */
        }
    }
    *t.p = 0;
    return (int)(t.p - buf);
}

/* Returns the length of what was written, which is at most size - 1. */
int snprintf(char *buf, size_t size, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = format(buf, size, fmt, ap);
    va_end(ap);
    return n;
}

void die(const char *fmt, ...)
{
    static const char name[] = "mini-ollama: ";
    char buf[256];
    size_t n = sizeof name - 1;
    va_list ap;

    memcpy(buf, name, n);
    va_start(ap, fmt);
    n += (size_t)format(buf + n, sizeof buf - n - 1, fmt, ap);
    va_end(ap);
    buf[n++] = '\n';
    write(2, buf, n);
    exit(1);
}

/* ---------------- output ---------------- */

static char   out[256];
static size_t n_out;

/* Write the bytes putchar has collected, in one system call. */
void flush(void)
{
    if (n_out > 0)
        write(1, out, n_out);
    n_out = 0;
}

int putchar(int c)
{
    if (n_out == sizeof out)
        flush();
    out[n_out++] = (char)c;
    return c;
}
