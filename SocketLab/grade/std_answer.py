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

import os
import time
import subprocess
from common_exceptions import ServerLaunchFailure, ClientLaunchFailure

global execution_path


def subprocess_server_ref() -> subprocess.Popen[bytes]:
    return subprocess.Popen(
        ["./ref/server", "-m", "models/model.gguf"],
        stdout=subprocess.PIPE,
        stdin=subprocess.PIPE,
        cwd=execution_path,
    )


def subprocess_client_ref() -> subprocess.Popen[bytes]:
    return subprocess.Popen(
        ["./ref/client", "-m", "models/model.gguf"],
        stdout=subprocess.PIPE,
        stdin=subprocess.PIPE,
        cwd=execution_path,
    )


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
    # get the path of the current file
    global execution_path
    current_file_path = os.path.dirname(os.path.realpath(__file__))
    execution_path = os.path.join(current_file_path, "..")

    # open server and client
    server_process = open_server(subprocess_server_ref)
    client_process = open_client(subprocess_client_ref)

    # write "Hello" to stdin of the client
    client_process.stdin.write(b"Hello\n")
    client_process.stdin.flush()
    client_output = client_process.stdout.readline().decode().strip()

    # stop server and client
    client_process.terminate()
    server_process.terminate()

    with open("std_answer.txt", "w") as f:
        f.write(client_output)


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(e)
