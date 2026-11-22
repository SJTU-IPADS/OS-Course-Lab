"""``filter.cjs`` as a function: markdown in, HTML out.

One engine is one node process. It starts with the first document and ends
when the engine is closed, so a render pays node's start once.
"""

from __future__ import annotations

import json
import shutil
import subprocess
from pathlib import Path

FILTER = Path(__file__).with_name("filter.cjs")
MODULES = Path(__file__).resolve().parents[2] / "node_modules"


class Engine:
    def __init__(self) -> None:
        self._process: subprocess.Popen | None = None

    def _start(self) -> subprocess.Popen:
        node = shutil.which("node")
        if node is None or not (MODULES / "@marp-team" / "marp-core").is_dir():
            missing = "node is not on PATH" if node is None else f"{MODULES} has no marp-core"
            raise RuntimeError(
                f"cannot parse slide text ({missing}): install Node and run scripts/prepare.sh"
            )
        # stderr is inherited, so the filter's warnings reach the user as they come.
        return subprocess.Popen(
            [node, str(FILTER)], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            encoding="utf-8",
        )

    def render(self, markdown: str) -> str:
        if self._process is None:
            self._process = self._start()
        try:
            # JSON keeps the document on one line, and ASCII in it keeps node's
            # readline from splitting at U+2028.
            self._process.stdin.write(json.dumps(markdown) + "\n")
            self._process.stdin.flush()
            reply = self._process.stdout.readline()
        except BrokenPipeError:
            reply = ""
        if not reply:
            raise RuntimeError(
                f"the markdown filter stopped (exit {self._process.wait()}): {FILTER}"
            )
        return json.loads(reply)

    def close(self) -> None:
        process, self._process = self._process, None
        if process is None:
            return
        try:
            process.communicate(timeout=10)  # end of input ends the filter
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
