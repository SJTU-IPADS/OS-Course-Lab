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

import time
import subprocess
from common import (
    subprocess_server_ref,
    subprocess_client_ref,
    subprocess_client,
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


def main() -> None:
    server_process = open_server(subprocess_server_ref)
    client_process = open_client(subprocess_client)

    # write "Hello" to stdin of the client
    client_process.stdin.write(b"Hello\n")
    client_process.stdin.flush()
    client_output = client_process.stdout.readline().decode().strip()

    # stop server and client
    client_process.terminate()

    # open the reference Python client
    client_process = open_client(subprocess_client_ref)

    # write "Hello" to stdin of the client
    client_process.stdin.write(b"Hello\n")
    client_process.stdin.flush()
    ref_output = client_process.stdout.readline().decode().strip()

    # stop server and client
    client_process.terminate()
    server_process.terminate()

    # print the score
    print(f"Client Output:     {client_output}")
    print(f"Reference Output:  {ref_output}")
    print(f"Final Score:       {20 if client_output == ref_output else 0}")


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(e)
        print("Final Score: 0")
