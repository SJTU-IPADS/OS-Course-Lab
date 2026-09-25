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

#ifndef LOADER_H
#define LOADER_H

#include "liblaunch.h"

/**
 * This struct represents an abstract program loader.
 * A program loader can be used to launch a dynamically linked
 * program. When doing so, the loader itself would be launched
 * as a process first, then it would load and dynamically link
 * the really program to be run in its address space.
 */
struct loader;

/**
 * @brief Get a loader object by its path. Thread-Safe.
 *
 * By default, this function would cache loader objects used before.
 * And returning the same loader object if it used to be invoked.
 * As a loader file would be specified by many dynamically linked programs, it's
 * would be useful to reuse memory consumed by this loader(including the loader
 * object, file content loaded into memory for this loader, and maybe other
 * memory depending on loader object implementation), instead of creating
 * multiple loader objects for the same loader file.
 *
 * @param loader_path [In]
 * @param loader [Out] WARNING: the returning pointer is owned by this function,
 * and caller just borrow it. Caller is not allowed to free this pointer,
 * otherwise the behavior is undefined.
 * @return 0 if success, otherwise -errno is returned. All memory resources
 * consumed by this function are guaranteed to be freed if not success.
 */
int find_loader(const char *loader_path, struct loader **loader);

/**
 * @brief Launch a dynamically linked program using given loader. Thread-Safe.
 *
 * @param this [In] loader object to be used
 * @param lp_args [In] WARNING: This function just borrow the pointer. Caller
 * owns this pointer, So it's the caller's responsibility to free it if it's a
 * heap pointer.
 * @return 0 if success, otherwise -errno is returned. All memory resources
 * consumed by this function are guaranteed to be freed if not success.
 */
int launch_process_using_loader(struct loader *this,
                                struct launch_process_args *lp_args);

#endif /* LOADER_H */