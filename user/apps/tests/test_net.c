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

#include <chcore/launcher.h>
#include <chcore/proc.h>

#include "test_tools.h"

#define LOGFILE_PATH "test_net.log"

int main(int argc, char *argv[])
{
        int pid, status = -1;
        const char *info_content = "LWIP tcp server up!";

        info("Test net begins...\n");

        /* Create server process */
        argv[0] = "/test_net_server.bin";
        pid = create_process(argc, argv, NULL);
        chcore_assert(pid > 0, "create process %s failed!", argv[0]);
        expect_log(LOGFILE_PATH, info_content);

        /* Create client process and wait for return*/
        argv[0] = "/test_net_client.bin";
        create_process(argc, argv, NULL);
        chcore_assert(pid > 0, "create process %s failed!", argv[0]);
        chcore_waitpid(pid, &status, 0, 0);
        chcore_assert(status == 0, "test_net_client failed! status = %d", status);

        /* UDP big package send/recv TEST*/
        argv[0] = "/test_udp_server.bin";
        pid = create_process(argc, argv, NULL);
        chcore_assert(pid > 0, "create process %s failed!", argv[0]);
        chcore_waitpid(pid, &status, 0, 0);
        chcore_assert(status == 0, "test_udp_server failed! status = %d", status);

        green_info("Test net finished!\n");

        return 0;
}