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

#include <stdlib.h>

#include "ffi.h"

#ifdef CHCORE_ARCH_AARCH64
void __ffi_call0(void* target, ffi_type_t ret_type, void* ret_val)
{
        asm volatile("stp x29, x30, [sp, #-16]!\n\t"
                     "mov x29, sp");
        u64 ret;
        register void* callee_saved_ret_ptr asm("r19");
        register ffi_type_t callee_saved_ret_type asm("r20");
        asm volatile("mov %[saved_ptr], %[ret_ptr]\n\t"
                     "mov %[saved_type], %[ret_type]\n\t"
                     : [saved_ptr] "+r"(callee_saved_ret_ptr),
                       [saved_type] "+r"(callee_saved_ret_type)
                     : [ret_ptr] "r"(ret_val), [ret_type] "r"(ret_type));
        asm volatile("blr %[target]\n\t"
                     "mov %[ret], x0"
                     : [ret] "=r"(ret)
                     : [target] "r"(target)
                     : "cc", "memory");

        if (callee_saved_ret_ptr) {
                switch (callee_saved_ret_type) {
                case FFI_S32:
                        asm volatile("str %w[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;
                case FFI_U32:
                        asm volatile("str %w[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;
                case FFI_S64:
                        asm volatile("str %x[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;
                case FFI_U64:
                        asm volatile("str %x[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;
                case FFI_PTR:
                        asm volatile("str %x[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;

                default:
                        break;
                }
        }
        asm volatile("ldp x29, x30, [sp], #16\n\t");
}
void __ffi_call1(void* target, ffi_type_t ret_type, void* ret_val, u64 a0)
{
        asm volatile("stp x29, x30, [sp, #-16]!\n\t"
                     "mov x29, sp");
        u64 ret;
        register void* callee_saved_ret_ptr asm("r19");
        register ffi_type_t callee_saved_ret_type asm("r20");
        asm volatile("mov %[saved_ptr], %[ret_ptr]\n\t"
                     "mov %[saved_type], %[ret_type]\n\t"
                     : [saved_ptr] "+r"(callee_saved_ret_ptr),
                       [saved_type] "+r"(callee_saved_ret_type)
                     : [ret_ptr] "r"(ret_val), [ret_type] "r"(ret_type));
        asm volatile("mov x0, %[arg0]\n\t"
                     "blr %[target]\n\t"
                     "mov %[ret], x0"
                     : [ret] "=r"(ret)
                     : [arg0] "r"(a0), [target] "r"(target)
                     : "cc", "memory", "r0");

        if (callee_saved_ret_ptr) {
                switch (callee_saved_ret_type) {
                case FFI_S32:
                        asm volatile("str %w[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;
                case FFI_U32:
                        asm volatile("str %w[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;
                case FFI_S64:
                        asm volatile("str %x[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;
                case FFI_U64:
                        asm volatile("str %x[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;
                case FFI_PTR:
                        asm volatile("str %x[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;

                default:
                        break;
                }
        }
        asm volatile("ldp x29, x30, [sp], #16\n\t");
}
void __ffi_call2(void* target, ffi_type_t ret_type, void* ret_val, u64 a0,
                 u64 a1)
{
        asm volatile("stp x29, x30, [sp, #-16]!\n\t"
                     "mov x29, sp");
        u64 ret;
        register void* callee_saved_ret_ptr asm("r19");
        register ffi_type_t callee_saved_ret_type asm("r20");
        asm volatile("mov %[saved_ptr], %[ret_ptr]\n\t"
                     "mov %[saved_type], %[ret_type]\n\t"
                     : [saved_ptr] "+r"(callee_saved_ret_ptr),
                       [saved_type] "+r"(callee_saved_ret_type)
                     : [ret_ptr] "r"(ret_val), [ret_type] "r"(ret_type));
        asm volatile("mov x0, %[arg0]\n\t"
                     "mov x1, %[arg1]\n\t"
                     "blr %[target]\n\t"
                     "mov %[ret], x0"
                     : [ret] "=r"(ret)
                     : [arg0] "r"(a0), [arg1] "r"(a1), [target] "r"(target)
                     : "cc", "memory", "r0", "r1");

        if (callee_saved_ret_ptr) {
                switch (callee_saved_ret_type) {
                case FFI_S32:
                        asm volatile("str %w[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;
                case FFI_U32:
                        asm volatile("str %w[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;
                case FFI_S64:
                        asm volatile("str %x[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;
                case FFI_U64:
                        asm volatile("str %x[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;
                case FFI_PTR:
                        asm volatile("str %x[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;

                default:
                        break;
                }
        }
        asm volatile("ldp x29, x30, [sp], #16\n\t");
}
void __ffi_call3(void* target, ffi_type_t ret_type, void* ret_val, u64 a0,
                 u64 a1, u64 a2)
{
        asm volatile("stp x29, x30, [sp, #-16]!\n\t"
                     "mov x29, sp");
        u64 ret;
        register void* callee_saved_ret_ptr asm("r19");
        register ffi_type_t callee_saved_ret_type asm("r20");
        asm volatile("mov %[saved_ptr], %[ret_ptr]\n\t"
                     "mov %[saved_type], %[ret_type]\n\t"
                     : [saved_ptr] "+r"(callee_saved_ret_ptr),
                       [saved_type] "+r"(callee_saved_ret_type)
                     : [ret_ptr] "r"(ret_val), [ret_type] "r"(ret_type));
        asm volatile("mov x0, %[arg0]\n\t"
                     "mov x1, %[arg1]\n\t"
                     "mov x2, %[arg2]\n\t"
                     "blr %[target]\n\t"
                     "mov %[ret], x0"
                     : [ret] "=r"(ret)
                     : [arg0] "r"(a0),
                       [arg1] "r"(a1),
                       [arg2] "r"(a2),
                       [target] "r"(target)
                     : "cc", "memory", "r0", "r1", "r2");

        if (callee_saved_ret_ptr) {
                switch (callee_saved_ret_type) {
                case FFI_S32:
                        asm volatile("str %w[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;
                case FFI_U32:
                        asm volatile("str %w[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;
                case FFI_S64:
                        asm volatile("str %x[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;
                case FFI_U64:
                        asm volatile("str %x[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;
                case FFI_PTR:
                        asm volatile("str %x[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;

                default:
                        break;
                }
        }
        asm volatile("ldp x29, x30, [sp], #16\n\t");
}
void __ffi_call4(void* target, ffi_type_t ret_type, void* ret_val, u64 a0,
                 u64 a1, u64 a2, u64 a3)
{
        asm volatile("stp x29, x30, [sp, #-16]!\n\t"
                     "mov x29, sp");
        u64 ret;
        register void* callee_saved_ret_ptr asm("r19");
        register ffi_type_t callee_saved_ret_type asm("r20");
        asm volatile("mov %[saved_ptr], %[ret_ptr]\n\t"
                     "mov %[saved_type], %[ret_type]\n\t"
                     : [saved_ptr] "+r"(callee_saved_ret_ptr),
                       [saved_type] "+r"(callee_saved_ret_type)
                     : [ret_ptr] "r"(ret_val), [ret_type] "r"(ret_type));
        asm volatile("mov x0, %[arg0]\n\t"
                     "mov x1, %[arg1]\n\t"
                     "mov x2, %[arg2]\n\t"
                     "mov x3, %[arg3]\n\t"
                     "blr %[target]\n\t"
                     "mov %[ret], x0"
                     : [ret] "=r"(ret)
                     : [arg0] "r"(a0),
                       [arg1] "r"(a1),
                       [arg2] "r"(a2),
                       [arg3] "r"(a3),
                       [target] "r"(target)
                     : "cc", "memory", "r0", "r1", "r2", "r3");

        if (callee_saved_ret_ptr) {
                switch (callee_saved_ret_type) {
                case FFI_S32:
                        asm volatile("str %w[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;
                case FFI_U32:
                        asm volatile("str %w[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;
                case FFI_S64:
                        asm volatile("str %x[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;
                case FFI_U64:
                        asm volatile("str %x[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;
                case FFI_PTR:
                        asm volatile("str %x[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;

                default:
                        break;
                }
        }
        asm volatile("ldp x29, x30, [sp], #16\n\t");
}
void __ffi_call5(void* target, ffi_type_t ret_type, void* ret_val, u64 a0,
                 u64 a1, u64 a2, u64 a3, u64 a4)
{
        asm volatile("stp x29, x30, [sp, #-16]!\n\t"
                     "mov x29, sp");
        u64 ret;
        register void* callee_saved_ret_ptr asm("r19");
        register ffi_type_t callee_saved_ret_type asm("r20");
        asm volatile("mov %[saved_ptr], %[ret_ptr]\n\t"
                     "mov %[saved_type], %[ret_type]\n\t"
                     : [saved_ptr] "+r"(callee_saved_ret_ptr),
                       [saved_type] "+r"(callee_saved_ret_type)
                     : [ret_ptr] "r"(ret_val), [ret_type] "r"(ret_type));
        asm volatile("mov x0, %[arg0]\n\t"
                     "mov x1, %[arg1]\n\t"
                     "mov x2, %[arg2]\n\t"
                     "mov x3, %[arg3]\n\t"
                     "mov x4, %[arg4]\n\t"
                     "blr %[target]\n\t"
                     "mov %[ret], x0"
                     : [ret] "=r"(ret)
                     : [arg0] "r"(a0),
                       [arg1] "r"(a1),
                       [arg2] "r"(a2),
                       [arg3] "r"(a3),
                       [arg4] "r"(a4),
                       [target] "r"(target)
                     : "cc", "memory", "r0", "r1", "r2", "r3", "r4");

        if (callee_saved_ret_ptr) {
                switch (callee_saved_ret_type) {
                case FFI_S32:
                        asm volatile("str %w[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;
                case FFI_U32:
                        asm volatile("str %w[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;
                case FFI_S64:
                        asm volatile("str %x[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;
                case FFI_U64:
                        asm volatile("str %x[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;
                case FFI_PTR:
                        asm volatile("str %x[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;

                default:
                        break;
                }
        }
        asm volatile("ldp x29, x30, [sp], #16\n\t");
}
void __ffi_call6(void* target, ffi_type_t ret_type, void* ret_val, u64 a0,
                 u64 a1, u64 a2, u64 a3, u64 a4, u64 a5)
{
        asm volatile("stp x29, x30, [sp, #-16]!\n\t"
                     "mov x29, sp");
        u64 ret;
        register void* callee_saved_ret_ptr asm("r19");
        register ffi_type_t callee_saved_ret_type asm("r20");
        asm volatile("mov %[saved_ptr], %[ret_ptr]\n\t"
                     "mov %[saved_type], %[ret_type]\n\t"
                     : [saved_ptr] "+r"(callee_saved_ret_ptr),
                       [saved_type] "+r"(callee_saved_ret_type)
                     : [ret_ptr] "r"(ret_val), [ret_type] "r"(ret_type));
        asm volatile("mov x0, %[arg0]\n\t"
                     "mov x1, %[arg1]\n\t"
                     "mov x2, %[arg2]\n\t"
                     "mov x3, %[arg3]\n\t"
                     "mov x4, %[arg4]\n\t"
                     "mov x5, %[arg5]\n\t"
                     "blr %[target]\n\t"
                     "mov %[ret], x0"
                     : [ret] "=r"(ret)
                     : [arg0] "r"(a0),
                       [arg1] "r"(a1),
                       [arg2] "r"(a2),
                       [arg3] "r"(a3),
                       [arg4] "r"(a4),
                       [arg5] "r"(a5),
                       [target] "r"(target)
                     : "cc", "memory", "r0", "r1", "r2", "r3", "r4", "r5");

        if (callee_saved_ret_ptr) {
                switch (callee_saved_ret_type) {
                case FFI_S32:
                        asm volatile("str %w[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;
                case FFI_U32:
                        asm volatile("str %w[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;
                case FFI_S64:
                        asm volatile("str %x[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;
                case FFI_U64:
                        asm volatile("str %x[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;
                case FFI_PTR:
                        asm volatile("str %x[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;

                default:
                        break;
                }
        }
        asm volatile("ldp x29, x30, [sp], #16\n\t");
}
void __ffi_call7(void* target, ffi_type_t ret_type, void* ret_val, u64 a0,
                 u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6)
{
        asm volatile("stp x29, x30, [sp, #-16]!\n\t"
                     "mov x29, sp");
        u64 ret;
        register void* callee_saved_ret_ptr asm("r19");
        register ffi_type_t callee_saved_ret_type asm("r20");
        asm volatile("mov %[saved_ptr], %[ret_ptr]\n\t"
                     "mov %[saved_type], %[ret_type]\n\t"
                     : [saved_ptr] "+r"(callee_saved_ret_ptr),
                       [saved_type] "+r"(callee_saved_ret_type)
                     : [ret_ptr] "r"(ret_val), [ret_type] "r"(ret_type));
        asm volatile(
                "mov x0, %[arg0]\n\t"
                "mov x1, %[arg1]\n\t"
                "mov x2, %[arg2]\n\t"
                "mov x3, %[arg3]\n\t"
                "mov x4, %[arg4]\n\t"
                "mov x5, %[arg5]\n\t"
                "mov x6, %[arg6]\n\t"
                "blr %[target]\n\t"
                "mov %[ret], x0"
                : [ret] "=r"(ret)
                : [arg0] "r"(a0),
                  [arg1] "r"(a1),
                  [arg2] "r"(a2),
                  [arg3] "r"(a3),
                  [arg4] "r"(a4),
                  [arg5] "r"(a5),
                  [arg6] "r"(a6),
                  [target] "r"(target)
                : "cc", "memory", "r0", "r1", "r2", "r3", "r4", "r5", "r6");

        if (callee_saved_ret_ptr) {
                switch (callee_saved_ret_type) {
                case FFI_S32:
                        asm volatile("str %w[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;
                case FFI_U32:
                        asm volatile("str %w[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;
                case FFI_S64:
                        asm volatile("str %x[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;
                case FFI_U64:
                        asm volatile("str %x[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;
                case FFI_PTR:
                        asm volatile("str %x[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;

                default:
                        break;
                }
        }
        asm volatile("ldp x29, x30, [sp], #16\n\t");
}
void __ffi_call8(void* target, ffi_type_t ret_type, void* ret_val, u64 a0,
                 u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6, u64 a7)
{
        asm volatile("stp x29, x30, [sp, #-16]!\n\t"
                     "mov x29, sp");
        u64 ret;
        register void* callee_saved_ret_ptr asm("r19");
        register ffi_type_t callee_saved_ret_type asm("r20");
        asm volatile("mov %[saved_ptr], %[ret_ptr]\n\t"
                     "mov %[saved_type], %[ret_type]\n\t"
                     : [saved_ptr] "+r"(callee_saved_ret_ptr),
                       [saved_type] "+r"(callee_saved_ret_type)
                     : [ret_ptr] "r"(ret_val), [ret_type] "r"(ret_type));
        asm volatile("mov x0, %[arg0]\n\t"
                     "mov x1, %[arg1]\n\t"
                     "mov x2, %[arg2]\n\t"
                     "mov x3, %[arg3]\n\t"
                     "mov x4, %[arg4]\n\t"
                     "mov x5, %[arg5]\n\t"
                     "mov x6, %[arg6]\n\t"
                     "mov x7, %[arg7]\n\t"
                     "blr %[target]\n\t"
                     "mov %[ret], x0"
                     : [ret] "=r"(ret)
                     : [arg0] "r"(a0),
                       [arg1] "r"(a1),
                       [arg2] "r"(a2),
                       [arg3] "r"(a3),
                       [arg4] "r"(a4),
                       [arg5] "r"(a5),
                       [arg6] "r"(a6),
                       [arg7] "r"(a7),
                       [target] "r"(target)
                     : "cc",
                       "memory",
                       "r0",
                       "r1",
                       "r2",
                       "r3",
                       "r4",
                       "r5",
                       "r6",
                       "r7");

        if (callee_saved_ret_ptr) {
                switch (callee_saved_ret_type) {
                case FFI_S32:
                        asm volatile("str %w[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;
                case FFI_U32:
                        asm volatile("str %w[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;
                case FFI_S64:
                        asm volatile("str %x[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;
                case FFI_U64:
                        asm volatile("str %x[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;
                case FFI_PTR:
                        asm volatile("str %x[callee_ret], [%[saved_ret_ptr]]"
                                     :
                                     : [callee_ret] "r"(ret),
                                       [saved_ret_ptr] "r"(callee_saved_ret_ptr)
                                     : "memory");
                        break;

                default:
                        break;
                }
        }
        asm volatile("ldp x29, x30, [sp], #16\n\t");
}

void ffi_call(void* target, struct ffi* call_env, void* ret_value)
{
        u64 arg0 = 0;
        u64 arg1 = 0;
        u64 arg2 = 0;
        u64 arg3 = 0;
        u64 arg4 = 0;
        u64 arg5 = 0;
        u64 arg6 = 0;
        u64 arg7 = 0;
        int cur_arg_id = 0;
        struct ffi_arg* cur_arg;
        for_each_in_list (cur_arg, struct ffi_arg, node, &call_env->args) {
                switch (cur_arg_id) {
                case 0:
                        arg0 = cur_arg->arg.u64;
                        break;
                case 1:
                        arg1 = cur_arg->arg.u64;
                        break;
                case 2:
                        arg2 = cur_arg->arg.u64;
                        break;
                case 3:
                        arg3 = cur_arg->arg.u64;
                        break;
                case 4:
                        arg4 = cur_arg->arg.u64;
                        break;
                case 5:
                        arg5 = cur_arg->arg.u64;
                        break;
                case 6:
                        arg6 = cur_arg->arg.u64;
                        break;
                case 7:
                        arg7 = cur_arg->arg.u64;
                        break;
                default:
                        break;
                }
                cur_arg_id++;
        }
        int argc = cur_arg_id;
        switch (argc) {
        case 0:
                __ffi_call0(target, call_env->ret_type, ret_value);
                break;
        case 1:
                __ffi_call1(target, call_env->ret_type, ret_value, arg0);
                break;
        case 2:
                __ffi_call2(target, call_env->ret_type, ret_value, arg0, arg1);
                break;
        case 3:
                __ffi_call3(
                        target, call_env->ret_type, ret_value, arg0, arg1, arg2);
                break;
        case 4:
                __ffi_call4(target,
                            call_env->ret_type,
                            ret_value,
                            arg0,
                            arg1,
                            arg2,
                            arg3);
                break;
        case 5:
                __ffi_call5(target,
                            call_env->ret_type,
                            ret_value,
                            arg0,
                            arg1,
                            arg2,
                            arg3,
                            arg4);
                break;
        case 6:
                __ffi_call6(target,
                            call_env->ret_type,
                            ret_value,
                            arg0,
                            arg1,
                            arg2,
                            arg3,
                            arg4,
                            arg5);
                break;
        case 7:
                __ffi_call7(target,
                            call_env->ret_type,
                            ret_value,
                            arg0,
                            arg1,
                            arg2,
                            arg3,
                            arg4,
                            arg5,
                            arg6);
                break;
        case 8:
                __ffi_call8(target,
                            call_env->ret_type,
                            ret_value,
                            arg0,
                            arg1,
                            arg2,
                            arg3,
                            arg4,
                            arg5,
                            arg6,
                            arg7);
                break;
        default:
                break;
        }
}
#else /* CHCORE_PLAT_RASPI3 */
void ffi_call(void* target, struct ffi* call_env, void* ret_value)
{
        /* Not supported for other platforms now */
        BUG("platform NOT supported!");
}
#endif /* CHCORE_PLAT_RASPI3 */
