"""Run a lecture's `p.demo(...)` commands from the live preview.

A demo block is a command the author wrote in their own source. The deck cannot
be trusted to *carry* that command back — a page is HTML, and HTML in a browser
is reachable by anything else the browser loaded — so the button carries only an
**identifier** and the server resolves it against the lecture. The identifier is
the command's own content hash, which makes the mapping one-way: a caller can
name a command that exists in the source and nothing else. Two pages that run
the same command share an id, which is correct — it is the same command.

The resolved table is written beside the deck as ``demos.json`` on every render,
so it tracks the source with no second copy to keep in sync: delete the block,
re-render, and the id stops resolving.

Output arrives as it is produced. `ollama run` prints a token at a time and a
lecture wants to watch it happen, so `stream` is the primitive here and `run`
— give me the whole thing when it is over — is written in terms of it.
"""

from __future__ import annotations

import codecs
import hashlib
import json
import os
import re
import select
import signal
import subprocess
import time
from collections.abc import Iterator
from dataclasses import dataclass
from pathlib import Path

from . import model

#: The table of runnable commands, written next to the deck by the viewer
#: renderer and read back by the dev server. Rewritten on every render — always,
#: even when empty, so a deleted demo cannot stay runnable through a stale file.
DEMOS_FILENAME = "demos.json"

#: How long a demo may run before it is killed, when the block does not say.
#: Generous, because a demo that loads a model is still a demo; a command with
#: no natural end (`ollama serve`) says ``timeout=0`` and is stopped by hand.
DEFAULT_TIMEOUT_S = 120.0

#: Output past this is dropped, with a marker. The whole body travels to the
#: browser and sits in one element; a progress bar redrawing itself for four
#: minutes would otherwise get there a byte at a time.
MAX_OUTPUT_BYTES = 512 * 1024

#: How long a quiet command may stay quiet before the stream says something
#: anyway. Two jobs: it keeps the browser's clock honest, and a write is the
#: only way this end learns the listener hung up (see ``stream``).
TICK_S = 1.0

_ID_LENGTH = 12
_READ_BYTES = 8192

# Terminal control sequences, dropped on the way through. A spinner is for a
# terminal that can move its cursor; the drawer is a <pre> and would show the
# escapes themselves. Carriage returns are *kept* — the client redraws the line,
# which is what makes a download's progress bar readable.
_ANSI_RE = re.compile(
    r"\x1b\[[0-9;?]*[ -/]*[@-~]"  # CSI ... final
    r"|\x1b\][^\x07\x1b]*(?:\x07|\x1b\\)"  # OSC ... BEL/ST
    r"|\x1b[@-Z\\-_]"  # two-character escapes
)
_ANSI_CARRY_MAX = 32


def demo_id(command: str) -> str:
    """The stable identifier for ``command`` — a prefix of its SHA-256."""
    return hashlib.sha256(command.encode("utf-8")).hexdigest()[:_ID_LENGTH]


@dataclass(frozen=True)
class Spec:
    """A runnable demo: what to run, and what the block said about how long.

    ``timeout`` is the author's own number and nothing else: ``None`` when the
    block did not mention one — the session's default then applies — and ``0``
    when it said this command has no natural end (``ollama serve``). Resolving
    the two into the seconds a run actually gets is `timeout_for`.
    """

    command: str
    timeout: float | None = None

    def as_json(self) -> dict:
        return {"command": self.command, "timeout": self.timeout}


def _timeout_field(raw) -> float | None:
    """A block's ``timeout=`` as a number, or ``None`` if it did not give one."""
    if raw is None:
        return None
    try:
        return max(0.0, float(raw))
    except (TypeError, ValueError):
        return None


def timeout_for(spec: Spec, default_s: float | None) -> float | None:
    """Seconds this run gets, or ``None`` for no limit.

    The block's answer wins over the session's; zero, from either, is no limit.
    """
    seconds = spec.timeout if spec.timeout is not None else default_s
    return seconds if seconds and seconds > 0 else None


def collect(lecture: model.Lecture) -> dict[str, Spec]:
    """``demo id -> spec`` for every demo block the lecture still carries.

    Disabled blocks are left out: `.disable()` takes a block out of every target
    ahead of any other control, so its command is not part of this deck.
    ``only=``/``except_=`` are *not* consulted — a command that no target draws
    simply has no button to press, and keeping the filter here would mean
    duplicating the renderer's visibility rules in a second place.
    """
    table: dict[str, Spec] = {}
    for page in model.flatten_pages(lecture.children):
        for block in page.blocks:
            if block.kind != "demo" or block.disabled:
                continue
            command = str(block.content["command"])
            table[demo_id(command)] = Spec(
                command=command,
                timeout=_timeout_field(block.content.get("timeout")),
            )
    return table


def write(lecture: model.Lecture, output_dir: Path) -> Path:
    """Write ``demos.json`` into a rendered bundle and return its path."""
    path = Path(output_dir, DEMOS_FILENAME)
    table = {key: spec.as_json() for key, spec in collect(lecture).items()}
    path.write_text(
        json.dumps(table, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    return path


def read(output_dir: Path) -> dict[str, Spec]:
    """Read a bundle's ``demos.json``; an absent or broken file resolves nothing."""
    try:
        data = json.loads(Path(output_dir, DEMOS_FILENAME).read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return {}
    if not isinstance(data, dict):
        return {}
    table: dict[str, Spec] = {}
    for key, value in data.items():
        if not isinstance(value, dict) or not isinstance(value.get("command"), str):
            continue
        table[str(key)] = Spec(
            command=value["command"], timeout=_timeout_field(value.get("timeout"))
        )
    return table


def prompt_lines(command: str) -> list[str]:
    """``command`` as a shell transcript reads it: a ``$`` per command line.

    A line continues the one above it when that one ended with a backslash, or
    left a quote open — a ``curl -d '{...}'`` whose JSON runs over three lines is
    one command, and printing a prompt in the middle of its argument would be
    saying otherwise. Continuation lines keep the author's own indentation.
    """
    lines: list[str] = []
    quote: str | None = None
    prompt = True
    for line in command.splitlines():
        lines.append(f"$ {line}" if prompt else line)
        quote = _scan_quotes(line, quote)
        prompt = quote is None and not line.rstrip().endswith("\\")
    return lines


def _scan_quotes(line: str, quote: str | None) -> str | None:
    """Which quote, if any, ``line`` leaves open — given the one it started in.

    Enough shell to tell a wrapped argument from the next command, and no more:
    quotes, backslash escapes outside single quotes, and a ``#`` that starts a
    comment. It decides where a ``$`` is printed and nothing else, so being
    wrong costs a misplaced prompt rather than a misplaced command.
    """
    escaped = False
    previous = " "
    for char in line:
        if escaped:
            escaped = False
        elif char == "\\" and quote != "'":
            escaped = True
        elif quote is None and char == "#" and previous.isspace():
            break
        elif quote is None and char in "'\"":
            quote = char
        elif char == quote:
            quote = None
        previous = char
    return quote


@dataclass(frozen=True)
class Chunk:
    """Some output, as soon as the command produced it."""

    text: str


@dataclass(frozen=True)
class Tick:
    """Nothing happened for a second. Sent so that something still does."""

    elapsed_s: float


@dataclass(frozen=True)
class Done:
    """The command is over."""

    exit_code: int | None  # None when the run was killed for running too long
    timed_out: bool
    duration_s: float


@dataclass(frozen=True)
class Result:
    """A whole run, once there is nothing left to wait for."""

    command: str
    output: str
    exit_code: int | None
    timed_out: bool
    duration_s: float

    def as_dict(self) -> dict:
        return {
            "command": self.command,
            "output": self.output,
            "exit": self.exit_code,
            "timedOut": self.timed_out,
            "duration": round(self.duration_s, 3),
        }


def _strip_ansi(text: str, carry: str) -> tuple[str, str]:
    """Drop terminal escapes, holding back one that a read cut in half."""
    text = carry + text
    start = text.rfind("\x1b")
    carry = ""
    if start != -1 and not _ANSI_RE.match(text, start):
        # An escape with no final byte yet: it finishes in the next read, and
        # showing its first half now would put a stray `[0m` on the screen.
        if len(text) - start <= _ANSI_CARRY_MAX:
            carry, text = text[start:], text[:start]
    return _ANSI_RE.sub("", text), carry


def stream(
    command: str,
    *,
    cwd: Path,
    timeout_s: float | None = DEFAULT_TIMEOUT_S,
) -> Iterator[Chunk | Tick | Done]:
    """Run ``command`` through a shell in ``cwd``, yielding output as it comes.

    A shell is the point: a demo is a command line as the author would type it
    at the lectern (``gcc -O2 -S demo.c && cat demo.s``), and the string comes
    from their own source file — the same file the dev server already imports and
    executes on every render. stdout and stderr are merged because the audience
    is reading one transcript, not two streams.

    The child gets its own session, so a pipeline is killed whole rather than
    leaving the tail of it running. That happens on **any** way out of this
    generator: a timeout, or the consumer closing it — which is the whole stop
    mechanism, because the consumer is a socket and closing it is what a listener
    that walked away does for itself.

    ``timeout_s=None`` lets the command run until it, or somebody, stops it.
    """
    started = time.monotonic()
    proc = subprocess.Popen(
        command,
        shell=True,
        cwd=str(cwd),
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        stdin=subprocess.DEVNULL,
        text=False,
        start_new_session=True,
    )
    decoder = codecs.getincrementaldecoder("utf-8")(errors="replace")
    deadline = None if timeout_s is None else started + timeout_s
    carry = ""
    sent = 0
    capped = False
    timed_out = False
    try:
        while True:
            ready, _, _ = select.select([proc.stdout], [], [], TICK_S)
            if ready:
                data = proc.stdout.read1(_READ_BYTES)
                if not data:
                    break
                text, carry = _strip_ansi(decoder.decode(data), carry)
                if not text:
                    continue
                if capped:
                    continue  # still draining, so the child is never blocked
                sent += len(data)
                if sent > MAX_OUTPUT_BYTES:
                    capped = True
                    text += f"\n… [output past {MAX_OUTPUT_BYTES} bytes dropped]"
                yield Chunk(text)
            else:
                yield Tick(time.monotonic() - started)
            if deadline is not None and time.monotonic() > deadline:
                timed_out = True
                yield Chunk(f"\n… [killed after {timeout_s:g}s]")
                break
        proc.stdout.close()
        exit_code = None if timed_out else proc.wait()
        yield Done(
            exit_code=exit_code,
            timed_out=timed_out,
            duration_s=time.monotonic() - started,
        )
    finally:
        if proc.poll() is None:
            _kill_session(proc)
            proc.wait()


def run(
    command: str,
    *,
    cwd: Path,
    timeout_s: float | None = DEFAULT_TIMEOUT_S,
) -> Result:
    """``stream`` with the waiting already done: the whole run, as one Result."""
    parts: list[str] = []
    done = Done(exit_code=None, timed_out=True, duration_s=0.0)
    for event in stream(command, cwd=cwd, timeout_s=timeout_s):
        if isinstance(event, Chunk):
            parts.append(event.text)
        elif isinstance(event, Done):
            done = event
    return Result(
        command=command,
        output="".join(parts),
        exit_code=done.exit_code,
        timed_out=done.timed_out,
        duration_s=done.duration_s,
    )


def _kill_session(proc: subprocess.Popen) -> None:
    """Kill the child's whole process group, falling back to the child itself."""
    try:
        os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
    except (ProcessLookupError, PermissionError, OSError):
        try:
            proc.kill()
        except OSError:
            pass
