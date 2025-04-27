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
import time
import subprocess
from common import (
    subprocess_server_epoll,
    subprocess_client_ref,
    silent_client,
    message_client,
    get_threads_count,
)
from common_exceptions import ServerLaunchFailure, ClientLaunchFailure


def open_server(func: callable) -> subprocess.Popen[bytes]:
    try:
        return func()
    except Exception as e:
        raise ServerLaunchFailure("Server failed to start") from e


def open_client(func: callable) -> subprocess.Popen[bytes]:
    # try to start the client
    for _ in range(5):
        # create a new client process
        client_process = func()

        # check if the client has connected to the server
        client_output = client_process.stdout.readline().decode().strip()
        if client_output.split(" ")[0].lower() != "connected":
            client_process.terminate()
            time.sleep(1.0)
        else:
            return client_process

    raise ClientLaunchFailure("Client failed to connect to the server")


async def open_1024_silent_clients() -> list:
    # create 1024 silent clients
    loop = asyncio.get_event_loop()
    tasks = [silent_client(loop) for _ in range(1024)]
    return await asyncio.gather(*tasks)


async def open_10_message_clients() -> None:
    # get the event loop
    loop = asyncio.get_event_loop()

    # send 10 messages to the server
    tasks = [message_client("Hello", loop) for _ in range(10)]
    _ = await asyncio.gather(*tasks)


def main() -> None:
    # open the C server
    server_process = open_server(subprocess_server_epoll)
    client_process = open_client(subprocess_client_ref)

    # write "Hello" to stdin of the client
    client_process.stdin.write(b"Hello\n")
    client_process.stdin.flush()
    _ = client_process.stdout.readline().decode().strip()

    # get the thread count of the server
    threads = get_threads_count(server_process.pid)
    if threads != 1:
        # close the server and client
        client_process.terminate()
        server_process.terminate()

        # print the final score
        print(f"Your threads:        {threads}")
        print(f"Epected threads:     1")
        print(f"Final Score:         0")
        return

    # terminate the client
    client_process.terminate()

    # open 1024 silent clients
    clients = asyncio.run(open_1024_silent_clients())

    # get the thread count of the server
    threads = get_threads_count(server_process.pid)
    if threads != 1:
        # close the server and clients
        for client in clients:
            client.close()
        server_process.terminate()

        # print the final score
        print(f"Your threads:        {threads}")
        print(f"Epected threads:     1")
        print(f"Final Score:         0")
        return

    # send one message to the server
    asyncio.run(open_10_message_clients())

    # get the thread count of the server
    threads = get_threads_count(server_process.pid)
    if threads != 1:
        # close the server and clients
        for client in clients:
            client.close()
        server_process.terminate()

        # print the final score
        print(f"Your threads:        {threads}")
        print(f"Epected threads:     1")
        print(f"Final Score:         0")
        return

    # close the server and clients
    for client in clients:
        client.close()
    server_process.terminate()

    # print the final score
    print(f"Final Score: 30")


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(e)
        print("Final Score: 0")
