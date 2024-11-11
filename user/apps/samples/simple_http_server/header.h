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

#pragma once

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

struct header {
        char *key;
        char *value;
        struct header *next;
};

static inline int _strcmp_icase(const char *s1, const char *s2)
{
        for (; tolower(*s1) == tolower(*s2) && *s1; s1++, s2++)
                ;
        return tolower(*s1) - tolower(*s2);
}

static inline const char *header_get(struct header *headers, const char *key)
{
        struct header *h = headers;
        while (h) {
                if (0 == _strcmp_icase(h->key, key))
                        return h->value;
                h = h->next;
        }
        return NULL;
}

static inline struct header *header_parse(const char *header_str)
{
        /*
         * header_str:
         *
         * Host: xxxxx
         * Cookie: xxxxx
         */
        if (!header_str || !*header_str)
                return NULL;

        struct header *first = NULL, *last = NULL, *curr = NULL;
        char *key, *value;
        const char *colon, *val_end;
        while ((colon = strchr(header_str, ':'))) {
                // get header key
                key = strndup(header_str, colon - header_str);
                // skip spaces
                for (header_str = colon + 1; *header_str == ' '; header_str++)
                        ;
                // find end of value
                for (val_end = header_str;
                     *val_end && *val_end != '\r' && *val_end != '\n';
                     val_end++)
                        ;
                // get header value
                value = strndup(header_str, val_end - header_str);
                // skip new-line charaters
                for (header_str = val_end;
                     *header_str == '\r' || *header_str == '\n';
                     header_str++)
                        ;

                curr = (struct header *)malloc(sizeof(struct header));
                memset(curr, 0, sizeof(struct header));
                curr->key = key;
                curr->value = value;
                if (!last) {
                        first = curr;
                } else {
                        last->next = curr;
                }
                last = curr;
        }
        return first;
}

static inline void header_destroy(struct header *headers)
{
        unused(headers);
}
