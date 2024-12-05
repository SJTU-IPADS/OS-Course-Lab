#!/usr/bin/env python3

import argparse
import asyncio
import json
import sys
from asyncio.subprocess import DEVNULL, PIPE, STDOUT
from dataclasses import dataclass

if sys.version_info[0] != 3 or sys.version_info[1] < 6:
    print(
        "This script requires Python version 3.7 and later. Please upgrade your Python version to grade this lab."
    )
    sys.exit(255)

try:
    import psutil
except ImportError:
    print(
        "This script requires PsUtil as main dependencies. Please install this module first."
    )
    sys.exit(255)


@dataclass
class LineCapture:
    """Line capture definition. Used for parsing scores.json in lab folders."""

    content: str
    msg: str
    proposed: int
    actual: int = 0
    userland: bool = False


def killall(pid):
    """Kill all children processes of a process."""
    try:
        parent = psutil.Process(pid)
        for child in parent.children(recursive=True):
            child.kill()
        parent.kill()
    except psutil.NoSuchProcess:
        pass


captures = []
process = None


async def main(args: argparse.Namespace):
    """Main grading function."""

    try:
        with open(args.file, "rb") as f:
            line_captures = json.load(f)
    except FileNotFoundError:
        print(f"File {args.file} not found.")
        raise
    except json.JSONDecodeError:
        print(f"File {args.file} is not a valid JSON file.")
        raise

    for line_capture in line_captures:
        try:
            captures.append(
                LineCapture(
                    content=line_capture["capture"],
                    msg=line_capture["msg"],
                    proposed=line_capture["proposed"],
                    userland=line_capture.get("userland", False),
                )
            )
        except KeyError:
            print("Invalid line capture definition.")
            raise

    process = await asyncio.create_subprocess_exec(
        *args.command, stdin=DEVNULL, stdout=PIPE, stderr=STDOUT
    )

    passed_kernel_serial = False
    passed = 0
    while passed != len(captures):
        try:
            line = await asyncio.wait_for(process.stdout.readline(), args.timeout)
        except asyncio.TimeoutError:
            print("Terminated Due to Timeout.")
            break
        if len(line) == 0 and process.returncode is not None:
            break

        decoded_line = line.decode()

        if args.verbose:
            print(decoded_line, end="")

        for line_capture in captures:
            if (
                args.serial
                and "End of Kernel Checkpoints: {}".format(args.serial) in decoded_line
            ):

                passed_kernel_serial = True

            if line_capture.content in decoded_line:
                if line_capture.userland:
                    if not args.serial or passed_kernel_serial:
                        line_capture.actual = line_capture.proposed
                        passed += 1
                else:
                    if not args.serial or args.serial in decoded_line:
                        line_capture.actual = line_capture.proposed
                        passed += 1
                break
    killall(process.pid)
    await process.wait()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="A simple grader on listening the stdout"
    )
    parser.add_argument(
        "-f",
        "--file",
        type=str,
        dest="file",
        required=True,
        help="Score definition on certain lines of stdout.",
    )
    parser.add_argument(
        "-t",
        "--timeout",
        type=int,
        dest="timeout",
        required=True,
        help="Timeout for the grading process.",
    )
    parser.add_argument(
        "-s",
        "--serial",
        type=str,
        dest="serial",
        required=False,
        help="Serial Number to proceed.",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        required=False,
        default=False,
        help="Verbose mode.",
    )
    parser.add_argument("command", nargs=argparse.REMAINDER, help="Command to grade.")
    args = parser.parse_args()
    try:
        result = asyncio.run(main(args))
    except KeyboardInterrupt:
        if process:
            killall(process.pid)

    scores = 0
    for line_capture in captures:
        print(f"{line_capture.msg}: {line_capture.actual}")
        scores += line_capture.actual

    sys.exit(scores)
