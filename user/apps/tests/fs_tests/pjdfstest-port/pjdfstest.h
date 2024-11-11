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
#include <stdbool.h>
/* Test Functions in pjdfstest */

void ftruncate_00();
void ftruncate_01();
void ftruncate_02();
void ftruncate_03();
void ftruncate_04();
void ftruncate_05();
void ftruncate_06();
void ftruncate_07();
void ftruncate_08();
void ftruncate_09();
void ftruncate_10();
void ftruncate_11();
void ftruncate_12();
void ftruncate_13();
void ftruncate_14();
void mkdir_00();
void mkdir_01();
void mkdir_02();
void mkdir_03();
void mkdir_04();
void mkdir_05();
void mkdir_06();
void mkdir_07();
void mkdir_08();
void mkdir_09();
void mkdir_10();
void mkdir_11();
void mkdir_12();
void rename_00();
void rename_01();
void rename_02();
void rename_03();
void rename_04();
void rename_05();
void rename_06();
void rename_07();
void rename_08();
void rename_09();
void rename_10();
void rename_11();
void rename_12();
void rename_13();
void rename_14();
void rename_15();
void rename_16();
void rename_17();
void rename_18();
void rename_19();
void rename_20();
void rename_21();
void rename_22();
void rename_23();
void rmdir_00();
void rmdir_01();
void rmdir_02();
void rmdir_03();
void rmdir_04();
void rmdir_05();
void rmdir_06();
void rmdir_07();
void rmdir_08();
void rmdir_09();
void rmdir_10();
void rmdir_11();
void rmdir_12();
void rmdir_13();
void rmdir_14();
void rmdir_15();
void unlink_00();
void unlink_01();
void unlink_02();
void unlink_03();
void unlink_04();
void unlink_05();
void unlink_06();
void unlink_07();
void unlink_08();
void unlink_09();
void unlink_10();
void unlink_11();
void unlink_12();
void unlink_13();
void unlink_14();

struct pjdtest_entry {
        void (*test_func)();
        bool linux_flag;
        bool chcore_flag;
};

extern const struct pjdtest_entry pjdtest_entrys[];
extern int pjdtest_nr_entrys;

void run_pjdtest();
