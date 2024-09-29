#!/usr/bin/env python3
import json
import os
import sys

commands = []
for file in sys.stdin:
    file = file.strip()
    with open(file, "r") as f:
        for item in json.load(f):
            if item.get("command"):
                argv = item.get("command").split()
                if "musl-gcc" in argv[0]:
                    item["command"] = (
                        " ".join(
                            [
                                "/usr/bin/aarch64-linux-gnu-gcc",
                                "-nostdinc",
                                "-I{}/build/chcore-libc/include".format(os.getenv("LABDIR")),
                            ]
                            + argv[1:]
                        )
                    )
            commands.append(item)

with open("compile_commands.json", "w") as f:
    json.dump(commands, f)
