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

#include "loader.h"
#include "chcore/defs.h"
#include "liblaunch.h"
#include "libchcoreelf.h"
#include <chcore/container/list.h>
#include <chcore/memory.h>
#include <chcore/type.h>
#include <chcore/aslr.h>
#include <pthread.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define LOADER_PATH_MAX_LENGTH (255)

/**
 * @brief Defining loader interface or base loader object. So this
 * struct represents an abstract loader. And concrete loader objects
 * should embed it as the first member. This layout constraint is used
 * by following simple OOP system to implement polymorphism.
 */
struct loader {
        struct list_head node;
        char path[LOADER_PATH_MAX_LENGTH + 1];
        /**
         * @brief Method to init private state of a concrete
         * loader object, is invoked when initializing a new concrete object.
         *
         * @return 0 if success, otherwise -errno. All memory resources used
         * by this function should be freed on error.
         */
        int (*init_loader)(struct loader *this);
        /**
         * @brief Method to launch the dynamically linked program specified in
         * lp_args.
         *
         * Implementation of this method is must Thread-Safe.
         *
         * @return 0 if success, otherwise -errno. All memory resources used
         * by this function should be freed on error.
         */
        int (*launch_process)(struct loader *this,
                              struct launch_process_args *lp_args);
        /**
         * @brief Method to free a concrete loader object. This
         * function should free not only the object itself, but also all
         * memory resources used by its private state.
         */
        void (*destructor)(struct loader *this);
};

/**
 * This list protected by a mutex is functioned as a thread-safe cache for all
 * concrete loader objects being used previously.
 */
struct list_head loaders_list;
pthread_mutex_t loaders_mu;
pthread_once_t loaders_ctrl = PTHREAD_ONCE_INIT;

static void __init_loaders_list(void)
{
        init_list_head(&loaders_list);
        pthread_mutex_init(&loaders_mu, NULL);
}

static void init_loaders_list(void)
{
        pthread_once(&loaders_ctrl, __init_loaders_list);
}

static struct loader *__find_loader_in_list(const char *path)
{
        struct loader *iter;
        for_each_in_list (iter, struct loader, node, &loaders_list) {
                if (strncmp(path, iter->path, LOADER_PATH_MAX_LENGTH) == 0) {
                        return iter;
                }
        }
        return NULL;
}

/**
 * @brief Defining constructors, or "classes" of concrete loader
 * implementations. A constructor function should return a pointer to a concrete
 * loader object, it don't have to implement singleton pattern. It should
 * allocate memory for the new object, performing interface binding, but
 * **don't** invoke init_loader method.
 */
typedef struct loader *(*constructor_t)(const char *path);

/**
 * @brief Each different path corrsponds to a constructor function. And the same
 * constructor can be used by multiple entries.
 */
struct constructor_table_entry {
        char path[LOADER_PATH_MAX_LENGTH + 1];
        constructor_t constructor;
};

static struct loader *elf_so_loader_constructor(const char *path);

struct constructor_table_entry constructor_table[] = {
        {CHCORE_LOADER, elf_so_loader_constructor}};

#define CONSTRUCTOR_CNT \
        (sizeof(constructor_table) / sizeof(struct constructor_table_entry))

constructor_t __find_loader_constructor(const char *path)
{
        for (int i = 0; i < CONSTRUCTOR_CNT; i++) {
                if (strncmp(path,
                            constructor_table[i].path,
                            LOADER_PATH_MAX_LENGTH)
                    == 0) {
                        return constructor_table[i].constructor;
                }
        }

        return NULL;
}

/**
 * @brief Factory function to create a concrete loader object. This function
 * implements singleton pattern, so it will return the same object if the same
 * path is passed in.
 *
 * Specifically, this function will lookup the path in the cache above to see if
 * there is a loader object for the path. If not, it will create a new object by
 * querying the constructor table and invoking the corresponding constructor
 * function, and add it to the cache.
 *
 * @param loader_path [In]
 * @param loader [Out] WARNING: the returning pointer is owned by this function,
 * and caller just borrow it. Caller is not allowed to free this pointer,
 * otherwise the behavior is undefined.
 * @return 0 if success, otherwise -errno is returned. All memory resources
 * consumed by this function are guaranteed to be freed if not success.
 */
static int loader_factory(const char *loader_path, struct loader **loader)
{
        int ret = 0;
        struct loader *target;
        constructor_t constructor;
        pthread_mutex_lock(&loaders_mu);
        target = __find_loader_in_list(loader_path);
        if (target) {
                *loader = target;
                goto out;
        }

        constructor = __find_loader_constructor(loader_path);
        if (!constructor) {
                ret = -EINVAL;
                goto out;
        }

        target = constructor(loader_path);
        if (!target) {
                ret = -ENOMEM;
                goto out;
        }

        ret = target->init_loader(target);
        if (ret < 0) {
                goto out_init_fail;
        }

        list_append(&target->node, &loaders_list);
        *loader = target;
out:
        pthread_mutex_unlock(&loaders_mu);
        return ret;
out_init_fail:
        pthread_mutex_unlock(&loaders_mu);
        target->destructor(target);
        return ret;
}

int find_loader(const char *loader_path, struct loader **loader)
{
        init_loaders_list();

        if (!loader_path) {
                return -EINVAL;
        }

        return loader_factory(loader_path, loader);
}

int launch_process_using_loader(struct loader *this,
                                struct launch_process_args *lp_args)
{
        if (!this) {
                return -EINVAL;
        }

        return this->launch_process(this, lp_args);
}

/* Load offset of libc.so */
#if __SIZEOF_POINTER__ == 4
#define LIBC_LDSO_BASE (0x40000000UL)
#else
#define LIBC_LDSO_BASE (0x400000000000UL)
#endif

/**
 * Concrete loader implementation for loaders who are a DYN ELF file itself.
 */
struct elf_so_loader {
        /** Note: this member must be the first member */
        struct loader base;
        struct user_elf *elf_so;
};

static int elf_so_loader_init(struct loader *_this)
{
        int ret, i;
        struct user_elf *loader_elf;
        struct user_elf_seg *cur_seg;
        struct elf_so_loader *this = (struct elf_so_loader *)_this;

        ret = load_elf_from_fs(this->base.path, &loader_elf);

        if (ret < 0) {
                return ret;
        }

        for (i = 0; i < loader_elf->segs_nr; i++) {
                /**
                 * Because the loader is a DYN ELF file itself, it should be
                 * shared by all processes created using this loader through CoW
                 * to save memory. So for each loadable writable segment, we
                 * remove VMR_WRITE permission and make it VMR_COW.
                 */
                cur_seg = &loader_elf->user_elf_segs[i];
                if (cur_seg->perm & VMR_WRITE) {
                        cur_seg->perm &= (~VMR_WRITE);
                        cur_seg->perm |= VMR_COW;
                }
        }
        this->elf_so = loader_elf;
        return 0;
}

static void elf_so_loader_deinit(struct loader *_this)
{
        struct elf_so_loader *this = (struct elf_so_loader *)_this;

        if (this->elf_so) {
                free_user_elf(this->elf_so);
        }

        free(this);
}

static int elf_so_loader_launch_process(struct loader *_this,
                                        struct launch_process_args *lp_args)
{
        int ret = 0, i, argc;
        char **prepended_argv, **argv;
        struct elf_so_loader *this = (struct elf_so_loader *)_this;

        /**
         * The ELF file content should be loaded by the loader program, instead
         * of the caller.
         */
        if (lp_args->user_elf) {
                ret = -EINVAL;
                goto out;
        }

        argc = lp_args->argc;
        argv = lp_args->argv;

        /**
         * The loader itself would be launched as the new process at first, and
         * it would find the real program to be launched from command line
         * arguments. So its command line arguments should be prepended with
         * itself, then the original command line arguments.
         *
         * E.g., /hello_dl.bin -> /libc.so /hello_dl.bin
         */
        prepended_argv = malloc(sizeof(char *) * (argc + 1));
        if (!prepended_argv) {
                ret = -ENOMEM;
                goto out;
        }

        prepended_argv[0] = this->base.path;
        for (i = 0; i < argc; i++) {
                prepended_argv[1 + i] = argv[i];
        }

        /**
         * Modify lp_args like a proxy.
         */
        lp_args->user_elf = this->elf_so;
        lp_args->argc = argc + 1;
        lp_args->argv = prepended_argv;
        /**
         * Loader itself would use a high range of virtual address space of the
         * new process. Avoid conflicts with the real memory image of new
         * process.
         */
        lp_args->load_offset =
                LIBC_LDSO_BASE
                + ASLR_RAND_OFFSET;

        ret = launch_process_with_pmos_caps(lp_args);

        /**
         * Backup and resume original lp_args content, because we just borrow
         * the pointer.
         */
        lp_args->user_elf = NULL;
        lp_args->argv = argv;

        free(prepended_argv);
out:
        return ret;
}

static struct loader *elf_so_loader_constructor(const char *path)
{
        struct elf_so_loader *this = malloc(sizeof(*this));
        if (!this) {
                return NULL;
        }
        this->elf_so = NULL;

        strncpy(this->base.path, path, LOADER_PATH_MAX_LENGTH);
        this->base.init_loader = elf_so_loader_init;
        this->base.launch_process = elf_so_loader_launch_process;
        this->base.destructor = elf_so_loader_deinit;
        return (struct loader *)this;
}