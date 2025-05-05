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
import re
import subprocess

score_list = []
test_filenames = [
    "client_test.py",
    "server_test.py",
    "concurrency_test.py",
    "epoll_test.py",
]


def execute_tests(test_filename: str) -> None:
    # get the path of the current file
    current_file_path = os.path.dirname(os.path.realpath(__file__))
    execution_path = os.path.join(current_file_path, "..")
    test_filename = os.path.join(current_file_path, test_filename)

    with subprocess.Popen(
        ["python3", test_filename],
        stdout=subprocess.PIPE,
        cwd=execution_path,
    ) as proc:
        score = re.findall(r"Final Score:\s+(\d+)", proc.stdout.read().decode())
        assert len(score) == 1
        score_list.append(int(score[0]))


def execute_commond_in_current_dir(commands: list[str]) -> None:
    # get the path of the current file
    current_file_path = os.path.dirname(os.path.realpath(__file__))
    execution_path = os.path.join(current_file_path, "..")

    with subprocess.Popen(
        commands,
        stdout=subprocess.PIPE,
        cwd=execution_path,
    ) as proc:
        print(proc.stdout.read().decode())


def main() -> None:
    execute_commond_in_current_dir(["make", "clean"])
    execute_commond_in_current_dir(["make"])
    for test_filename in test_filenames:
        execute_tests(test_filename)

    for score, test_filename in zip(score_list, test_filenames):
        print(f"{test_filename}: {score}")
    print(f"Final Score: {sum(score_list)}")


if __name__ == "__main__":
    main()
