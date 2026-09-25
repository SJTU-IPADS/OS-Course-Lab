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
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "config.h"
#include "llama/simple_llama_chat.h"
#include "json/simple_json.h"

#define MAX_EVENTS 10

void send_token(int conn, const char *token) {
    // TODO: convert the token with a specific format

    // TODO: send `message` to the client with file descriptor `conn`
}

void send_eog_token(int conn) {
    // TODO: convert the token with a specific format

    // TODO: send `message` to the client with file descriptor `conn`
}

static void set_socket_reusable(int socket) {
    int opt = 1;
    if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        close(socket);
        exit(EXIT_FAILURE);
    }
}

static void bind_socket_addr_port(int socket, const char *addr, int port) {
    // TODO: bind the server socket to the specified IP address and port
}

static void listen_socket(int socket) {
    // TODO: listen for incoming connections
}

static void set_nonblocking(int fd) {
    // set the socket to non-blocking mode
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        perror("fcntl(F_GETFL)");
        exit(EXIT_FAILURE);
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("fcntl(F_SETFL)");
        exit(EXIT_FAILURE);
    }
}

static void add_to_epoll(int epoll_fd, int fd) {
    // TODO: add the file descriptor `fd` to the epoll instance `epoll_fd`
    //       for watching input events
}

/********************** we need to create threads here **********************/

struct quest_args {
    int conn;
    char message[MAX_BUFFER_SIZE];
};

void *handle_response(void *arg) {
    // here we need to call quest and send the response back
    struct quest_args *args = (struct quest_args *)arg;

    // TODO: parse the message and generate a response

    quest_for_response(args->conn, NULL, NULL);

    // free resources
    free(args);
    return NULL;
}

/********************** we need to create threads here **********************/

int main(int argc, char *argv[]) {
    // initialize llama.cpp
    if (initialize_llama_chat(argc, argv) != 0) {
        fprintf(stderr, "Failed to initialize llama server.\n");
        return 1;
    }

    // TODO: create the server socket
    int server_socket = 0;

    // NOTE: this shouldn't be discarded to reuse the address and port
    set_socket_reusable(server_socket);

    // bind the server socket to the specified port and start listening
    bind_socket_addr_port(server_socket, SERVER_ADDR, SERVER_PORT);
    listen_socket(server_socket);
    printf("Server listening on %s:%d\n", SERVER_ADDR, SERVER_PORT);

    // NOTE: set server socket to non-blocking mode in order to use epoll
    set_nonblocking(server_socket);

    // TODO: create epoll instance
    int epoll_fd;

    // TODO: implement epoll instance using the logic from the README.md
    while (1) {
        // TODO: wait for epoll events

        // TODO: handle epoll events
    }

    // close the server socket
    close(server_socket);
    close(epoll_fd);

    // free llama.cpp resources
    free_llama_chat();
    return 0;
}
