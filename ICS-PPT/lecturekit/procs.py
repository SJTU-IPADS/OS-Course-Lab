"""Children that lead a session of their own, and ending such a session whole.

A child started in a new session is out of the terminal's reach: a Ctrl-C or a
hangup there never gets to it, so whoever started it has to end it. All that it
starts is in its session, though not in one process group: GNU ``timeout`` and
a shell with job control make groups of their own. A process that makes a
session for itself has left, and is not followed.

Windows has no sessions to signal: there a child's tree is killed while the
child runs.
"""

from __future__ import annotations

import os
import signal
import subprocess
import threading

# How often a session that outlived its leader is looked at again.
_POLL_S = 1.0


class Closed(Exception):
    """The `Sessions` were closed, and start nothing more."""


def start(argv: list[str], **options) -> subprocess.Popen:
    """``Popen``, at the head of a new session (Windows ignores that part)."""
    return subprocess.Popen(argv, start_new_session=True, **options)


class Sessions:
    """The sessions one owner started, to be ended with it.

    One stays on the list until nothing is left of it, which can be after its
    leader is over, as with a server that a command left in the background.
    """

    def __init__(self) -> None:
        self._lock = threading.Lock()
        self._leaders: set[subprocess.Popen] = set()
        self._closed = threading.Event()

    def start(self, argv: list[str], **options) -> subprocess.Popen:
        # Under the lock, so that `close` misses no child that is being started.
        with self._lock:
            if self._closed.is_set():
                raise Closed
            proc = start(argv, **options)
            self._leaders.add(proc)
            return proc

    def release(self, proc: subprocess.Popen) -> None:
        """``proc`` is over: forget its session once that is empty."""
        if alive(proc):
            threading.Thread(target=self._forget_when_empty, args=(proc,), daemon=True).start()
        else:
            self._forget(proc)

    def close(self) -> None:
        """End every session on the list, and start no other."""
        with self._lock:
            self._closed.set()
            leaders, self._leaders = self._leaders, set()
        for proc in leaders:
            kill(proc)
        for proc in leaders:
            _reap(proc)

    def _forget_when_empty(self, proc: subprocess.Popen) -> None:
        # Promptly: once a session is empty, its number can be given out again.
        while alive(proc):
            if self._closed.wait(_POLL_S):
                return
        self._forget(proc)

    def _forget(self, proc: subprocess.Popen) -> None:
        with self._lock:
            self._leaders.discard(proc)


def alive(proc: subprocess.Popen) -> bool:
    """Is anything left of the session ``proc`` led?"""
    if os.name != "posix":
        return proc.poll() is None
    sid = _session(proc)
    return sid is not None and bool(_groups(sid))


def kill(proc: subprocess.Popen) -> None:
    """Kill what is left of ``proc``'s session, at once."""
    if os.name != "posix":
        if proc.poll() is None:
            kill_tree(proc)
        return
    sid = _session(proc)
    if sid is None:
        return
    # Again until no new group turns up: one can be made while this goes on.
    groups, killed = {sid} | _groups(sid), set()
    while groups:
        _signal(groups, signal.SIGKILL)
        killed |= groups
        groups = _groups(sid) - killed


def stop(proc: subprocess.Popen, grace_s: float) -> None:
    """Ask ``proc``'s session to end, give its leader ``grace_s``, then kill it.

    The kill follows either way: a wrapper (npx) can be gone while the worker
    it started ignores the request.
    """
    if os.name != "posix":
        kill(proc)
        _reap(proc, grace_s)
        return
    sid = _session(proc)
    if sid is not None:
        _signal({sid} | _groups(sid), signal.SIGTERM)
    _reap(proc, grace_s)
    kill(proc)
    _reap(proc)


def kill_tree(proc: subprocess.Popen) -> None:
    """Kill ``proc`` and every process under it, on Windows (``taskkill /T``)."""
    try:
        subprocess.run(
            ["taskkill", "/T", "/F", "/PID", str(proc.pid)],
            stdin=subprocess.DEVNULL,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
    except OSError:
        try:
            proc.kill()
        except OSError:
            pass


def _reap(proc: subprocess.Popen, timeout_s: float = 1.0) -> None:
    try:
        proc.wait(timeout=timeout_s)
    except (OSError, subprocess.TimeoutExpired):
        pass


def _session(proc: subprocess.Popen) -> int | None:
    """The id of the session ``proc`` leads, while that still names it.

    The id is the leader's pid, which is not given out again as long as the
    session has members. So once the leader is reaped, a process with that pid
    means the session is gone and the number is somebody else's.
    """
    sid = proc.pid
    # 1 would be every process there is, and our own session is never a child's.
    if not isinstance(sid, int) or sid <= 1 or sid == os.getsid(0):
        return None
    if proc.returncode is None:
        return sid
    try:
        os.kill(sid, 0)
    except ProcessLookupError:
        return sid
    except OSError:
        pass
    return None


def _signal(groups: set[int], signum: int) -> None:
    ours = os.getpgrp()
    for group in groups:
        if group > 1 and group != ours:
            try:
                os.killpg(group, signum)
            except OSError:
                pass


def _groups(sid: int) -> set[int]:
    """The process groups in session ``sid``."""
    groups = set()
    for pid in _pids():
        try:
            group = os.getpgid(pid)
            if os.getsid(pid) == sid:
                groups.add(group)
        except OSError:
            continue  # gone meanwhile
    return groups


def _pids() -> list[int]:
    """Every pid there is: from /proc, or where there is none (macOS) from ps."""
    try:
        found = [int(name) for name in os.listdir("/proc") if name.isdigit()]
    except OSError:
        found = []
    if found:
        return found
    try:
        listed = subprocess.run(
            ["ps", "-A", "-o", "pid="],
            capture_output=True, stdin=subprocess.DEVNULL, timeout=5,
            start_new_session=True,  # out of reach of a second Ctrl-C
        ).stdout
    except (OSError, subprocess.SubprocessError):
        return []
    return [int(pid) for pid in listed.split() if pid.isdigit()]
