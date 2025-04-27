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
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "llama/simple_llama_chat.h"
#include "json/simple_json.h"

__attribute__((unused)) static _Atomic int client_num = 0;

void send_token(int conn, const char *token) {
    // TODO: convert the token with a specific format

    // TODO: send `message` to the client with file descriptor `conn`
}

void send_eog_token(int conn) {
    // TODO: convert the token with a specific format

    // TODO: send `message` to the client with file descriptor `conn`
}

static void set_socket_reusable(int socket) {
    // allow the socket to be reused immediately after the server is closed
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

/********************** we need to create threads here **********************/

void *handle_client(void *arg) {
    // TODO: handle the client connection

    // TODO: infinite loop to receive messages from the client
    //       if any, process the message and send a response

    // TODO: if the client disconnects, close the connection and free resources
    return NULL;
}

/********************** we need to create threads here **********************/

int main(int argc, char *argv[]) {
    // initialize llama.cpp
    if (initialize_llama_chat(argc, argv) != 0) {
        fprintf(stderr, "Failed to initialize llama server.\n");
        return 1;
    }

    // TODO: create a socket file descriptor for the server socket
    int server_socket = 0;

    // NOTE: this shouldn't be discarded to reuse the address and port
    set_socket_reusable(server_socket);

    // bind the socket to an address and port and start listening
    bind_socket_addr_port(server_socket, SERVER_ADDR, SERVER_PORT);
    listen_socket(server_socket);
    printf("The server has been started, listening on: %s:%d\n", SERVER_ADDR,
           SERVER_PORT);

    while (1) {
        // TODO: accept a new incoming connection
        // THINK: why we need to allocate memory on the heap for
        //        the client socket
        int *client_socket = (int *)malloc(sizeof(int));

        // TODO: create a new thread (`handle_client`) to handle the client
        //       you may need to invoke `pthread_detach` to avoid memory leaks
        pthread_t client_thread;
    }

    // close the server socket
    close(server_socket);

    // free llama.cpp resources
    free_llama_chat();
    return 0;
}
