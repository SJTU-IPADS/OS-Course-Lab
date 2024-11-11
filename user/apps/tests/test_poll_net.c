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
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "test_tools.h"

#define LOGFILE_PATH "test_poll.log"

int main()
{
        char *server_argv[1];
        char *client_argv[2];
        int server_pid, client_pid;
        int server_ret, client_ret;
        FILE *file;
        const char *info_content = "LWIP server up!";
        char str[20];

        info("Test poll begins...\n");

        file = fopen(LOGFILE_PATH, "w+");

        /* Create server process */
        server_argv[0] = "/poll_server.bin";
        server_pid = create_process(1, server_argv, NULL);
        chcore_assert(server_pid >= 0, "create server process failed!");

        /* Wait until the server is ready */
        while (1) {
                rewind(file);
                fread(str, strlen(info_content) + 1, 1, file);
                if (strcmp(str, info_content) == 0)
                        break;
        }

        /* Create client process */
        client_argv[0] = "/poll_client.bin";
        client_argv[1] = "9002";
        client_pid = create_process(2, client_argv, NULL);
        chcore_assert(server_pid >= 0, "create client process failed!");

        /* Wait the server and the client to finish */
        chcore_waitpid(server_pid, &server_ret, 0, 0);
        chcore_waitpid(client_pid, &client_ret, 0, 0);

        /* Clean up */
        fclose(file);
        unlink(LOGFILE_PATH);

        chcore_assert(server_ret == 0, "test poll server failed");
        chcore_assert(client_ret == 0, "test poll client failed");

        green_info("Test poll finished!\n");

        return 0;
}
