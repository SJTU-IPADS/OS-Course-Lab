#ifndef UTILS_H
#define UTILS_H

static const char hexchars[] = "0123456789abcdef";

int hex(char ch);
char *mem2hex(char *mem, char *buf, int count);
char *hex2mem(char *buf, char *mem, int count);
int unescape(char *msg, int len);

#endif /* UTILS_H */
