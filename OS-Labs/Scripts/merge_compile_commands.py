#!/usr/bin/env python3
import json
import os
import re
import sys

commands = []
for file in sys.stdin:
    file = file.strip()
    with open(file, "r") as f:
        for item in json.load(f):
            if argv := item.get("command"):
                argv = argv.split()
                if "musl-gcc" in argv[0]:
                    item["command"] = " ".join(
                        [
                            "/usr/bin/aarch64-linux-gnu-gcc",
                            "-nostdinc",
                            "-I{}/build/chcore-libc/include".format(
                                os.getenv("LABDIR")
                            ),
                        ]
                        + argv[1:]
                    )

            directory: str = item.get("directory")

            if argv := item.get("arguments"):
                for i, arg in enumerate(argv):
                    if include := re.match(r"^-I([^/]+/(.*))$", arg):
                        argv[i] = "-I" + "/".join([directory, include.group(1)])
                    elif not arg.startswith("-I"):
                        if file := re.match(r"^[^/]+/(.*)$", arg):
                            argv[i] = "/".join([directory, arg])
                item["command"] = " ".join(argv)
                item.pop("arguments")

            file: str = item.get("file")
            if not file.startswith("/"):
                item["file"] = "/".join([directory, file])

            commands.append(item)

with open("compile_commands.json", "w") as f:
    json.dump(commands, f)
