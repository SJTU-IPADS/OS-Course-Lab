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
 * This file contains utils of pathname resolution for tmpfs.
 *
 * a path: "/this/is/a/path"
 * a component: "this", "is", etc.
 *
 * In general, a path can be divided into two parts, and we deal with each
 * part separately. One part is the "final component" part, "path" in the above
 * example. The other part is the "everything else" part, "/this/is/a" in the
 * above example.
 * The reason for such separation is that the final component
 * usually requires some special care, for example in a call to open() syscall,
 * whether or not to create the file if it's missing.
 * The every thing else part is much easier, since we always follow
 * intermediate symlink when we can, and the everything else part should
 * only resolve to existing directories.
 *
 * The way tmpfs' pathname resolution works is as follows:
 * 0. walk_component() finds a dentry for one component each time, and update
 *    nd->current by calling step_into() so we get one more level
 *    deeper in the directory tree. It also deals with symlinks and
 *    some other special cases.
 *
 * 1. We call walk_prefix() to do a general purpose pathname lookup on the
 *    everything else part. The function will update nd->last in each of its
 *    loop to process one component in the pathname, then call
 *    walk_component() to actually find the dentry of that component name,
 *    in which nd->current will be updated. walk_prefix() only go as far as
 *    setting nd->last to the final component and nd->current to the
 *    parent directory entry of that final component.
 *
 * 2. Based on the requirements of different FS system calls, we may take
 *    different approaches to deal with the final component, for example create
 *    it when it's missing. So we wrap this functionality into three wrapper
 *    functions path_parentat(), path_lookupat(), path_openat(). The interfaces
 *    of tmpfs will call these wrappers to do a full pathname lookup and deal
 *    with corner cases.
 */

#include "namei.h"
#include "chcore/error.h"
#include "defs.h"
#include "limits.h"
#include "stddef.h"
#include <errno.h>
#include "fcntl.h"
#include <string.h>

/**
 * @brief length until next '/' or '\0'
 * @param name pointer to a null terminated string
 * @return u64 The length of the first component pointed to by name
 */
static inline u64 get_component_len(const char *name)
{
        int i = 0;
        while (name[i] != '\0' && name[i] != '/') {
                i++;
        }
        return i;
}

static void init_nd(struct nameidata *nd, unsigned flags)
{
        nd->depth = 0;
        nd->flags = flags;
        /*
         * The current implementation of fs_base ensures tmpfs only get absolute
         * path when dealing with file system requests
         */
        nd->current = tmpfs_root_dent;
        nd->total_link_count = 0;
        nd->last.str = NULL;
}

/**
 * @brief Decide whether we can follow the symlink.
 * @param nd The nameidata structure to check.
 * @return int 0 if no error, or errno.
 */
static int can_follow_symlink(struct nameidata *nd)
{
        /* No stack space for the new symlink */
        if (nd->depth == MAX_STACK_SIZE) {
                return -ENOMEM;
        }

        /* Too many symlink encountered */
        if (nd->total_link_count++ == MAX_SYM_CNT) {
                return -ELOOP;
        }

        return 0;
}

/**
 * @brief update nd->current according to the found dentry
 * @param nd The nd structure of this path lookup.
 * @param trailing Indicate whether this is a trailing component, if it is, it
 * requires some special care.
 * @param dentry The dentry for nd->current to update to.
 * @return const char* Return NULL if not a symlink or should not follow, return
 * the symlink otherwise.
 */
static const char *step_into(struct nameidata *nd, bool trailing,
                             struct dentry *dentry)
{
        int err;

        /*
         * The dentry is not a symlink or should not be followed
         */
        if (dentry->inode->type != FS_SYM
            /* only the trailing symlink needs to check if it can be followed.
               always follow non-trailing symlinks */
            || (trailing && !(nd->flags & ND_FOLLOW))) {
                nd->current = dentry;
                return NULL;
        }

        /* dealing with a symlink now */

        err = can_follow_symlink(nd);
        if (err) {
                return CHCORE_ERR_PTR(err);
        }

        /* symlink is not allowed */
        if (nd->flags & ND_NO_SYMLINKS) {
                return CHCORE_ERR_PTR(-ELOOP);
        }

        /*
         * we directly return the actual symlink,
         * without copying it
         */
        const char *name = dentry->inode->symlink;

        if (!name) {
                return NULL;
        }

        if (*name == '/') {
                /* Absolute path symlink, jump to root */
                nd->current = tmpfs_root_dent;

                do {
                        name++;
                } while (*name == '/');
        }

        /* we do not update nd->current if the symlink is a relative path. */

        return *name ? name : NULL;
}

/**
 * @brief Lookup a single component named "nd->last" under the dentry
 * "nd->current". Find the dentry in walk_component() then call step_into() to
 * process symlinks and some flags.
 * @param nd The nd structure representing this path lookup.
 * @param trailing If this is the trailing component, some special care would be
 * taken when considering symlinks.
 * @return const char* Return NULL if no symlink encountered or should/can not
 * follow, return the symlink to follow it.
 * @note The result of this function during a pathname lookup will be an updated
 * nd->current, serving as the parent directory of next component lookup.
 */
static const char *walk_component(struct nameidata *nd, bool trailing)
{
        struct dentry *dentry;
        struct inode *i_parent;

        i_parent = nd->current->inode;
        if (i_parent->type != FS_DIR) {
                return CHCORE_ERR_PTR(-ENOTDIR);
        }

        /* Find the dentry of the nd->last component under nd->current */
        dentry = i_parent->d_ops->dirlookup(
                i_parent, nd->last.str, (int)nd->last.len);

        if (dentry == NULL) {
                return CHCORE_ERR_PTR(-ENOENT); /* File not exist */
        }

        return step_into(nd, trailing, dentry);
};

/**
 * @brief A simple wrapper of walk_component, used when looking up the last
 * component in the path.
 * @param nd The nd structure representing this path lookup.
 * @return const char* Return NULL if no symlink encountered or should/can not
 * follow, return the symlink to follow it.
 */
static inline const char *lookup_last(struct nameidata *nd)
{
        if (nd->last.str == NULL) {
                return NULL;
        }

        return walk_component(nd, true);
}

/**
 * @brief lookup the dentry that is to be opened, if it exists, return it.
 * If it does not exist and O_CREAT is set, creat a regular file of the name
 * @param nd The nd structure representing this path lookup.
 * @param open_flags The open flags of this open() syscall.
 * @return struct dentry* Return the found/created dentry, or NULL if not found.
 */
static struct dentry *lookup_create(struct nameidata *nd, unsigned open_flags)
{
        struct dentry *dir = nd->current;
        struct inode *i_dir = dir->inode;
        struct dentry *dentry;
        int err;

        dentry =
                i_dir->d_ops->dirlookup(i_dir, nd->last.str, (int)nd->last.len);
        if (dentry) {
                return dentry;
        }

        /* not found, create it */
        if (open_flags & O_CREAT) {
                dentry = i_dir->d_ops->alloc_dentry();
                if (CHCORE_IS_ERR(dentry)) {
                        return dentry;
                }

                err = i_dir->d_ops->add_dentry(
                        i_dir, dentry, nd->last.str, nd->last.len);
                if (err) {
                        i_dir->d_ops->free_dentry(dentry);
                        return CHCORE_ERR_PTR(err);
                }

                /* we are not currently handling mode in open() */
                mode_t faked_mode = 0x888;

                err = i_dir->d_ops->mknod(i_dir, dentry, faked_mode, FS_REG);
                if (err) {
                        i_dir->d_ops->remove_dentry(i_dir, dentry);
                        i_dir->d_ops->free_dentry(dentry);
                        return CHCORE_ERR_PTR(err);
                }
        }

        return dentry;
}

/**
 * @brief Used in open when looking up the last component of a path, may create
 * the file if not found.
 * @param nd The nd structure representing this path lookup.
 * @param open_flags The open flags of this open() syscall.
 * @return const char* Return NULL if no symlink encountered or should/can not
 * follow, return the symlink to follow it.
 */
static const char *lookup_last_open(struct nameidata *nd, unsigned open_flags)
{
        struct dentry *dentry;

        if (!nd->last.str) {
                return NULL;
        }

        if ((open_flags & O_CREAT)) {
                /*
                 * when O_CREAT is set
                 * the path should not have trailing slashes
                 */
                if (nd->flags & ND_TRAILING_SLASH) {
                        return CHCORE_ERR_PTR(-EISDIR);
                }
        }

        dentry = lookup_create(nd, open_flags);

        if (!dentry) {
                return CHCORE_ERR_PTR(-ENOENT);
        }

        if (CHCORE_IS_ERR(dentry)) {
                return (char *)dentry;
        }

        return step_into(nd, true, dentry);
}

/**
 * @brief lookup a path except its final component
 * @param name The full pathname to lookup.
 * @param nd The nd structure to represent this pathname lookup and to store
 * state information of this lookup.
 * @return int 0 on success, errno on failure.
 */
static int walk_prefix(const char *name, struct nameidata *nd)
{
        int err;
        if (CHCORE_IS_ERR(name)) {
                return CHCORE_PTR_ERR(name);
        }

        while (*name == '/') {
                name++;
        }

        if (!*name) {
                return 0;
        }

        /* each loop deals with one next path component or get a new symlink */
        for (;;) {
                const char *link;
                u64 component_len = get_component_len(name);

                if (component_len > NAME_MAX) {
                        return -ENAMETOOLONG;
                }

                err = init_string(&nd->last, name, component_len);
                if (err) {
                        return err;
                }

                name += component_len;
                /* skipping postfixing '/'s till next component name */
                while (*name == '/') {
                        name++;
                }

                if (!*name) {
                        if (!nd->depth)
                                /* this is the trailing component */
                                return 0;

                        /* pop a link, continue processing */
                        name = nd->stack[--nd->depth];
                }

                link = walk_component(nd, false);

                /* we have another symlink to process */
                if (link) {
                        if (CHCORE_IS_ERR(link)) {
                                return CHCORE_PTR_ERR(link);
                        }

                        /* checked in step_into() that we have space on stack */
                        nd->stack[nd->depth++] = name; /* store current name */
                        name = link; /* deal with the symlink first */
                        continue;
                }

                /* next loop requires nd->current to be a directory */
                if (nd->current->inode->type != FS_DIR) {
                        return -ENOTDIR;
                }
        }
        return 0;
}

/**
 * @brief Find the parent directory of a given pathname. A very simple wrapper
 * of walk_prefix(). Used by rename(), unlink(), etc.
 * @param nd The nd structure representing this lookup.
 * @param path The full path to lookup.
 * @param flags Some restriction of this lookup can be passed by the flags
 * param.
 * @return int 0 on success, errno on failure.
 * @return struct dentry* Returned by pointer, the parent directory's
 * dentry
 * @return char* Returned in nd->last, the name of the last component of the
 * pathname.
 * @note We **DO NOT** call free_string() here because it is normal
 * for the caller to use nd->last after calling path_parentat() (to
 * create/rename/remove it under the parent directory). It should be viewed as
 * the return value of this call.
 */
int path_parentat(struct nameidata *nd, const char *path, unsigned flags,
                  struct dentry **parent)
{
        init_nd(nd, flags);

        /*
         * there's no need to do the checking of trailing slashes here,
         * since path_parentat never cares about the final component.
         */

        int err = walk_prefix(path, nd);
        if (!err) {
                *parent = nd->current;
        }

        /*
         * we **do not** call free_string() here because it is normal for the
         * caller to use nd->last after calling path_parentat()
         *
         * one should also view nd->last as an output of path_parentat()
         * the subtlety here is annoying but at least we made it clear...
         */
        return err;
}

/**
 * @brief Get the dentry of the full path. Used by: stat(), chmod(), etc.
 * @param nd The nd structure representing this lookup.
 * @param path The full path to lookup.
 * @param flags Some restriction of this lookup can be passed by the flags
 * param.
 * @return int 0 on success, errno on failure.
 * @return struct dentry* Returned by pointer, the final component's
 * dentry.
 * @note We call free_string() here because the caller should never use
 * nd->last after calling path_lookupat().
 */
int path_lookupat(struct nameidata *nd, const char *path, unsigned flags,
                  struct dentry **dentry)
{
        int err;
        init_nd(nd, flags);

        if (path[strlen(path) - 1] == '/') {
                nd->flags |= ND_TRAILING_SLASH | ND_DIRECTORY | ND_FOLLOW;
        }

        while (!(err = walk_prefix(path, nd))
               && (path = lookup_last(nd)) != NULL) {
                ;
        }

        /* requiring a directory(because of trailing slashes) */
        if (!err && (nd->flags & ND_DIRECTORY)
            && nd->current->inode->type != FS_DIR) {
                err = -ENOTDIR;
        }

        if (!err) {
                *dentry = nd->current;
        }

        /*
         * we call free_string() here because the caller should never use
         * nd->last after calling path_lookupat()
         *
         * in other words, the only effective output of path_lookupat
         * is *dentry when no err is encountered.
         */
        free_string(&nd->last);
        return err;
}

/**
 * @brief Called by open(), behaviour is determined by open_flags.
 * We lookup the path, and do special handling of open().
 * @param nd The nd structure representing this lookup.
 * @param path The full path to lookup.
 * @param open_flags The open flags of the open() syscall.
 * @param flags Some restriction of this lookup can be passed by the flags
 * param.
 * @return int 0 on success, errno on failure.
 * @return struct dentry* Returned by pointer, NULL if not found and cannot be
 * created, or the found/created dentry.
 * @note We call free_string() here because the caller should never use
 * nd->last after calling path_openat().
 *
 */
int path_openat(struct nameidata *nd, const char *path, unsigned open_flags,
                unsigned flags, struct dentry **dentry)
{
        int err;

        init_nd(nd, flags);

        if (path[strlen(path) - 1] == '/') {
                nd->flags |= ND_TRAILING_SLASH | ND_DIRECTORY | ND_FOLLOW;
        }

        /* we don't follow symlinks at end by default */
        if (!(open_flags & O_NOFOLLOW)) {
                nd->flags |= ND_FOLLOW;
        }

        if (open_flags & O_DIRECTORY) {
                nd->flags |= ND_DIRECTORY;
        }

        while (!(err = walk_prefix(path, nd))
               && (path = lookup_last_open(nd, open_flags)) != NULL) {
                ;
        }

        if (!err) {
                struct inode *inode = nd->current->inode;

                /* we can check O_CREAT | O_EXCL here, but fs_base handles it */

                if ((open_flags & O_CREAT) && (inode->type == FS_DIR)) {
                        err = -EISDIR;
                        goto error;
                }

                if ((nd->flags & ND_DIRECTORY) && !(inode->type == FS_DIR)) {
                        err = -ENOTDIR;
                        goto error;
                }

                /* we can check O_TRUNCATE here, but fs_base handles it */

                *dentry = nd->current;
        }

error:
        /*
         * we call free_string() here because the caller should never use
         * nd->last after calling path_openat(). the reason is the same
         * as explained in path_lookupat()
         */
        free_string(&nd->last);
        return err;
}