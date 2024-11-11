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

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "header.h"

enum http_method {
        HTTP_GET,
        HTTP_POST,
        // TODO
};

struct request {
        enum http_method method;
        char *path;
        struct header *headers;
};

static inline struct request *request_parse(const char *raw)
{
        if (!raw || !*raw)
                return NULL;

        struct request *req = (struct request *)malloc(sizeof(struct request));
        const char *sth_end;

        // parse method
        sth_end = strchr(raw, ' ');
        if (!sth_end)
                goto bad_request;
        if (0 == strncmp("GET", raw, sth_end - raw)) {
                req->method = HTTP_GET;
        } else {
                // TODO: other method
                goto bad_request;
        }
        for (raw = sth_end; *raw == ' '; raw++)
                ;

        // parse path
        sth_end = strchr(raw, ' ');
        if (!sth_end)
                goto bad_request;
        req->path = strndup(raw, sth_end - raw);
        for (raw = sth_end; *raw == ' '; raw++)
                ;

        // TODO: ignore http version for now

        // find new line
        for (; *raw && *raw != '\r' && *raw != '\n'; raw++)
                ;
        // find headers
        for (; *raw == '\r' || *raw == '\n'; raw++)
                ;
        // parse headers
        req->headers = header_parse(raw);
        if (!req->headers)
                goto bad_request;

        return req;
bad_request:
        free(req);
        return NULL;
}

static inline void request_destroy(struct request *request)
{
        unused(request);
}
