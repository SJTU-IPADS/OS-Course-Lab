"""Ending a watch session ends marp and every demo, however it is ended.

The end-to-end tests start a real `view --watch` with a stand-in for marp.
Every process they care about is a fixtures/shutdown/child.py, which holds a
lock while it lives: a test signals the process it started, and to clean up
only a pid whose lock is still held, so never anybody else's process.
"""

import contextlib
import json
import os
import select
import shlex
import signal
import socket
import subprocess
import sys
import threading
import time
import urllib.request
from pathlib import Path
from unittest.mock import MagicMock, patch

import pytest

from lecturekit import cli, demo, dev_server

# Sessions, signals and terminals: POSIX has them, and Windows not these modules.
fcntl = pytest.importorskip("fcntl")
pty = pytest.importorskip("pty")

ROOT = Path(__file__).resolve().parents[1]
CHILD = Path(__file__).parent / "fixtures" / "shutdown" / "child.py"
PATIENCE_S = 20.0

# `view` as from a shell: the signals at their defaults whatever runs the tests
# (unless HUP is to be ignored, as nohup has it), and no browser opened.
LAUNCH = """
import atexit, fcntl, os, signal, sys, termios, time, webbrowser
import watchfiles.main
work, browse_s, exit_s, nohup, *args = sys.argv[1:]
def exiting():
    open(os.path.join(work, "exiting"), "w").close()
    time.sleep(float(exit_s))
atexit.register(exiting)  # first, so it runs last
signal.signal(signal.SIGINT, signal.default_int_handler)
signal.signal(signal.SIGTERM, signal.SIG_DFL)
signal.signal(signal.SIGHUP, signal.SIG_IGN if nohup == "nohup" else signal.SIG_DFL)
if os.isatty(0):
    fcntl.ioctl(0, termios.TIOCSCTTY, 0)
def browse(url):
    open(os.path.join(work, "browsing"), "w").close()
    time.sleep(float(browse_s))
    return True
webbrowser.open = browse
notify = watchfiles.main.RustNotify
def notify_and_say(*args):
    made = notify(*args)  # the watches are in place once it is made
    open(os.path.join(work, "watching"), "w").close()
    return made
watchfiles.main.RustNotify = notify_and_say
from lecturekit import cli, dev_server
bind = dev_server.QuietHTTPServer.server_bind
def bind_once_marp_runs(server):
    until = time.monotonic() + 10
    while time.monotonic() < until:
        path = os.path.join(work, "marp-worker.pid")
        if os.path.exists(path) and open(path).read().endswith("\\n"):
            break
        time.sleep(0.02)
    bind(server)
dev_server.QuietHTTPServer.server_bind = bind_once_marp_runs
raise SystemExit(cli.main(args))
"""


def alive(record: Path) -> bool:
    try:
        with open(record) as file:
            fcntl.flock(file, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except BlockingIOError:
        return True
    except FileNotFoundError:
        pass
    return False


def until(condition, what: str, timeout_s: float = PATIENCE_S) -> None:
    deadline = time.monotonic() + timeout_s
    while not condition():
        assert time.monotonic() < deadline, what
        time.sleep(0.02)


def started(record: Path) -> bool:
    return record.exists() and record.read_text().endswith("\n") and alive(record)


def child(record: Path, *how: str) -> str:
    return shlex.join([sys.executable, str(CHILD), str(record), *how])


@pytest.fixture
def work(tmp_path):
    yield tmp_path
    for record in tmp_path.glob("*.pid"):
        if alive(record):
            with contextlib.suppress(ValueError, ProcessLookupError):
                pid = int(record.read_text())
                if pid > 1:
                    os.kill(pid, signal.SIGKILL)


class Session:
    """A `view --watch` of a lecture whose demos are child.py in every shape."""

    # The records of each demo, all alive while the session is.
    DEMOS = {
        "plain": ["plain"],
        "stream": ["stream"],
        "stubborn": ["stubborn"],
        "family": ["parent", "grandchild"],
        "group": ["own-group"],
        "terminal": ["pty", "pty-group"],
        "background": ["background", "background-group"],
    }

    def __init__(self, work: Path):
        self.work = work
        self.lecture, self.out = work / "lecture", work / "out"
        self.proc = None
        self.master = None
        self.said = bytearray()
        self._drained = threading.Event()
        self._reader = None
        self._clients = []
        self.lecture.mkdir()

        def run(name, *how):
            return child(work / f"{name}.pid", *how)

        waited = " && ".join(
            f"[ -s {shlex.quote(str(work / name))}.pid ]" for name in self.DEMOS["background"]
        )
        self.commands = {
            "plain": run("plain"),
            "stream": f"echo started; {run('stream')}",
            "stubborn": run("stubborn", "ignore"),
            "family": f"{run('grandchild', 'ignore')} & {run('parent')}; wait",
            "group": f"{run('own-group', 'ignore', 'group')} & wait",
            "terminal": f"{run('pty-group', 'ignore', 'group')} & {run('pty', 'ignore')}",
            # Over by itself, and two of it stay. They let go of all the output,
            # fd 19 included, which demo_prologue.bash makes of stdout.
            "background": f"{run('background', 'ignore')} >/dev/null 2>&1 19>&- & "
            f"{run('background-group', 'ignore', 'group')} >/dev/null 2>&1 19>&- & "
            f"until {waited}; do sleep 0.05; done",
        }
        options = {"stream": ", timeout=0", "terminal": ", interactive=True"}
        demos = "".join(
            f"    p.demo({name!r}, {command!r}{options.get(name, '')})\n"
            for name, command in self.commands.items()
        )
        self.write_lecture(demos)
        self.marp = work / "marp"
        self.write_marp()
        with socket.socket() as probe:
            probe.bind(("127.0.0.1", 0))
            self.port = probe.getsockname()[1]

    def write_lecture(self, demos: str, second_load: str = "") -> None:
        (self.lecture / "lecture.py").write_text(
            "from pathlib import Path\n"
            "from lecturekit.dsl import Lecture\n"
            f"{second_load}"
            "def body(p):\n"
            "    p.title('Shutdown')\n"
            "    p.slide('x')\n"
            f"{demos}"
            "lecture = Lecture(id='shutdown', title='Shutdown')\n"
            "lecture.page('p', body=body)\n",
            encoding="utf-8",
        )

    def write_marp(self, *how: str) -> None:
        """The stand-in for marp. ``ignore``: it will not exit when asked to."""
        self.marp.write_text(
            "#!/bin/sh\n"
            "echo '<html><head></head><body>deck</body></html>' > slides.html\n"
            f"{child(self.work / 'marp-worker.pid', 'ignore')} &\n"
            f"exec {child(self.work / 'marp.pid', *how)}\n"
        )
        self.marp.chmod(0o755)

    def start(self, *, tty: bool = False, browse_s: float = 0, exit_s: float = 0,
              nohup: bool = False):
        argv = [
            sys.executable, "-c", LAUNCH, str(self.work), str(browse_s), str(exit_s),
            "nohup" if nohup else "-", "view", str(self.lecture), "--watch",
            "--out", str(self.out), "--port", str(self.port), "--debounce", "50",
        ]
        env = dict(os.environ, LECTUREKIT_MARP=str(self.marp),
                   PYTHONPATH=str(ROOT), PYTHONDONTWRITEBYTECODE="1")
        if tty:
            self.master, slave = pty.openpty()
            streams = dict(stdin=slave, stdout=slave, stderr=slave)
        else:
            log = open(self.work / "said", "wb")
            streams = dict(stdin=subprocess.DEVNULL, stdout=log, stderr=log)
        try:
            self.proc = subprocess.Popen(
                argv, cwd=ROOT, env=env, start_new_session=True, **streams
            )
        finally:
            if tty:
                os.close(slave)
            else:
                log.close()
        if tty:
            self._reader = threading.Thread(target=self._drain, daemon=True)
            self._reader.start()
        return self

    def _drain(self):
        while not self._drained.is_set():
            ready, _, _ = select.select([self.master], [], [], 0.05)
            if ready:
                try:
                    data = os.read(self.master, 65536)
                except OSError:
                    return
                if not data:
                    return
                self.said += data

    def close_terminal(self):
        if self.master is not None:
            self._drained.set()
            self._reader.join(PATIENCE_S)
            os.close(self.master)
            self.master = None

    def output(self) -> str:
        said = self.work / "said"
        return (said.read_bytes() if said.exists() else bytes(self.said)).decode("utf-8", "replace")

    def ready(self):
        # Watching is what a session does last, and a change saved before
        # that is one nobody sees.
        until((self.work / "watching").exists, "the session did not start")
        until(lambda: started(self.work / "marp.pid"), "marp did not start")
        return self

    def press(self, name: str):
        request = urllib.request.Request(
            f"http://127.0.0.1:{self.port}{dev_server.DEMO_PATH}",
            data=json.dumps({"id": demo.demo_id(self.commands[name])}).encode(),
            headers={"Content-Type": "application/json"},
        )

        def listen():
            with contextlib.suppress(OSError, ValueError):
                with urllib.request.urlopen(request, timeout=PATIENCE_S * 3) as response:
                    for _ in response:
                        pass

        client = threading.Thread(target=listen, daemon=True)
        client.start()
        self._clients.append(client)
        return client

    def press_all(self):
        until(lambda: (self.out / demo.DEMOS_FILENAME).exists(), "no demo table")
        clients = {name: self.press(name) for name in self.commands}
        for names in self.DEMOS.values():
            for name in names:
                until(lambda: started(self.work / f"{name}.pid"), f"{name} did not start")
        clients["background"].join(PATIENCE_S)
        assert not clients["background"].is_alive(), "a command's own end ends its run"
        assert alive(self.work / "background.pid"), "and leaves what it put in the background"
        return self

    def ended(self, status: int) -> str:
        try:
            code = self.proc.wait(PATIENCE_S)
        except subprocess.TimeoutExpired:
            pytest.fail(f"the session did not end:\n{self.output()}")
        for record in self.work.glob("*.pid"):
            until(lambda: not alive(record), f"{record.stem} was left running", 5.0)
        if self._reader is not None:
            self._reader.join(1.0)
        said = self.output()
        assert code == status, said
        with socket.socket() as again:
            again.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            again.bind(("127.0.0.1", self.port))
        assert "Traceback" not in said, said
        assert "marp exited" not in said, said
        return said

    def dispose(self):
        if self.proc is not None and self.proc.poll() is None:
            self.proc.kill()
            self.proc.wait()
        self.close_terminal()
        for client in self._clients:
            client.join(1.0)


@pytest.fixture
def session(work):
    made = Session(work)
    yield made
    made.dispose()


# --- a session with every kind of demo running ---


@pytest.mark.parametrize(
    "signum, status",
    [(signal.SIGINT, 0), (signal.SIGTERM, 143), (signal.SIGHUP, 129)],
    ids=["int", "term", "hup"],
)
def test_a_signal_ends_marp_and_every_demo(session, signum, status):
    session.start().ready().press_all()
    session.proc.send_signal(signum)
    said = session.ended(status)
    assert said.count("shutting down") == (0 if signum == signal.SIGHUP else 1)


@pytest.mark.parametrize(
    "first, then, status",
    [
        (signal.SIGINT, [signal.SIGINT, signal.SIGINT], 0),
        (signal.SIGTERM, [signal.SIGINT], 143),
    ],
    ids=["int-repeated", "term-then-int"],
)
def test_a_second_signal_does_not_cut_the_cleanup_short(session, first, then, status):
    session.write_marp("ignore")  # the cleanup waits for it
    session.start().ready().press_all()
    session.proc.send_signal(first)
    until(lambda: "shutting down" in session.output(), "the cleanup did not begin")
    for signum in then:
        session.proc.send_signal(signum)
    assert session.ended(status).count("shutting down") == 1


def test_ctrl_c_typed_twice_at_the_terminal(session):
    session.write_marp("ignore")
    session.start(tty=True).ready().press_all()
    os.write(session.master, b"\x03")
    until(lambda: "shutting down" in session.output(), "the cleanup did not begin")
    os.write(session.master, b"\x03")
    assert "shutting down" in session.ended(0)


@pytest.mark.parametrize("signum, status", [(signal.SIGINT, 0), (signal.SIGTERM, 143)])
def test_another_signal_while_the_process_exits_changes_nothing(session, signum, status):
    session.start(exit_s=1.0).ready()
    session.proc.send_signal(signum)
    until((session.work / "exiting").exists, "the process did not get to its exit")
    session.proc.send_signal(signum)
    session.ended(status)


def test_closing_the_terminal_ends_it(session):
    session.start(tty=True).ready().press_all()
    session.close_terminal()
    session.ended(129)


def test_nohup_still_keeps_it_running(session):
    session.start(nohup=True).ready()
    session.proc.send_signal(signal.SIGHUP)
    with pytest.raises(subprocess.TimeoutExpired):
        session.proc.wait(1.0)
    session.proc.send_signal(signal.SIGTERM)
    session.ended(143)


def test_a_render_that_fails_for_good_ends_all_the_same(session):
    session.start().ready().press_all()
    # The error preview has nowhere to be written, so the session is over.
    (session.out / "index.html").unlink()
    (session.out / "index.html").mkdir()
    (session.lecture / "lecture.py").write_text("syntax ((( error", encoding="utf-8")
    assert session.proc.wait(PATIENCE_S) == 1
    for record in session.work.glob("*.pid"):
        until(lambda: not alive(record), f"{record.stem} was left running", 5.0)


# --- on the way up ---

SLOW_SECOND_LOAD = """loads = Path(__file__).parent.parent / "loads"
if loads.exists():
    (loads.parent / "rendering").touch()
    {wait}
loads.touch()
"""
WAITS = {
    "waiting": "__import__('time').sleep(30)",
    "swallowing": "try: __import__('time').sleep(30)\n    except BaseException: pass",
}


@pytest.mark.parametrize("wait", WAITS)
@pytest.mark.parametrize("signum, status", [(signal.SIGINT, 0), (signal.SIGTERM, 143)])
def test_interrupted_in_the_first_render(session, wait, signum, status):
    session.write_lecture("", SLOW_SECOND_LOAD.format(wait=WAITS[wait]))
    session.start()
    until((session.work / "rendering").exists, "the render did not start")
    session.proc.send_signal(signum)
    session.ended(status)
    assert not (session.work / "marp.pid").exists(), "marp was started after the interrupt"


def test_interrupted_in_a_later_render_that_swallows_it(session):
    session.start().ready()
    (session.work / "loads").touch()  # every load from here is the slow one
    session.write_lecture("", SLOW_SECOND_LOAD.format(wait=WAITS["swallowing"]))
    until((session.work / "rendering").exists, "the change was not rendered")
    session.proc.send_signal(signal.SIGTERM)
    session.ended(143)


@pytest.mark.parametrize("signum, status", [(signal.SIGINT, 0), (signal.SIGHUP, 129)])
def test_interrupted_while_the_browser_opens(session, signum, status):
    session.start(browse_s=30)
    until((session.work / "browsing").exists, "no browser was asked for")
    until(lambda: started(session.work / "marp-worker.pid"), "marp did not start")
    session.proc.send_signal(signum)
    session.ended(status)


def test_a_port_in_use_leaves_no_marp(session):
    with socket.socket() as holder:
        holder.bind(("127.0.0.1", session.port))
        holder.listen()
        session.start()
        assert session.proc.wait(PATIENCE_S) == 1
    assert "Address already in use" in session.output()
    for record in ("marp.pid", "marp-worker.pid"):
        until(lambda: not alive(session.work / record), f"{record} was left running", 5.0)


def test_ctrl_c_while_the_lecture_loads(tmp_path, capsys):
    with patch.object(cli, "load_lecture_for", side_effect=KeyboardInterrupt):
        try:
            status = cli.main(["view", str(tmp_path), "--watch"])
        except KeyboardInterrupt:
            pytest.fail("the interrupt reached the top as a traceback")
        assert status == 0
        assert "shutting down" in capsys.readouterr().err
        with pytest.raises(KeyboardInterrupt):
            cli.main(["render", str(tmp_path)])


# --- one run ---


@pytest.mark.parametrize("how", ["timeout", "stop"])
@pytest.mark.parametrize("group", [[], ["group"]], ids=["same-group", "own-group"])
def test_stopping_a_run_reaches_what_its_shell_left_behind(work, how, group):
    record = work / "left.pid"
    # bash is over at once; the child still holds the output.
    command = (
        f"{child(record, 'ignore', *group)} & "
        f"until [ -s {shlex.quote(str(record))} ]; do sleep 0.05; done"
    )
    events = demo.stream(command, cwd=work, timeout_s=1.5 if how == "timeout" else None)
    for event in events:
        if isinstance(event, demo.Done):
            assert event.timed_out
        elif how == "stop" and isinstance(event, demo.Tick) and started(record):
            events.close()
            break
    until(lambda: not alive(record), "the shell's child was left running", 5.0)


def test_without_proc_the_session_is_found_with_ps(work):
    from lecturekit import procs

    record = work / "grouped.pid"
    leader = procs.start(["bash", "-c", f"{child(record, 'ignore', 'group')} & wait"])
    try:
        until(lambda: started(record), "the child did not start")
        with patch.object(procs.os, "listdir", side_effect=FileNotFoundError):
            procs.kill(leader)
        until(lambda: not alive(record), "the child in its own group was left running", 5.0)
    finally:
        if leader.poll() is None:
            leader.kill()
        leader.wait()


def test_a_finished_session_is_forgotten_once_it_is_empty(work):
    from lecturekit import procs

    sessions = procs.Sessions()
    quick = sessions.start(["true"])
    quick.wait()
    sessions.release(quick)
    until(lambda: not sessions._leaders, "an empty session was kept", 5.0)
    sessions.close()
    with pytest.raises(procs.Closed):
        sessions.start(["true"])


# --- what may be signalled, with nothing really signalled ---


class Leader:
    def __init__(self, pid, returncode=None):
        self.pid, self.returncode = pid, returncode

    def poll(self):
        return self.returncode

    def wait(self, timeout=None):
        return 0


@pytest.mark.parametrize("pid", [-1, 0, 1, "own session", "a mock"])
def test_only_a_child_session_is_ever_signalled(pid):
    from lecturekit import procs

    leader = {"own session": lambda: Leader(os.getsid(0)), "a mock": MagicMock}.get(
        pid, lambda: Leader(pid)
    )()
    with patch.object(procs.os, "killpg") as killpg, patch.object(procs.os, "kill") as kill:
        procs.kill(leader)
        procs.stop(leader, 0)
    killpg.assert_not_called()
    kill.assert_not_called()


def test_a_session_number_given_out_again_is_left_alone():
    from lecturekit import procs

    # Reaped, and yet a process has its pid: that one is somebody else.
    with patch.object(procs.os, "killpg") as killpg, patch.object(procs.os, "kill"):
        procs.kill(Leader(424242, returncode=0))
    killpg.assert_not_called()


def test_a_marp_reaped_long_ago_is_not_signalled():
    from lecturekit import procs

    # Nothing has its pid now, which says nothing of the session with that id:
    # its leader can be somebody else's, started and gone since.
    with (
        patch.object(procs.os, "kill", side_effect=ProcessLookupError),
        patch.object(procs.os, "killpg") as killpg,
    ):
        dev_server._terminate_process_group(Leader(424242, returncode=1))
    killpg.assert_not_called()


def test_our_own_process_group_is_never_signalled():
    from lecturekit import procs

    with patch.object(procs.os, "killpg") as killpg:
        procs._signal({0, 1, os.getpgrp()}, signal.SIGKILL)
    killpg.assert_not_called()


def test_windows_ends_the_trees_that_still_run():
    from lecturekit import procs

    running, over = Leader(4242), Leader(4343, returncode=0)
    sessions = procs.Sessions()
    with patch.object(procs, "start", side_effect=[running, over]):
        sessions.start(["a"])
        sessions.start(["b"])
    with patch.object(procs.os, "name", "nt"), patch.object(procs, "kill_tree") as kill_tree:
        sessions.close()
    kill_tree.assert_called_once_with(running)


# --- serve as a library ---


@pytest.fixture
def served(tmp_path):
    """`serve` with nothing real behind it, and what was asked to stop."""
    marp = MagicMock(stdout=[])
    marp.poll.return_value = None

    def serve():
        dev_server.serve(tmp_path / "lecture", tmp_path / "out", open_browser=False)

    with (
        patch.object(dev_server, "render_once", return_value=()),
        patch.object(dev_server, "watch_command", return_value=["marp"]),
        patch.object(dev_server, "watch_marp_health"),
        patch.object(dev_server, "QuietHTTPServer"),
        patch.object(dev_server, "watch", return_value=[]),
        patch.object(dev_server.subprocess, "Popen", return_value=marp) as popen,
        patch.object(dev_server, "_terminate_process_group") as stop,
        patch.object(dev_server, "atexit"),
    ):
        yield serve, popen, stop


def test_a_signal_while_marp_is_being_started_loses_no_marp(served):
    serve, popen, stop = served
    started_marp = popen.return_value

    def interrupted(*args, **kwargs):
        signal.raise_signal(signal.SIGINT)
        return started_marp

    popen.side_effect = interrupted
    previous = signal.signal(signal.SIGINT, signal.default_int_handler)
    try:
        serve()
    except KeyboardInterrupt:
        pytest.fail("the signal lost the marp that had just started")
    finally:
        signal.signal(signal.SIGINT, previous)
    stop.assert_called_once_with(started_marp)


def test_serve_works_off_the_main_thread(served):
    serve, _, stop = served
    failed = []

    def run():
        try:
            serve()
        except BaseException as error:
            failed.append(error)

    thread = threading.Thread(target=run)
    thread.start()
    thread.join(PATIENCE_S)
    assert not failed and not thread.is_alive()
    stop.assert_called_once()


def test_serve_gives_the_signal_handlers_back(served):
    serve, _, _ = served
    signals = (signal.SIGINT, signal.SIGTERM, signal.SIGHUP)
    before = [signal.getsignal(signum) for signum in signals]
    serve()
    assert [signal.getsignal(signum) for signum in signals] == before
