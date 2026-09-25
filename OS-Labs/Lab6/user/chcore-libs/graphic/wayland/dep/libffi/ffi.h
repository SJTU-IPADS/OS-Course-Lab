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

#include <chcore/container/list.h>
#include <stdlib.h>

typedef union {
        int s32;
        unsigned int u32;
        long long s64;
        unsigned long long u64;
        unsigned long long ptr;
} ffi_arg_t;

typedef enum { FFI_S32, FFI_U32, FFI_S64, FFI_U64, FFI_PTR } ffi_type_t;

struct ffi_arg {
        ffi_arg_t arg;
        ffi_type_t type;
        struct list_head node;
};

struct ffi {
        struct list_head args;
        ffi_type_t ret_type;
};

static inline void ffi_add_arg(u64 arg, ffi_type_t type, struct ffi* ffi)
{
        // printf("ffi: add %llx\n", arg);
        struct ffi_arg* new_arg =
                (struct ffi_arg*)malloc(sizeof(struct ffi_arg));
        ffi_arg_t argu;
        switch (type) {
        case FFI_S32:
                argu.s32 = (s32)arg;
                break;
        case FFI_U32:
                argu.u32 = (u32)arg;
                break;
        case FFI_S64:
                argu.s64 = (s64)arg;
                break;
        case FFI_U64:
                argu.u64 = (u64)arg;
                break;
        case FFI_PTR:
                argu.ptr = (u64)arg;
                break;

        default:
                break;
        }
        new_arg->arg = argu;
        new_arg->type = type;
        list_append(&new_arg->node, &ffi->args);
}

static inline struct ffi* init_ffi(ffi_type_t ret_type)
{
        struct ffi* new_ffi = (struct ffi*)malloc(sizeof(struct ffi));
        new_ffi->ret_type = ret_type;
        init_list_head(&new_ffi->args);
        return new_ffi;
}

void ffi_call(void* target, struct ffi* call_env, void* ret_value);
