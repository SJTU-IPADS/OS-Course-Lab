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

#include "pjdfstest.h"
#include <stdio.h>

const struct pjdtest_entry pjdtest_entrys[] = {
        /* {TEST_CASE, LINUX, CHCORE} */
        {ftruncate_00, true, true}, {ftruncate_01, true, true},
        {ftruncate_02, true, true}, {ftruncate_03, true, true},
        {ftruncate_04, true, true}, {ftruncate_05, true, true},
        {ftruncate_06, true, true}, {ftruncate_07, true, true},
        {ftruncate_08, true, true}, {ftruncate_09, true, true},
        {ftruncate_10, true, true}, {ftruncate_11, true, true},
        {ftruncate_12, true, true}, {ftruncate_13, true, true},
        {ftruncate_14, true, true}, {mkdir_00, true, true},
        {mkdir_01, true, true},     {mkdir_02, true, true},
        {mkdir_03, true, true},     {mkdir_04, true, true},
        {mkdir_05, true, true},     {mkdir_06, true, true},
        {mkdir_07, true, true},     {mkdir_08, true, true},
        {mkdir_09, true, true},     {mkdir_10, true, true},
        {mkdir_11, true, true},     {mkdir_12, true, true},
        {rename_00, true, true},    {rename_01, true, true},
        {rename_02, true, true},    {rename_03, true, true},
        {rename_04, true, true},    {rename_05, true, true},
        {rename_06, true, true},    {rename_07, true, true},
        {rename_08, true, true},    {rename_09, true, true},
        {rename_10, true, true},    {rename_11, true, true},
        {rename_12, true, true},    {rename_13, true, true},
        {rename_14, true, true},    {rename_15, true, true},
        {rename_16, true, true},    {rename_17, true, true},
        {rename_18, true, true},    {rename_19, true, true},
        {rename_20, true, true},    {rename_21, true, true},
        {rename_22, true, true},    {rename_23, true, true},
        {rmdir_00, true, true},     {rmdir_01, true, true},
        {rmdir_02, true, true},     {rmdir_03, true, true},
        {rmdir_04, true, true},     {rmdir_05, true, true},
        {rmdir_06, true, true},     {rmdir_07, true, true},
        {rmdir_08, true, true},     {rmdir_09, true, true},
        {rmdir_10, true, true},     {rmdir_11, true, true},
        {rmdir_12, true, true},     {rmdir_13, true, true},
        {rmdir_14, true, true},     {rmdir_15, true, true},
        {unlink_00, true, true},    {unlink_01, true, true},
        {unlink_02, true, true},    {unlink_03, true, true},
        {unlink_04, true, true},    {unlink_05, true, true},
        {unlink_06, true, true},    {unlink_07, true, true},
        {unlink_08, true, true},    {unlink_09, true, true},
        {unlink_10, true, true},    {unlink_11, true, true},
        {unlink_12, true, true},    {unlink_13, true, true},
        {unlink_14, true, false}, /* fat & ext do not support */
};

int pjdtest_nr_entrys = sizeof(pjdtest_entrys) / sizeof(pjdtest_entrys[0]);

void run_pjdtest()
{
        int i;

        printf("\nRunning pjdfstest\n");
        for (i = 0; i < pjdtest_nr_entrys; i++) {
#ifdef FS_TEST_LINUX
                if (pjdtest_entrys[i].linux_flag)
                        pjdtest_entrys[i].test_func();
#else
                if (pjdtest_entrys[i].chcore_flag)
                        pjdtest_entrys[i].test_func();
#endif
        }
}