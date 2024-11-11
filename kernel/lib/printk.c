/*
 * Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
 * Licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

/*
 * BSD-3-Clause License
 *
 * Copyright (c) 2016, Matt Redfearn
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <io/uart.h>
#include <lib/printk.h>
#include <common/types.h>

#define PRINT_BUF_LEN 64

typedef __builtin_va_list va_list;
#define va_start(v, l) __builtin_va_start(v, l)
#define va_end(v)      __builtin_va_end(v)
#define va_arg(v, l)   __builtin_va_arg(v, l)
#define va_copy(d, s)  __builtin_va_copy(d, s)

static void simple_outputchar(char **str, char c)
{
        if (str) {
                **str = c;
                ++(*str);
        } else {
                uart_send(c);
        }
}

enum flags { PAD_ZERO = 1, PAD_RIGHT = 2 };

static int prints(char **out, const char *string, int width, int flags)
{
        int pc = 0, padchar = ' ';

        if (width > 0) {
                int len = 0;
                const char *ptr;
                for (ptr = string; *ptr; ++ptr)
                        ++len;
                if (len >= width)
                        width = 0;
                else
                        width -= len;
                if (flags & PAD_ZERO)
                        padchar = '0';
        }
        if (!(flags & PAD_RIGHT)) {
                for (; width > 0; --width) {
                        simple_outputchar(out, padchar);
                        ++pc;
                }
        }
        for (; *string; ++string) {
                simple_outputchar(out, *string);
                ++pc;
        }
        for (; width > 0; --width) {
                simple_outputchar(out, padchar);
                ++pc;
        }

        return pc;
}

static int simple_outputi(char **out, long i, int base, int sign, int width,
                          int flags, int letbase)
{
        char print_buf[PRINT_BUF_LEN];
        char *s;
        int t, neg = 0, pc = 0;
        unsigned long u = i;

        if (i == 0) {
                print_buf[0] = '0';
                print_buf[1] = '\0';
                return prints(out, print_buf, width, flags);
        }

        if (sign && base == 10 && i < 0) {
                neg = 1;
                u = -i;
        }

        s = print_buf + PRINT_BUF_LEN - 1;
        *s = '\0';

        while (u) {
                t = u % base;
                if (t >= 10)
                        t += letbase - '0' - 10;
                *--s = t + '0';
                u /= base;
        }

        if (neg) {
                if (width && (flags & PAD_ZERO)) {
                        simple_outputchar(out, '-');
                        ++pc;
                        --width;
                } else {
                        *--s = '-';
                }
        }

        return pc + prints(out, s, width, flags);
}

static int simple_vsprintf(char **out, const char *format, va_list ap)
{
        int width, flags;
        int pc = 0;
        char scr[2];
        union {
                char c;
                char *s;
                int i;
                unsigned int u;
                long li;
                unsigned long lu;
                long long lli;
                unsigned long long llu;
                short hi;
                unsigned short hu;
                signed char hhi;
                unsigned char hhu;
                void *p;
        } u;

        for (; *format != 0; ++format) {
                if (*format == '%') {
                        ++format;
                        width = flags = 0;
                        if (*format == '\0')
                                break;
                        if (*format == '%')
                                goto out;
                        if (*format == '-') {
                                ++format;
                                flags = PAD_RIGHT;
                        }
                        while (*format == '0') {
                                ++format;
                                flags |= PAD_ZERO;
                        }
                        if (*format == '*') {
                                width = va_arg(ap, int);
                                format++;
                        } else {
                                for (; *format >= '0' && *format <= '9';
                                     ++format) {
                                        width *= 10;
                                        width += *format - '0';
                                }
                        }
                        switch (*format) {
                        case ('d'):
                                u.i = va_arg(ap, int);
                                pc += simple_outputi(
                                        out, u.i, 10, 1, width, flags, 'a');
                                break;

                        case ('b'):
                                u.i = va_arg(ap, int);
                                pc += simple_outputi(
                                        out, u.i, 2, 1, width, flags, 'a');
                                break;

                        case ('u'):
                                u.u = va_arg(ap, unsigned int);
                                pc += simple_outputi(
                                        out, u.u, 10, 0, width, flags, 'a');
                                break;

                        case ('p'):
                                u.llu = va_arg(ap, unsigned long);
                                pc += simple_outputi(
                                        out, u.llu, 16, 0, width, flags, 'a');
                                break;

                        case ('x'):
                                u.u = va_arg(ap, unsigned int);
                                pc += simple_outputi(
                                        out, u.u, 16, 0, width, flags, 'a');
                                break;

                        case ('X'):
                                u.u = va_arg(ap, unsigned int);
                                pc += simple_outputi(
                                        out, u.u, 16, 0, width, flags, 'A');
                                break;

                        case ('c'):
                                u.c = va_arg(ap, int);
                                scr[0] = u.c;
                                scr[1] = '\0';
                                pc += prints(out, scr, width, flags);
                                break;

                        case ('s'):
                                u.s = va_arg(ap, char *);
                                pc += prints(out,
                                             u.s ? u.s : "(null)",
                                             width,
                                             flags);
                                break;
                        case ('l'):
                                ++format;
                                switch (*format) {
                                case ('d'):
                                        u.li = va_arg(ap, long);
                                        pc += simple_outputi(out,
                                                             u.li,
                                                             10,
                                                             1,
                                                             width,
                                                             flags,
                                                             'a');
                                        break;

                                case ('u'):
                                        u.lu = va_arg(ap, unsigned long);
                                        pc += simple_outputi(out,
                                                             u.lu,
                                                             10,
                                                             0,
                                                             width,
                                                             flags,
                                                             'a');
                                        break;

                                case ('x'):
                                        u.lu = va_arg(ap, unsigned long);
                                        pc += simple_outputi(out,
                                                             u.lu,
                                                             16,
                                                             0,
                                                             width,
                                                             flags,
                                                             'a');
                                        break;

                                case ('X'):
                                        u.lu = va_arg(ap, unsigned long);
                                        pc += simple_outputi(out,
                                                             u.lu,
                                                             16,
                                                             0,
                                                             width,
                                                             flags,
                                                             'A');
                                        break;

                                case ('l'):
                                        ++format;
                                        switch (*format) {
                                        case ('d'):
                                                u.lli = va_arg(ap, long long);
                                                pc += simple_outputi(out,
                                                                     u.lli,
                                                                     10,
                                                                     1,
                                                                     width,
                                                                     flags,
                                                                     'a');
                                                break;

                                        case ('u'):
                                                u.llu = va_arg(
                                                        ap, unsigned long long);
                                                pc += simple_outputi(out,
                                                                     u.llu,
                                                                     10,
                                                                     0,
                                                                     width,
                                                                     flags,
                                                                     'a');
                                                break;

                                        case ('x'):
                                                u.llu = va_arg(
                                                        ap, unsigned long long);
                                                pc += simple_outputi(out,
                                                                     u.llu,
                                                                     16,
                                                                     0,
                                                                     width,
                                                                     flags,
                                                                     'a');
                                                break;

                                        case ('X'):
                                                u.llu = va_arg(
                                                        ap, unsigned long long);
                                                pc += simple_outputi(out,
                                                                     u.llu,
                                                                     16,
                                                                     0,
                                                                     width,
                                                                     flags,
                                                                     'A');
                                                break;

                                        default:
                                                break;
                                        }
                                        break;
                                default:
                                        break;
                                }
                                break;
                        case ('h'):
                                ++format;
                                switch (*format) {
                                case ('d'):
                                        u.hi = va_arg(ap, int);
                                        pc += simple_outputi(out,
                                                             u.hi,
                                                             10,
                                                             1,
                                                             width,
                                                             flags,
                                                             'a');
                                        break;

                                case ('u'):
                                        u.hu = va_arg(ap, unsigned int);
                                        pc += simple_outputi(out,
                                                             u.lli,
                                                             10,
                                                             0,
                                                             width,
                                                             flags,
                                                             'a');
                                        break;

                                case ('x'):
                                        u.hu = va_arg(ap, unsigned int);
                                        pc += simple_outputi(out,
                                                             u.lli,
                                                             16,
                                                             0,
                                                             width,
                                                             flags,
                                                             'a');
                                        break;

                                case ('X'):
                                        u.hu = va_arg(ap, unsigned int);
                                        pc += simple_outputi(out,
                                                             u.lli,
                                                             16,
                                                             0,
                                                             width,
                                                             flags,
                                                             'A');
                                        break;

                                case ('h'):
                                        ++format;
                                        switch (*format) {
                                        case ('d'):
                                                u.hhi = va_arg(ap, int);
                                                pc += simple_outputi(out,
                                                                     u.hhi,
                                                                     10,
                                                                     1,
                                                                     width,
                                                                     flags,
                                                                     'a');
                                                break;

                                        case ('u'):
                                                u.hhu = va_arg(ap,
                                                               unsigned int);
                                                pc += simple_outputi(out,
                                                                     u.lli,
                                                                     10,
                                                                     0,
                                                                     width,
                                                                     flags,
                                                                     'a');
                                                break;

                                        case ('x'):
                                                u.hhu = va_arg(ap,
                                                               unsigned int);
                                                pc += simple_outputi(out,
                                                                     u.lli,
                                                                     16,
                                                                     0,
                                                                     width,
                                                                     flags,
                                                                     'a');
                                                break;

                                        case ('X'):
                                                u.hhu = va_arg(ap,
                                                               unsigned int);
                                                pc += simple_outputi(out,
                                                                     u.lli,
                                                                     16,
                                                                     0,
                                                                     width,
                                                                     flags,
                                                                     'A');
                                                break;

                                        default:
                                                break;
                                        }
                                        break;
                                default:
                                        break;
                                }
                                break;
                        default:
                                break;
                        }
                } else {
                out:
                        if (*format == '\n')
                                simple_outputchar(out, '\r');
                        simple_outputchar(out, *format);
                        ++pc;
                }
        }
        if (out)
                **out = '\0';
        return pc;
}

#ifdef CHCORE_OPENTRUSTEE
int simple_sprintf(char *str, const char *fmt, ...)
{
        va_list va;
        int count;
        
        va_start(va, fmt);
        count = simple_vsprintf(&str, fmt, va);
        va_end(va);

        return count;
}

#define BUF_SIZE 512
extern bool is_tlogger_on(void);
extern int append_chcore_log(const char *str, size_t len, bool is_kernel);
#endif /* CHCORE_OPENTRUSTEE */

void printk(const char *fmt, ...)
{
        va_list va;

#ifdef CHCORE_OPENTRUSTEE
        if (is_tlogger_on()) {
                char buf[BUF_SIZE];
                int count;
                char *ptr = buf;

                /*
                 * Caller(trusted) should check overflow before printk.
                 * Or (v)snprintf support in kernel.
                 */
                va_start(va, fmt);
                count = simple_vsprintf(&ptr, fmt, va);
                va_end(va);
                append_chcore_log(buf, count, true);
                return;
        }
#endif /* CHCORE_OPENTRUSTEE */

        va_start(va, fmt);
        simple_vsprintf(NULL, fmt, va);
        va_end(va);
}
