#
# Copyright (c) 2025 Institute of Parallel And Distributed Systems (IPADS),
# Shanghai Jiao Tong University (SJTU) Licensed under the Mulan PSL v2. You can
# use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
# KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
# NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE. See the
# Mulan PSL v2 for more details.
#

import sys
import socket
import json
import threading
from typing import List
from message_model import SocketMessageModel


################ Atomic Operations ################

# An atomic counter to keep track of sockets
global_counter = 0
counter_lock = threading.Lock()


def increase_and_read_counter() -> int:
    with counter_lock:
        global global_counter
        global_counter += 1
        return global_counter


################ Atomic Operations ################


################## Some Globals ###################

host = "127.0.0.1"
port = 12345

################## Some Globals ###################


############## External Declaration ###############

cdef extern from "llama/simple_llama_chat.h":
    int initialize_llama_chat(int argc, char **argv);
    void free_llama_chat();
    int add_chat_user(const char *user_name);
    int remove_chat_user(const char *user_name);
    int quest_for_response(int conn, const char *client_name,
                        const char *current_input);

############## External Declaration ###############


#################### C Imports ####################

from libc.stdlib cimport malloc, free
from libc.string cimport strdup

#################### C Imports ####################


################## Pyx Overrides ##################

def py_send_token(conn: int, token: str) -> None:
    """
    Send a token to the client.
    """
    s = socket.socket(fileno=conn)
    msg = json.dumps({"token": token, "eog": False})
    s.sendall(msg.encode())
    s.detach()


def py_send_eog_token(conn: int) -> None:
    """
    Send an end-of-generation token to the client.
    """
    s = socket.socket(fileno=conn)
    msg = json.dumps({"token": "", "eog": True})
    s.sendall(msg.encode())
    s.detach()


cdef public void send_token(int conn, const char *token):
    token_bytes = token.decode('utf-8')
    py_send_token(conn, token_bytes)


cdef public void send_eog_token(int conn):
    py_send_eog_token(conn)


################## Pyx Overrides ##################


################### Pyx Wrapper ###################


def initialize_llama_chat_wrapper(argv: List[str]) -> int:
    """
    Initialize the llama chat wrapper.
    """
    # declare the variables
    cdef int i
    cdef int argc = len(argv)

    # duplicate the arguments to pass to C
    cdef char** c_argv = <char**> malloc(argc * sizeof(char*))
    for i in range(argc):
        c_argv[i] = strdup(argv[i].encode('utf-8'))

    # initialize the llama chat
    cdef int ret = initialize_llama_chat(argc, c_argv)

    # free the allocated memory and then return
    for i in range(argc):
        free(c_argv[i])
    free(c_argv)
    return ret


################### Pyx Wrapper ###################


def handle_client(conn: socket.socket, addr):
    # print the address of the client
    print(f"Client connected from: {addr}")

    # create a unique identifier for the socket
    client_name = f"client_{increase_and_read_counter()}"
    add_chat_user(client_name.encode())

    try:
        while True:
            # get the message from the client
            message = conn.recv(1024).decode()
            if not message:
                print(f"Client {addr} closed the connection.")
                break
            print(f"Received {addr}: {message}")

            # convert the message to JSON
            message = SocketMessageModel(json_str=message)
            assert message.eog == True

            # quest for response
            quest_for_response(
                conn.fileno(),
                client_name.encode(),
                message.token.encode(),
            )

            # add a newline for better readability
            print()
    except KeyboardInterrupt:
        print("Server interrupted by user.")
    except Exception as e:
        print("Error: ", e)
    finally:
        conn.close()
        remove_chat_user(client_name.encode())
        print(f"Connection to {addr} has been closed.")


def server():
    # initialize the LLM instance
    if len(sys.argv) != 3 or sys.argv[1] != "-m":
        print(f"Usage: python3 {sys.argv[0]} -m <model_path>")
        sys.exit(1)

    if initialize_llama_chat_wrapper(sys.argv) != 0:
        print("Failed to initialize llama server.")
        sys.exit(1)

    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    # bind the socket to the specified host and port
    server_socket.bind((host, port))
    server_socket.listen()
    print(f"The server has been started, listening on: {host}:{port}")

    try:
        while True:
            conn, addr = server_socket.accept()
            client_thread = threading.Thread(
                target=handle_client,
                args=(conn, addr),
                daemon=True,
            )
            client_thread.start()
    except KeyboardInterrupt:
        print("Server interrupted by user.")
    except Exception as e:
        print("Error: ", e)
    finally:
        server_socket.close()
        free_llama_chat()
