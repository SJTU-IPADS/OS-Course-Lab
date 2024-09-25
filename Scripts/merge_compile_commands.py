#!/usr/bin/env python3
import json
import sys

commands = []
for file in sys.stdin:
    file = file.strip()
    with open(file, "r") as f:
        commands.extend(json.load(f))

with open("compile_commands.json", "w") as f:
    json.dump(commands, f)
