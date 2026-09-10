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

import asyncio
import os
import socket
import subprocess
from typing import List
from message_model import SocketMessageModel

################## Some Globals ###################

host = "127.0.0.1"
port = 12345

################## Some Globals ###################


def subprocess_server_ref() -> subprocess.Popen[bytes]:
    return subprocess.Popen(
        ["python3", "./ref/server.py", "-m", "models/model.gguf"],
        stdout=subprocess.PIPE,
        stdin=subprocess.PIPE,
    )


def subprocess_server() -> subprocess.Popen[bytes]:
    return subprocess.Popen(
        ["./build/src/server", "-m", "models/model.gguf"],
        stdout=subprocess.PIPE,
        stdin=subprocess.PIPE,
    )


def subprocess_server_epoll() -> subprocess.Popen[bytes]:
    return subprocess.Popen(
        ["./build/src/server_epoll", "-m", "models/model.gguf"],
        stdout=subprocess.PIPE,
        stdin=subprocess.PIPE,
    )


def subprocess_client_ref() -> subprocess.Popen[bytes]:
    return subprocess.Popen(
        ["python3", "./ref/client.py", "-m", "models/model.gguf"],
        stdout=subprocess.PIPE,
        stdin=subprocess.PIPE,
    )


def subprocess_client() -> subprocess.Popen[bytes]:
    return subprocess.Popen(
        ["./build/src/client", "-m", "models/model.gguf"],
        stdout=subprocess.PIPE,
        stdin=subprocess.PIPE,
    )


def get_threads_count(pid: int) -> int:
    return int(os.popen(f"ps -T -p {pid} | wc -l").read().strip()) - 1


async def silent_client(loop: asyncio.AbstractEventLoop) -> socket.socket:
    # create non-blocking socket
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client_socket.setblocking(False)

    # connect to the server
    await loop.sock_connect(client_socket, (host, port))
    print(f"Connected to {host}:{port}")
    return client_socket


async def message_client(
    messages: List[str], loop: asyncio.AbstractEventLoop
) -> List[str]:
    # create non-blocking socket
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client_socket.setblocking(False)

    # connect to the server
    await loop.sock_connect(client_socket, (host, port))
    print(f"Connected to {host}:{port}")

    received_messages = []
    for message in messages:
        # send message to the server
        await loop.sock_sendall(
            client_socket,
            str(SocketMessageModel(token=message, eog=True)).encode(),
        )

        received_message = "> "
        while True:
            # receive the message from the server
            data = await loop.sock_recv(client_socket, 1024)
            if not data:
                print("Server closed the connection.")
                break

            # decode the received message and check if it is the end of the text
            current_message = SocketMessageModel(json_str=data.decode())
            if current_message.eog:
                break

            # append the received message to the final message
            received_message += current_message.token

        # append the received message to the list of received messages
        received_messages.append(received_message)

    # close the client socket and return the received message
    client_socket.close()
    return received_messages
