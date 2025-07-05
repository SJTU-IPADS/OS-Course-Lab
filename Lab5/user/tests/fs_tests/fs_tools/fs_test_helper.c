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

#include "fs_test.h"
#include <time.h>
#include <stdlib.h>

#ifdef FS_TEST_LINUX

#define PATH_JOIN_SEPERATOR "/"

static int __str_starts_with(const char *str, const char *start)
{
        for (;; str++, start++)
                if (!*start)
                        return 1;
                else if (*str != *start)
                        return 0;
}

static int __str_ends_with(const char *str, const char *end)
{
        int end_len;
        int str_len;

        if (NULL == str || NULL == end)
                return 0;

        end_len = strlen(end);
        str_len = strlen(str);

        return str_len < end_len ? 0 : !strcmp(str + str_len - end_len, end);
}

char *path_join(const char *dir, const char *file)
{
        char *filecopy;
        int size;
        char *buf;

        if (!dir)
                dir = "\0";
        if (!file)
                file = "\0";

        size = strlen(dir) + strlen(file) + 2;
        buf = malloc(size * sizeof(char));
        if (NULL == buf)
                return NULL;

        strcpy(buf, dir);

        /* add the sep if necessary */
        if (!__str_ends_with(dir, PATH_JOIN_SEPERATOR))
                strcat(buf, PATH_JOIN_SEPERATOR);

        /* remove the sep if necessary */
        if (__str_starts_with(file, PATH_JOIN_SEPERATOR)) {
                filecopy = strdup(file);
                if (NULL == filecopy) {
                        free(buf);
                        return NULL;
                }
                strcat(buf, ++filecopy);
                free(--filecopy);
        } else {
                strcat(buf, file);
        }

        return buf;
}

#endif

char *str_rand(int len)
{
        char *ret;
        int i;

        ret = (char *)malloc(len + 1);
        for (i = 0; i < len; i++) {
                ret[i] = rand() % ('z' - 'a' + 1) + 'a';
        }
        ret[len] = '\0';

        return ret;
}

char *str_plus(char *s1, char *s2)
{
        int l;
        char *ret;

        l = strlen(s1) + strlen(s2) + 1;
        ret = (char *)malloc(l);
        sprintf(ret, "%s%s", s1, s2);

        // Sanity Check
        assert(strlen(s1) + strlen(s2) == strlen(ret));

        return ret;
}

char *namegen_max(void)
{
#ifdef FS_TEST_LINUX
        return str_rand(255);
#else
        char cwd[4096];
        getcwd(cwd, 4096);
        // {cwd}/{ret} length = FS_REQ_PATH_LEN
        return str_rand(FS_REQ_PATH_LEN - 1 - strlen(cwd));
#endif
}

char *dirgen_max(void)
{
#ifdef FS_TEST_LINUX
        // PC_NAME_MAX = 255, PC_PATH_MAX = 4096
        char *ret;
        char *tmp;
        int i;
        int iters;

        // Generate dir max string, 15 NAME_MAX with '/' joint
        ret = (char *)malloc(4097);
        iters = (4096 / (255 + 1));
        for (i = 0; i < iters; i++) {
                tmp = str_rand(255);
                memcpy(ret + 256 * i, tmp, 255);
                ret[256 * (i + 1) - 1] = '/';
                free(tmp);
        }
        ret[iters * 256 - 1] = '\0';
        return ret;
#else
        char cwd[4096];
        char *ret;
        int ret_len;
        int i;
        char *tmp;

        getcwd(cwd, 4096);
        ret_len = FS_REQ_PATH_LEN - 1 - strlen(cwd);
        ret = (char *)malloc(ret_len + 1);
        memset(ret, 0, ret_len + 1);

        for (i = 0; i < ret_len - 32; i += 32) {
                tmp = str_rand(31);
                ret[i - 1] = '/';
                memcpy(ret + i, tmp, 31);
                free(tmp);
        }

        tmp = str_rand(ret_len - i);
        ret[i - 1] = '/';
        memcpy(ret + i, tmp, ret_len - i);
        ret[ret_len] = '\0';
        free(tmp);

        assert(strlen(ret) == ret_len);
        return ret;
#endif
}

void mkdir_p_dirmax(char *dirmax)
{
#ifdef FS_TEST_LINUX
        int iters;
        int i;
        char cwd[4096];
        char tmp[256];

        getcwd(cwd, 4096);
        iters = (4096 / (255 + 1)) - 1;
        for (i = 0; i < iters; i++) {
                strncpy(tmp, dirmax + 256 * i, 255);
                tmp[255] = '\0';
                fs_assert_zero(mkdir(tmp, 0));
                fs_assert_zero(chdir(tmp));
        }
        fs_assert_zero(chdir(cwd));
#else
        char cwd[4096];
        int iters;
        int i;
        char tmp[256];

        getcwd(cwd, 4096);
        iters = (strlen(dirmax) / 32);
        for (i = 0; i < iters; i++) {
                strncpy(tmp, dirmax + 32 * i, 31);
                tmp[31] = '\0';
                fs_assert_zero(mkdir(tmp, S_IRUSR | S_IWUSR | S_IXUSR));
                fs_assert_zero(chdir(tmp));
        }
        fs_assert_zero(chdir(cwd));
#endif
}

void rmdir_r_dirmax(char *dirmax)
{
#ifdef FS_TEST_LINUX
        int iters;
        int i;

        iters = (4096 / (255 + 1)) - 1;
        for (i = iters; i > 0; i--) {
                dirmax[256 * i - 1] = '\0';
                fs_assert_zero(rmdir(dirmax));
        }
#else
        int iters;
        int i;

        iters = (strlen(dirmax) / 32);
        for (i = iters; i > 0; i--) {
                dirmax[32 * i - 1] = '\0';
                fs_assert_zero(rmdir(dirmax));
        }
#endif
}
