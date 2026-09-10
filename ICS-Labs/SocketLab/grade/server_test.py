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
    subprocess_client_ref,
    message_client,
)
from typing import List
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


async def one_client() -> List[str]:
    # get the event loop
    loop = asyncio.get_event_loop()

    # create a message client
    return await message_client(["Hello", "Hello"], loop)


async def ten_client_simultaneously(ref_output: List[str]) -> bool:
    # get the event loop
    loop = asyncio.get_event_loop()

    # send 10 messages to the server
    tasks = [message_client(["Hello", "Hello"], loop) for _ in range(10)]
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
    server_process = open_server(subprocess_server)
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

    # stop server and client
    client_process.terminate()
    server_process.terminate()

    # wait for the server to stop completely
    while server_process.poll() is None:
        time.sleep(0.1)

    # use message_client to get reference output
    server_process = open_server(subprocess_server_ref)
    client_process = open_client(subprocess_client_ref)
    ref_output = asyncio.run(one_client())

    # stop the server and client
    client_process.terminate()
    server_process.terminate()

    # wait for the server to stop completely
    while server_process.poll() is None:
        time.sleep(0.1)

    # open the C server
    server_process = open_server(subprocess_server)
    client_process = open_client(subprocess_client_ref)

    # start 10 message clients
    if not asyncio.run(ten_client_simultaneously(ref_output)):
        # stop server and client
        client_process.terminate()
        server_process.terminate()

        # print the score and return
        print("Final Score: 0")
        return

    # stop server and client
    client_process.terminate()
    server_process.terminate()

    # print the score
    print("Final Score: 30")


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(e)
        print("Final Score: 0")
