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

#include "llama/simple_llama_chat.h"
#include <stdio.h>
#include <string.h>

static char buffer[4096];
static int buffer_pos = 0;

void send_token(int conn, const char *token) {
    // We only append the token to the buffer for simulation purposes.
    (void)conn;
    // Append the token to the buffer
    snprintf(buffer + buffer_pos, sizeof(buffer) - buffer_pos, "%s", token);
    buffer_pos += strlen(token);
}

void send_eog_token(int conn) {
    // We only print the response to the console for simulation purposes.
    (void)conn;
    printf("\nThe current response has ended\n");
    // Note: If you are interested, print the buffer here
    buffer_pos = 0; // Reset the buffer position
}

static void quest_wrapper(int conn, const char *client_name,
                          const char *message) {
    // Log the message to the console
    printf("User %s asks: %s\n", client_name, message);
    printf("Response: ");

    // Call the quest function to get the response
    quest_for_response(conn, client_name, message);
    printf("\n");
}

int main(int argc, char *argv[]) {
    // Parse command line arguments
    if (argc != 3 || strcmp(argv[1], "-m") != 0) {
        fprintf(stderr, "Usage: %s -m <model_path>\n", argv[0]);
        return 1;
    }

    // Example usage of the LLaMA CLI
    if (initialize_llama_chat(argc, argv) != 0)
        return 1;

    // Add clients
    const char *client_names[] = {"llama_client_1", "llama_client_2"};
    for (int i = 0; i < sizeof(client_names) / sizeof(client_names[0]); ++i) {
        if (add_chat_user(client_names[i]) != 0) {
            fprintf(stderr, "Failed to add client: %s\n", client_names[i]);
            free_llama_chat();
            return 1;
        }
    }

    // Execute the quest function for the client
    quest_wrapper(0, client_names[0], "请记住我是ICS助教。");
    quest_wrapper(0, client_names[0], "请问我是谁？");

    // Execute the quest function using another client
    // The second client will not be able to see the first client's messages
    quest_wrapper(0, client_names[1], "请问我是谁？");

    // Free the llama chat resources
    for (int i = 0; i < sizeof(client_names) / sizeof(client_names[0]); ++i) {
        if (remove_chat_user(client_names[i]) != 0) {
            fprintf(stderr, "Failed to remove client: %s\n", client_names[i]);
        }
    }

    // Free the llama CLI resources
    free_llama_chat();
    return 0;
}
