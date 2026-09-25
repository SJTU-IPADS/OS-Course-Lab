/*
 * Copyright (c) 2025 Institute of Parallel And Distributed Systems (IPADS),
 * Shanghai Jiao Tong University (SJTU) Licensed under the Mulan PSL v2. You can
 * use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 * NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE. See the
 * Mulan PSL v2 for more details.
 */

#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "json/simple_json.h"

__attribute__((unused)) static const char *json_object_name = "llama_client";

void handle_sigint(int signum) {
    (void)signum;
    printf("\nClient interrupted by user.\n");
    exit(0);
}

int connect_to_server() {
    // TODO: create socket file descriptor
    int sockfd;

    // TODO: set server address with "SERVER_ADDR:SERVER_PORT"
    struct sockaddr_in serv_addr;

    // TODO: connect to the server
    return sockfd;
}

char *get_input() {
    // read user input from stdin
    char *input = NULL;
    size_t len = 0;
    ssize_t nread = 0;
    do {
        printf("> ");
        fflush(stdout);
        nread = getline(&input, &len, stdin);
    } while (nread <= 1); // ignore empty input

    // ignore "\n" at the end of the input
    input[nread - 1] = '\0';
    return input;
}

int main() {
    // register SIGINT handler
    signal(SIGINT, handle_sigint);

    // connect to the server
    int sockfd = connect_to_server();
    if (sockfd < 0) {
        return -1;
    }

    // TODO: initialize json object

    // TODO: infinite loop to send user input to the server and receive response
    while (1) {
        // TODO: read user input from stdin (not empty)

        // TODO: format the input and send it to the server

        // TODO: receive response from the server
        // NOTE: you need to parse the response of a specific format
    }

    // close socket file descriptor and return
    close(sockfd);
    return 0;
}
