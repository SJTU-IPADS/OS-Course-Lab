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

#include <stdio.h>
#include <chcore/pmu.h>
#include <inttypes.h>


int main(int argc, char *argv[])
{
        u64 start = 0, end = 0;

        pmu_clear_cnt();
        start = pmu_read_real_cycle();
        printf("Begin at cycle %" PRId64 "\n", start);
        end = pmu_read_real_cycle();
        printf("End at cycle %" PRId64 "\n", end);
        printf("Total cycle %" PRId64 "\n", end - start);

        return 0;
}
