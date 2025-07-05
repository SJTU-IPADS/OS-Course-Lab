#!/usr/bin/env python3

# Copyright (c) 2025 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

"""
Expect script for grading lab assignments.
Better to use this script with Python 3.7 and later.
"""
from __future__ import annotations
import argparse
import json
import logging
import sys
from dataclasses import dataclass
from enum import Enum
from typing import TypedDict

if sys.version_info[0] != 3 or sys.version_info[1] < 6:
    print(
        "This script requires Python version 3.7 and later. Please upgrade your Python version to grade this lab."
    )
    sys.exit(255)

try:
    import pexpect
except ImportError:
    print(
        "This script requires pexpect as the main dependency. Please install this module first."
    )
    sys.exit(255)


class Colors(Enum):
    """ANSI color codes"""

    BLACK = "\033[0;30m"
    RED = "\033[0;31m"
    GREEN = "\033[0;32m"
    BROWN = "\033[0;33m"
    BLUE = "\033[0;34m"
    PURPLE = "\033[0;35m"
    CYAN = "\033[0;36m"
    LIGHT_GRAY = "\033[0;37m"
    DARK_GRAY = "\033[1;30m"
    LIGHT_RED = "\033[1;31m"
    LIGHT_GREEN = "\033[1;32m"
    YELLOW = "\033[1;33m"
    LIGHT_BLUE = "\033[1;34m"
    LIGHT_PURPLE = "\033[1;35m"
    LIGHT_CYAN = "\033[1;36m"
    LIGHT_WHITE = "\033[1;37m"
    BOLD = "\033[1m"
    FAINT = "\033[2m"
    ITALIC = "\033[3m"
    UNDERLINE = "\033[4m"
    BLINK = "\033[5m"
    NEGATIVE = "\033[7m"
    CROSSED = "\033[9m"
    END = "\033[0m"
    # cancel SGR codes if we don't write to a terminal
    if not __import__("sys").stdout.isatty():
        for _ in dir():
            if isinstance(_, str) and _[0] != "_":
                locals()[_] = ""
    else:
        # set Windows console in VT mode
        if __import__("platform").system() == "Windows":
            kernel32 = __import__("ctypes").windll.kernel32
            kernel32.SetConsoleMode(kernel32.GetStdHandle(-11), 7)
            del kernel32


@dataclass
class LineExpect:
    """Line capture definition. Used for parsing scores.json in lab folders."""

    content: str
    msg: str
    proposed: int
    actual: int = 0
    userland: bool = False


class RawExpect(TypedDict):
    capture: str
    msg: str
    proposed: int
    actual: int
    userland: bool


def load_captures(file: str, in_kernel: bool) -> list[LineExpect]:

    captures: list[LineExpect] = list()
    try:
        with open(file, "rb") as f:
            raw_captures: list[RawExpect] = json.load(f)

    except FileNotFoundError:
        print(f"File: {file} not found.")
        raise
    except json.JSONDecodeError:
        print(f"File: {file} is not a valid JSON file.")
        raise

    for index, raw_capture in enumerate(raw_captures):
        try:
            captures.append(
                LineExpect(
                    content=raw_capture["capture"],
                    msg=raw_capture["msg"],
                    proposed=raw_capture["proposed"],
                    userland=raw_capture.get("userland", False) if in_kernel else True,
                )
            )
        except KeyError:
            print(f"Invalid line {index} capture definition.")
            raise
    return captures


def main(args: argparse.Namespace):
    """Main grading function."""

    is_kernel_test = args.serial != ""

    captures = load_captures(args.file, is_kernel_test)

    expect_lines = [
        (
            f"{capture.content}: {args.serial}"
            if not capture.userland
            else f"{capture.content}"
        )
        for capture in captures
    ]

    if is_kernel_test:
        expect_lines += [f"End of Kernel Checkpoints: {args.serial}.*"]

    logging.debug(f"Expecting: {expect_lines}")
    process = pexpect.spawn(
        " ".join(args.command), timeout=args.timeout, encoding="utf-8"
    )
    process.logfile = sys.stdout if args.verbose else None
    proposed_total = sum([capture.proposed for capture in captures])
    in_userland = not is_kernel_test
    scores = 0

    if len(expect_lines) > 0:
        while scores < proposed_total:
            try:
                i = process.expect(expect_lines)

                logging.debug(f"Matched: {expect_lines[i]}")
                if i == len(expect_lines) - 1 and is_kernel_test:
                    logging.debug("End of Kernel Checkpoints detected.")
                    in_userland = True
                else:
                    if in_userland == captures[i].userland:
                        captures[i].actual = captures[i].proposed
                        scores += captures[i].actual
                    else:
                        logging.debug(
                            f"Userland Mismatch: {in_userland} != {captures[i].userland}"
                        )
            except (pexpect.EOF, pexpect.TIMEOUT, KeyboardInterrupt):
                break

    process.close()

    for capture in captures:
        print(f"{capture.msg}: {capture.actual}/{capture.proposed}")

    return scores


if __name__ == "__main__":

    parser = argparse.ArgumentParser(
        description="A simple grader script on listening to the stdout."
    )

    _ = parser.add_argument(
        "-f",
        "--file",
        type=str,
        dest="file",
        required=True,
        help="Score definition on certain lines of stdout.",
    )

    _ = parser.add_argument(
        "-t",
        "--timeout",
        type=int,
        dest="timeout",
        required=True,
        help="Timeout for the grading process.",
    )

    _ = parser.add_argument(
        "-s",
        "--serial",
        type=str,
        dest="serial",
        required=False,
        default="",
        help="Serial Number to proceed.",
    )

    _ = parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        required=False,
        default=False,
        help="Verbose mode.",
    )
    _ = parser.add_argument(
        "command", nargs=argparse.REMAINDER, help="Real Command to grade."
    )

    args = parser.parse_args()
    log_level = logging.DEBUG if args.verbose else logging.INFO
    logging.basicConfig(
        format=f"{Colors.GREEN.value}[EXPECT]: %(message)s{Colors.END.value}",
        level=log_level,
    )
    scores = main(args)
    sys.exit(scores)
