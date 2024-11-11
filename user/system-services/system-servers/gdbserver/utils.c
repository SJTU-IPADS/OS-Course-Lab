#include "utils.h"

int hex(char ch)
{
    if ((ch >= 'a') && (ch <= 'f'))
        return (ch - 'a' + 10);
    if ((ch >= '0') && (ch <= '9'))
        return (ch - '0');
    if ((ch >= 'A') && (ch <= 'F'))
        return (ch - 'A' + 10);
    return (-1);
}

char *mem2hex(char *mem, char *buf, int count)
{
    unsigned char ch;
    for (int i = 0; i < count; i++)
    {
        ch = *(mem++);
        *buf++ = hexchars[ch >> 4];
        *buf++ = hexchars[ch % 16];
    }
    *buf = 0;
    return (buf);
}

char *hex2mem(char *buf, char *mem, int count)
{
    unsigned char ch;
    for (int i = 0; i < count; i++)
    {
        ch = hex(*buf++) << 4;
        ch = ch + hex(*buf++);
        *(mem++) = ch;
    }
    return (mem);
}

int unescape(char *msg, int len)
{
    char *w = msg, *r = msg;
    while (r - msg < len)
    {
        char v = *r++;
        if (v != '}')
        {
            *w++ = v;
            continue;
        }
        *w++ = *r++ ^ 0x20;
    }
    return w - msg;
}
