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

#ifndef UAPI_PTRACE_H
#define UAPI_PTRACE_H

#define CHCORE_PTRACE_ATTACH		0
#define CHCORE_PTRACE_GETTHREADS	1
#define CHCORE_PTRACE_SUSPEND		2
#define CHCORE_PTRACE_CONT		3
#define CHCORE_PTRACE_GETREGS		4
#define CHCORE_PTRACE_PEEKDATA		5
#define CHCORE_PTRACE_POKEDATA		6
#define CHCORE_PTRACE_PEEKUSER		7
#define CHCORE_PTRACE_POKEUSER		8
#define CHCORE_PTRACE_WAITBKPT		9
#define CHCORE_PTRACE_SINGLESTEP	10
#define CHCORE_PTRACE_GETBKPTINFO	11
#define CHCORE_PTRACE_DETACH		12

#endif
