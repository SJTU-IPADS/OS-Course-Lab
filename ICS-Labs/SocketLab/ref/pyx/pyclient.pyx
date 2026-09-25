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

import socket
from message_model import SocketMessageModel

################## Some Globals ###################

host = "127.0.0.1"
port = 12345

################## Some Globals ###################


def get_input() -> str:
    # empty message is not allowed
    message = input("> ")
    while message.strip() == "":
        message = input()
    return message


def client():
    # create a TCP/IP socket
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # connect to the specified server and port
    client_socket.connect((host, port))
    print(f"Connected to {host}:{port}")

    try:
        while True:
            # get the user input
            message = get_input()
            # send the user input to the server
            client_socket.sendall(
                str(SocketMessageModel(token=message, eog=True)).encode()
            )

            while True:
                # receive the response from the server
                data = client_socket.recv(1024)
                if not data:
                    print("Server closed the connection.")
                    break

                # decode the received data and print it
                received_message = SocketMessageModel(json_str=data.decode())
                if received_message.eog:
                    break

                # print the received message without a newline
                print(received_message.token, end="", flush=True)

            # start a new line at the end of the response
            print()
    except KeyboardInterrupt:
        print("Client interrupted by user.")
    except Exception as e:
        print("Error: ", e)
    finally:
        client_socket.close()
