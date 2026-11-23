/* libc.c -- memory, strings and output for mini-ollama.

   Only what the program calls, each in its shortest form. The loops over
   bytes stay loops: the Makefile passes
   -fno-tree-loop-distribute-patterns, without which the compiler would
   turn the loop of memcpy into a call of memcpy. */
#include "libc.h"

/* ---------------- memory ---------------- */

enum { HEAP_STEP = 1 << 20 };           /* the break moves by multiples of this */

static char *heap, *heap_end;           /* the free bytes: heap .. heap_end */

/* size bytes at an address that is a multiple of 16, or NULL. The heap
   starts at the end of the program's data and grows through brk; blocks are
   handed out one after another and never taken back. */
void *malloc(size_t size)
{
    if (!heap_end) {
        heap_end = brk(0);              /* the current break */
        heap = heap_end + (-(size_t)heap_end & 15);
    }
    if (size > (size_t)-1 / 2)
        return NULL;
    size = (size + 15) & ~(size_t)15;
    if (heap + size > heap_end) {
        size_t grow = (size_t)(heap + size - heap_end);
        grow = (grow + HEAP_STEP - 1) / HEAP_STEP * HEAP_STEP;
        if ((char *)brk(heap_end + grow) < heap_end + grow)
            return NULL;                /* the break did not move */
        heap_end += grow;
    }
    void *block = heap;
    heap += size;
    return block;
}

/* The memory brk adds is zero and no block is handed out twice, so there
   is nothing to clear. */
void *calloc(size_t count, size_t size)
{
    if (size && count > (size_t)-1 / size)
        return NULL;
    return malloc(count * size);
}

void free(void *ptr)
{
    (void)ptr;
}

/* ---------------- bytes and strings ---------------- */

void *memcpy(void *dst, const void *src, size_t count)
{
    char *to = dst;
    const char *from = src;
    for (size_t pos = 0; pos < count; pos++)
        to[pos] = from[pos];
    return dst;
}

/* As memcpy, for blocks that overlap: when dst lies above src, the last
   byte is copied first. */
void *memmove(void *dst, const void *src, size_t count)
{
    char *to = dst;
    const char *from = src;
    if (to <= from)
        return memcpy(dst, src, count);
    while (count > 0) {
        count--;
        to[count] = from[count];
    }
    return dst;
}

void *memset(void *dst, int ch, size_t count)
{
    char *to = dst;
    for (size_t pos = 0; pos < count; pos++)
        to[pos] = (char)ch;
    return dst;
}

/* Negative, zero or positive as left[0..count) comes before, equals or
   comes after right[0..count). */
int memcmp(const void *left, const void *right, size_t count)
{
    const unsigned char *left_bytes = left, *right_bytes = right;
    for (size_t pos = 0; pos < count; pos++)
        if (left_bytes[pos] != right_bytes[pos])
            return left_bytes[pos] - right_bytes[pos];
    return 0;
}

/* Where sub first occurs in str, or NULL. The comparison stops at the first
   byte that differs, which is at the latest the end of str. */
char *strstr(const char *str, const char *sub)
{
    size_t len = strlen(sub);
    for (; *str; str++)
        if (memcmp(str, sub, len) == 0)
            return (char *)str;
    return len ? NULL : (char *)str;
}

int atoi(const char *str)
{
    unsigned value = 0;
    int neg = 0;
    while (*str == ' ')
        str++;
    if (*str == '-' || *str == '+')
        neg = *str++ == '-';
    for (; *str >= '0' && *str <= '9'; str++)
        value = value * 10 + (unsigned)(*str - '0');
    return neg ? -(int)value : (int)value;
}

/* ---------------- formatted text ---------------- */

struct text {
    char *next, *end;                   /* the next byte, the end of the room */
};

static void put(struct text *text, const char *str, size_t len)
{
    for (size_t pos = 0; pos < len && text->next < text->end; pos++)
        *text->next++ = str[pos];
}

static void put_number(struct text *text, unsigned long long value, int neg)
{
    char digit[24];
    int first = sizeof digit;
    do {
        digit[--first] = (char)('0' + value % 10);
        value /= 10;
    } while (value);
    if (neg)
        digit[--first] = '-';
    put(text, digit + first, sizeof digit - (size_t)first);
}

/* fmt into buf[0..size) with the arguments in place of the conversions
   %s, %.*s, %d, %u and %llu; text that does not fit is cut off. The result
   is terminated; its length is returned. size is at least 1. */
static int format(char *buf, size_t size, const char *fmt, va_list ap)
{
    struct text text = { buf, buf + size - 1 };

    for (; *fmt; fmt++) {
        if (fmt[0] != '%') {
            put(&text, fmt, 1);
        } else if (fmt[1] == 's') {
            const char *str = va_arg(ap, const char *);
            put(&text, str, strlen(str));
            fmt += 1;
        } else if (fmt[1] == '.' && fmt[2] == '*' && fmt[3] == 's') {
            int len = va_arg(ap, int);  /* the length comes first */
            const char *str = va_arg(ap, const char *);
            put(&text, str, len > 0 ? (size_t)len : 0);
            fmt += 3;
        } else if (fmt[1] == 'd') {
            int value = va_arg(ap, int);
            put_number(&text, value < 0 ? -(unsigned long long)value
                                        : (unsigned long long)value,
                       value < 0);
            fmt += 1;
        } else if (fmt[1] == 'u') {
            put_number(&text, va_arg(ap, unsigned), 0);
            fmt += 1;
        } else if (fmt[1] == 'l' && fmt[2] == 'l' && fmt[3] == 'u') {
            put_number(&text, va_arg(ap, unsigned long long), 0);
            fmt += 3;
        } else if (fmt[1] == '%') {
            put(&text, fmt, 1);
            fmt += 1;
        } else {
            put(&text, fmt, 1);         /* not a conversion: as it is */
        }
    }
    *text.next = 0;
    return (int)(text.next - buf);
}

/* Returns the length of what was written, which is at most size - 1. */
int snprintf(char *buf, size_t size, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int len = format(buf, size, fmt, ap);
    va_end(ap);
    return len;
}

void die(const char *fmt, ...)
{
    static const char name[] = "mini-ollama: ";
    char buf[256];
    size_t len = sizeof name - 1;
    va_list ap;

    memcpy(buf, name, len);
    va_start(ap, fmt);
    len += (size_t)format(buf + len, sizeof buf - len - 1, fmt, ap);
    va_end(ap);
    buf[len++] = '\n';
    write(2, buf, len);
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

int putchar(int ch)
{
    if (n_out == sizeof out)
        flush();
    out[n_out++] = (char)ch;
    return ch;
}
