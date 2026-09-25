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

The stream is a terminal transcript, not just output: each authored command is
announced (`Command`) as bash reaches it, so a three-line demo reads as three
prompts, each followed by what it printed. A demo marked ``interactive`` runs
on a pseudo-terminal instead of a pipe, and its `Terminal` is how keystrokes
from the deck reach it.
"""

from __future__ import annotations

import bisect
import codecs
import errno
import fcntl
import functools
import hashlib
import json
import os
import pty
import select
import signal
import struct
import subprocess
import termios
import threading
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

#: What an interactive run tells its programs they are talking to. The drawer
#: is xterm.js, which speaks this dialect.
TERM = "xterm-256color"

# Bounds on a terminal size the page asks for: a phone-sized drawer is still a
# terminal, and a runaway number is not a reason to allocate a huge screen.
_COLS = (2, 500)
_ROWS = (1, 200)

# How long a keystroke waits for a program that is not reading its input before
# it is refused. The terminal's own buffer holds a few KB of typing-ahead.
_WRITE_WAIT_S = 1.0

# Read by bash before every command (as BASH_ENV): it makes bash mark each
# top-level command's line in the output, which `stream` takes out again and
# turns into a `Command`. The file says why each part is there.
_PROLOGUE = Path(__file__).resolve().with_name("demo_prologue.bash")
_MARK = b"\x1b]7717;"
_MARK_MAX = len(_MARK) + 12  # the marker, digits and the BEL that ends it


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

    ``interactive`` runs the command on a terminal the deck can type into
    (``ollama run``); otherwise its stdin is ``/dev/null``.
    """

    command: str
    timeout: float | None = None
    interactive: bool = False

    def as_json(self) -> dict:
        return {
            "command": self.command,
            "timeout": self.timeout,
            "interactive": self.interactive,
        }


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
                interactive=bool(block.content.get("interactive")),
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
            command=value["command"],
            timeout=_timeout_field(value.get("timeout")),
            interactive=value.get("interactive") is True,
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


@functools.lru_cache(maxsize=256)
def command_spans(command: str) -> tuple[tuple[int, int], ...]:
    """The top-level commands of ``command``, as ``(first, last)`` line pairs.

    A loop written over four lines is one command: bash reads all four before
    it runs any, and the transcript shows them together. Where a command ends is
    bash's call, not ours — each growing run of lines is handed to ``bash -n``,
    and a run it can parse to the end is a whole command. Lines are 0-based and
    counted as bash counts them, on ``\\n`` alone.
    """
    lines = command.split("\n")
    if len(lines) == 1:
        return ((0, 0),)
    spans: list[tuple[int, int]] = []
    start = 0
    for end, line in enumerate(lines):
        if _continued(line) or not _parses("\n".join(lines[start : end + 1])):
            continue
        spans.append((start, end))
        start = end + 1
    if start < len(lines):  # unfinished to the end: bash will say so itself
        spans.append((start, len(lines) - 1))
    return tuple(spans)


def _continued(line: str) -> bool:
    """Does ``line`` end in a backslash that joins it to the next one?"""
    return (len(line) - len(line.rstrip("\\"))) % 2 == 1


# What `bash -n` says when the text is fine so far and simply stops early.
_UNFINISHED = ("unexpected end of file", "unexpected EOF", "delimited by end-of-file")


def _parses(text: str) -> bool:
    """Could bash run ``text`` as it stands, without reading another line?

    A syntax error that is not about the text ending early counts as yes: the
    command is as finished as it is going to get, and running it says why.
    """
    try:
        result = subprocess.run(
            ["bash", "-n"],
            input=text.encode("utf-8"),
            capture_output=True,
            env={**os.environ, "LC_ALL": "C"},
            timeout=5,
        )
    except (OSError, subprocess.SubprocessError):
        return True
    said = result.stderr.decode("utf-8", errors="replace")
    return not any(phrase in said for phrase in _UNFINISHED)


class _Transcript:
    """Which authored commands have started, told by bash's line markers.

    A marker names the line bash is about to run; every command up to the one
    holding that line is now on screen. Commands are shown once and in order,
    and a comment or blank line between them rides along with the next one.
    What never ran — the rest of a script that failed under ``set -e`` — is
    never shown. Each is shown as bash echoes a command typed at it: ``$`` on
    its first line, ``>`` on the rest.
    """

    def __init__(self, command: str):
        self._lines = command.split("\n")
        self._spans = command_spans(command)
        self._starts = [first for first, _ in self._spans]
        self._next = 0

    def reached(self, lineno: int) -> list[str]:
        """The commands that line ``lineno`` (bash's, 1-based) brings on."""
        index = bisect.bisect_right(self._starts, lineno - 1) - 1
        index = min(max(index, 0), len(self._spans) - 1)
        texts = []
        for first, last in self._spans[self._next : index + 1]:
            if "".join(self._lines[first : last + 1]).strip():
                texts.append(
                    "\n".join(
                        ("$ " if i == first else "> ") + self._lines[i]
                        for i in range(first, last + 1)
                    )
                )
        self._next = max(self._next, index + 1)
        return texts


class _Markers:
    """Takes bash's line markers out of a byte stream that may split them."""

    def __init__(self):
        self._carry = b""

    def feed(self, data: bytes) -> list[bytes | int]:
        """``data`` as output bytes and line numbers, in the order they came."""
        data, self._carry = self._carry + data, b""
        pieces: list[bytes | int] = []
        while data:
            start = data.find(_MARK)
            if start == -1:
                start = _partial_mark(data)
                if start == -1:
                    pieces.append(data)
                else:
                    pieces.append(data[:start])
                    self._carry = data[start:]
                break
            end = data.find(b"\x07", start + len(_MARK), start + _MARK_MAX)
            if end == -1:
                if len(data) - start < _MARK_MAX:  # finishes in the next read
                    pieces.append(data[:start])
                    self._carry = data[start:]
                    break
                pieces.append(data[: start + 1])  # not ours after all
                data = data[start + 1 :]
                continue
            digits = data[start + len(_MARK) : end]
            if not digits.isdigit():
                pieces.append(data[: start + 1])
                data = data[start + 1 :]
                continue
            pieces.append(data[:start])
            pieces.append(int(digits))
            data = data[end + 1 :]
        return [piece for piece in pieces if piece != b""]


def _partial_mark(data: bytes) -> int:
    """Where a marker's first bytes end ``data``, or -1."""
    start = data.rfind(b"\x1b", max(0, len(data) - len(_MARK)))
    if start != -1 and _MARK.startswith(data[start:]):
        return start
    return -1


class Terminal:
    """The keyboard end of an interactive run: a pseudo-terminal's master side.

    Made by the caller and handed to `stream`, which attaches it once the
    command is running and detaches it when the command is over; in between,
    `write` reaches the program's stdin and `resize` its window size. The lock
    is what makes "over" safe: the master is closed under it, so a keystroke
    that lost the race finds no descriptor rather than somebody else's file.
    """

    def __init__(self, cols: int = 80, rows: int = 24):
        self.cols, self.rows = _size(cols, rows)
        self._fd: int | None = None
        self._lock = threading.Lock()

    def write(self, data: bytes) -> bool:
        """Type ``data``; false once the run is over or input stopped moving."""
        with self._lock:
            if self._fd is None:
                return False
            view = memoryview(data)
            deadline = time.monotonic() + _WRITE_WAIT_S
            while view:
                try:
                    view = view[os.write(self._fd, view) :]
                except BlockingIOError:
                    left = deadline - time.monotonic()
                    if left <= 0:
                        return False
                    select.select([], [self._fd], [], left)
                except OSError:
                    return False
            return True

    def resize(self, cols: int, rows: int) -> None:
        """Take a new window size, and tell the program if it is running."""
        with self._lock:
            self.cols, self.rows = _size(cols, rows)
            if self._fd is not None:
                _set_window(self._fd, self.cols, self.rows)

    def _attach(self, fd: int) -> None:
        with self._lock:
            self._fd = fd

    def _detach(self) -> None:
        with self._lock:
            if self._fd is not None:
                os.close(self._fd)
                self._fd = None


def _size(cols, rows) -> tuple[int, int]:
    """A window size inside the bounds; what is not a number is 80x24."""

    def clamp(value, bounds, fallback):
        if isinstance(value, bool) or not isinstance(value, (int, float)):
            return fallback
        return min(max(int(value), bounds[0]), bounds[1])

    return clamp(cols, _COLS, 80), clamp(rows, _ROWS, 24)


def _set_window(fd: int, cols: int, rows: int) -> None:
    try:
        fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", rows, cols, 0, 0))
    except OSError:
        pass


def _take_terminal() -> None:
    """In the child, after `setsid`: make stdin its controlling terminal.

    Without one, Ctrl-C typed in the drawer is only a byte; with one, the
    terminal turns it into SIGINT for the command, as a real terminal would.
    """
    fcntl.ioctl(0, termios.TIOCSCTTY, 0)


@dataclass(frozen=True)
class Command:
    """The command bash is about to run, as the slide writes it (``$ ...``)."""

    text: str


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


def stream(
    command: str,
    *,
    cwd: Path,
    timeout_s: float | None = DEFAULT_TIMEOUT_S,
    terminal: Terminal | None = None,
) -> Iterator[Command | Chunk | Tick | Done]:
    """Run ``command`` through bash in ``cwd``, yielding the transcript as it comes.

    A shell is the point: a demo is a command line as the author would type it
    at the lectern (``gcc -O2 -S demo.c && cat demo.s``), and the string comes
    from their own source file — the same file the dev server already imports and
    executes on every render. stdout and stderr are merged because the audience
    is reading one transcript, not two streams, and each command is announced
    (`Command`) as bash reaches it, so the output lands under the line that
    made it. Output is passed through as the program wrote it, escapes and all:
    the drawer is a terminal.

    With a ``terminal`` the command runs on a pseudo-terminal — its controlling
    terminal, sized to the drawer — and the terminal carries the keystrokes;
    without one, stdin is ``/dev/null``.

    The child gets its own session, so a pipeline is killed whole rather than
    leaving the tail of it running. That happens on **any** way out of this
    generator: a timeout, or the consumer closing it — which is the whole stop
    mechanism, because the consumer is a socket and closing it is what a listener
    that walked away does for itself.

    ``timeout_s=None`` lets the command run until it, or somebody, stops it.
    """
    started = time.monotonic()
    env = {**os.environ, "BASH_ENV": str(_PROLOGUE)}
    if terminal is None:
        proc = subprocess.Popen(
            ["bash", "-c", command],
            cwd=str(cwd),
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            stdin=subprocess.DEVNULL,
            start_new_session=True,
        )
        fd = proc.stdout.fileno()
    else:
        proc, fd = _spawn_on(terminal, command, cwd, {**env, "TERM": TERM})
    transcript = _Transcript(command)
    markers = _Markers()
    decoder = codecs.getincrementaldecoder("utf-8")(errors="replace")
    deadline = None if timeout_s is None else started + timeout_s
    sent = 0
    capped = False
    timed_out = False
    try:
        while True:
            ready, _, _ = select.select([fd], [], [], TICK_S)
            if ready:
                try:
                    data = os.read(fd, _READ_BYTES)
                except BlockingIOError:
                    continue
                except OSError as error:
                    if error.errno != errno.EIO:  # EIO: the terminal's last
                        raise  # writer is gone, which is its end of file
                    data = b""
                if not data:
                    break
                for piece in markers.feed(data):
                    if capped:
                        continue  # still draining, so the child is never blocked
                    if isinstance(piece, int):
                        for text in transcript.reached(piece):
                            yield Command(text)
                        continue
                    text = decoder.decode(piece)
                    sent += len(piece)
                    if sent > MAX_OUTPUT_BYTES:
                        capped = True
                        text += f"\r\n… [output past {MAX_OUTPUT_BYTES} bytes dropped]"
                    if text:
                        yield Chunk(text)
            else:
                yield Tick(time.monotonic() - started)
            if deadline is not None and time.monotonic() > deadline:
                timed_out = True
                yield Chunk(f"\r\n… [killed after {timeout_s:g}s]")
                break
        rest = decoder.decode(b"", final=True)
        if rest and not capped:
            yield Chunk(rest)
        exit_code = None if timed_out else _shell_status(proc.wait())
        yield Done(
            exit_code=exit_code,
            timed_out=timed_out,
            duration_s=time.monotonic() - started,
        )
    finally:
        if proc.poll() is None:
            _kill_session(proc)
            proc.wait()
        if terminal is None:
            proc.stdout.close()
        else:
            terminal._detach()


def _spawn_on(
    terminal: Terminal, command: str, cwd: Path, env: dict[str, str]
) -> tuple[subprocess.Popen, int]:
    """Start ``command`` on a fresh pseudo-terminal; return it and the master."""
    master, slave = pty.openpty()
    try:
        _set_window(slave, terminal.cols, terminal.rows)
        proc = subprocess.Popen(
            ["bash", "-c", command],
            cwd=str(cwd),
            env=env,
            stdin=slave,
            stdout=slave,
            stderr=slave,
            start_new_session=True,
            preexec_fn=_take_terminal,
        )
    except BaseException:
        os.close(master)
        raise
    finally:
        os.close(slave)
    os.set_blocking(master, False)
    terminal._attach(master)
    return proc, master


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


def _shell_status(code: int) -> int:
    """A status as a shell reports it: death by signal N is 128 + N."""
    return 128 - code if code < 0 else code


def _kill_session(proc: subprocess.Popen) -> None:
    """Kill the child's whole process group, falling back to the child itself."""
    try:
        os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
    except (ProcessLookupError, PermissionError, OSError):
        try:
            proc.kill()
        except OSError:
            pass
