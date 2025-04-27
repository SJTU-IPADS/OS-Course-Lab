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
    subprocess_server_ref,
    subprocess_server,
    subprocess_server_epoll,
    subprocess_client_ref,
    silent_client,
    message_client,
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


async def open_10_message_clients(ref_output: str) -> bool:
    # get the event loop
    loop = asyncio.get_event_loop()

    # send 10 messages to the server
    tasks = [message_client("Hello", loop) for _ in range(10)]
    results = await asyncio.gather(*tasks)

    # check if the client output is the same as the reference output
    for result in results:
        if result != ref_output:
            print(f"Client Output:     {result}")
            print(f"Reference Output:  {ref_output}")
            print(f"Final Score:       0")
            return False
    return True


def main() -> None:
    # open the reference Python client and server
    server_process = open_server(subprocess_server_ref)
    client_process = open_client(subprocess_client_ref)

    # write "Hello" to stdin of the client
    client_process.stdin.write(b"Hello\n")
    client_process.stdin.flush()
    ref_output = client_process.stdout.readline().decode().strip()

    # stop server and client
    client_process.terminate()
    server_process.terminate()

    # wait for the server to stop completely
    while server_process.poll() is None:
        time.sleep(0.1)

    # open the C server
    server_process = open_server(subprocess_server_epoll)
    client_process = open_client(subprocess_client_ref)

    # write "Hello" to stdin of the client
    client_process.stdin.write(b"Hello\n")
    client_process.stdin.flush()
    client_output = client_process.stdout.readline().decode().strip()

    # check if the client output is the same as the reference output
    if client_output != ref_output:
        # stop server and client
        client_process.terminate()
        server_process.terminate()

        # print the score
        print(f"Client Output:     {client_output}")
        print(f"Reference Output:  {ref_output}")
        print(f"Final Score:       0")
        return

    # terminate the client
    client_process.terminate()

    # test the time used to deal with 10 messages
    start_time = time.time()
    if not asyncio.run(open_10_message_clients(ref_output)):
        # stop the server, print the score and return
        server_process.terminate()
        print("Final Score: 0")
        return

    # record the time used
    end_time = time.time()
    time_used_10_messages = end_time - start_time

    # open 1024 silent clients
    clients = asyncio.run(open_1024_silent_clients())

    # test the time used to deal with 10 messages
    start_time = time.time()
    if not asyncio.run(open_10_message_clients(ref_output)):
        # stop the server, print the score and return
        server_process.terminate()
        for client in clients:
            client.close()

        print("Final Score: 0")
        return

    # record the time used
    end_time = time.time()
    time_used_1024_clients = end_time - start_time

    # close all clients
    for client in clients:
        client.close()

    # stop the server
    server_process.terminate()

    # calculate the time difference
    diff = time_used_1024_clients - time_used_10_messages
    print(f"Time Difference: {diff}")
    print(f"Final Score:     {20 if diff < 5.0 else 0}")


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(e)
        print("Final Score: 0")
