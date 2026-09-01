from __future__ import annotations

import json
import os
import queue
import re
import select
import signal
import socket
import subprocess
import sys
import threading
import time
import traceback
import webbrowser
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Iterable

from watchfiles import Change, watch

from . import demo as demo_module
from .renderers.viewer.marp import (
    inject_svg_scope,
    marp_command,
    strip_watch_client,
    watch_command,
)

LIVERELOAD_PATH = "/__livereload"
DEMO_PATH = "/__demo"
RELOAD_MESSAGE = "reload"

# A demo request is an id and nothing else, so the body is tiny. Anything larger
# is not one of ours and is refused before it is read.
MAX_DEMO_REQUEST_BYTES = 4096

# How long after a demo run its file writes are still treated as the demo's own
# doing. Long enough to cover the render plus marp's rebuild, which is what a
# reload actually waits on. See `DemoQuiet`.
DEMO_QUIET_S = 3.0

# Coalesce a burst of source writes into one render. watchfiles yields a batch
# once no new change has arrived for `step` ms, capped by a max grouping window
# (`debounce`). A larger default `step` is what lets programmatic/automated edits
# — which write many files with >50ms gaps — land in a single render instead of
# one render per file. It costs some live-reload latency after a hand edit; tune
# it with `view --watch --debounce MS` (e.g. `--debounce 50` for the snappy old
# feel). See `serve`.
DEFAULT_DEBOUNCE_MS = 400

# How long to wait for marp's first `slides.html` before saying it never came.
# A cold start (even via npx, on a big deck) is a few seconds; 20 is generous.
MARP_GRACE_S = 20.0

# marp-cli logs an identical "[ INFO ] slides.md => slides.html" line on every
# rebuild, which floods the dev console with no new signal. We drop those and
# forward only the lines that carry signal (warnings, errors, anything else).
_MARP_INFO_LINE = re.compile(r"\[\s*INFO\s*\]")

LIVERELOAD_CLIENT = (
    "<script>\n"
    f"(function(){{var es=new EventSource({LIVERELOAD_PATH!r});"
    "es.onmessage=function(){location.reload();};})();\n"
    "</script>"
)

_REVEAL_ASSETS = Path(__file__).resolve().parent / "renderers" / "viewer" / "assets"

_IGNORED_WATCH_DIRS = frozenset(
    {
        ".git",
        ".hg",
        ".mypy_cache",
        ".pytest_cache",
        ".ruff_cache",
        ".tox",
        ".venv",
        ".ipynb_checkpoints",
        "__pycache__",
        "build",
        "dist",
        "node_modules",
        "third_party",
        "vendor",
    }
)


def _reveal_bundle() -> str:
    css = (_REVEAL_ASSETS / "reveal.css").read_text(encoding="utf-8")
    js = (_REVEAL_ASSETS / "reveal.js").read_text(encoding="utf-8")
    return f"<style>\n{css}</style>\n<script>\n{js}</script>"


def _demo_bundle() -> str:
    css = (_REVEAL_ASSETS / "demo.css").read_text(encoding="utf-8")
    js = (_REVEAL_ASSETS / "demo.js").read_text(encoding="utf-8")
    return f"<style>\n{css}</style>\n<script>\n{js}</script>"


def inject_demo(html: str) -> str:
    """Return ``html`` with the demo controller inserted before </body>.

    Like the reveal controller, this rides the HTTP response and never the file
    on disk — and unlike it, its presence is load-bearing rather than cosmetic:
    the chips Marp rendered ship ``disabled``, and only this script arms them. A
    bundle rendered by ``render`` therefore cannot run anything, which is the
    intended resting state for a deck that has no server behind it.
    """
    bundle = _demo_bundle()
    marker = "</body>"
    idx = html.rfind(marker)
    if idx == -1:
        return html + bundle
    return html[:idx] + bundle + html[idx:]


def inject_reveal(html: str) -> str:
    """Return ``html`` with the reveal controller inserted before </body>.

    Used only by the live dev server, applied to the HTTP response body — the
    on-disk slides.html (owned by ``marp --watch``) is never modified.
    """
    bundle = _reveal_bundle()
    marker = "</body>"
    idx = html.rfind(marker)
    if idx == -1:
        return html + bundle
    return html[:idx] + bundle + html[idx:]


def purge_lecture_modules(lecture_dir: Path) -> None:
    """Drop cached ``sys.modules`` entries whose source lives under ``lecture_dir``.

    ``lecture.py`` imports sibling modules (e.g. ``pages.py``) that get cached on
    first load; clearing them forces a fresh import so edits are picked up.
    """
    root = lecture_dir.resolve()
    for name in list(sys.modules):
        module = sys.modules.get(name)
        path = getattr(module, "__file__", None)
        if not path:
            continue
        try:
            resolved = Path(path).resolve()
        except (OSError, ValueError):
            continue
        if root == resolved or root in resolved.parents:
            del sys.modules[name]


class ReloadBroadcaster:
    """Fan out reload signals to every connected SSE client."""

    def __init__(self) -> None:
        self._lock = threading.Lock()
        self._subscribers: list[queue.Queue[str]] = []

    def subscribe(self) -> queue.Queue[str]:
        q: queue.Queue[str] = queue.Queue()
        with self._lock:
            self._subscribers.append(q)
        return q

    def unsubscribe(self, q: queue.Queue[str]) -> None:
        with self._lock:
            if q in self._subscribers:
                self._subscribers.remove(q)

    def broadcast(self) -> None:
        with self._lock:
            subscribers = list(self._subscribers)
        for q in subscribers:
            q.put(RELOAD_MESSAGE)


class DemoQuiet:
    """A window during which file changes are the running demo's own doing.

    A demo that compiles something leaves the object file next to the lecture,
    and the watcher cannot tell that write apart from the author saving an
    image — both are non-Python files under the lecture directory, and the
    second one has to reload the deck. So the distinction is made where it is
    actually known: here, by the code that just ran the command.

    Only the *reload* is suppressed; the render still happens, so the bundle on
    disk stays current. The cost is a source edit saved during the window: it
    renders but does not refresh the browser until the next save. That trades a
    rare, self-correcting staleness for the demo not wiping its own output off
    the screen a second after it appears.
    """

    def __init__(self, window_s: float = DEMO_QUIET_S) -> None:
        self.window_s = window_s
        self._until = 0.0
        self._lock = threading.Lock()

    def touch(self) -> None:
        with self._lock:
            self._until = time.monotonic() + self.window_s

    def active(self) -> bool:
        with self._lock:
            return time.monotonic() < self._until


def make_handler(
    directory: Path,
    broadcaster: ReloadBroadcaster,
    reveal: bool = True,
    demo_cwd: Path | None = None,
    demo_timeout_s: float | None = demo_module.DEFAULT_TIMEOUT_S,
    demo_quiet: "DemoQuiet | None" = None,
):
    """Build a request handler that serves ``directory`` with live reload.

    ``index.html`` is served with the SSE client injected; ``/__livereload`` is a
    long-lived event stream; everything else is a plain static file.
    ``slides.html`` always carries the SVG-scoping controller (see
    ``marp.inject_svg_scope``), and carries the reveal-on-Enter controller when
    ``reveal`` is true.

    Both are injected into the response, never the file: under ``--watch`` the
    on-disk ``slides.html`` belongs to ``marp --watch``, which rewrites it on
    every rebuild. A bundle from a plain ``render`` is patched on disk instead,
    by ``build_deck``.

    ``demo_cwd`` turns on ``p.demo(...)`` execution: the deck gets the demo
    controller and ``POST /__demo`` runs a command in that directory. Left at
    ``None`` — the default, and what every command but ``view --demo`` passes —
    the endpoint is not there and the chips stay inert.
    """
    directory = directory.resolve()

    class LiveReloadHandler(SimpleHTTPRequestHandler):
        def __init__(self, *args, **kwargs):
            super().__init__(*args, directory=str(directory), **kwargs)

        def log_message(self, *args):  # keep the dev console quiet
            pass

        def end_headers(self):
            # Nothing in a live-reload bundle should ever be cached. Without this
            # the stdlib serves slides.html/lecture.json/viewer.js with a 1-second
            # Last-Modified, so a rebuild within the same wall-clock second as a
            # cached copy answers conditional GETs with 304 and the viewer keeps
            # stale content (e.g. a page deleted from the source stays visible).
            self.send_header("Cache-Control", "no-store")
            super().end_headers()

        def do_GET(self):
            path = self.path.split("?", 1)[0].split("#", 1)[0]
            if path == LIVERELOAD_PATH:
                self._serve_livereload()
                return
            if path in ("/", "/index.html"):
                self._serve_index()
                return
            if path == "/slides.html":
                self._serve_slides()
                return
            # Drop conditional headers so the stdlib never answers with 304;
            # every request gets the freshly rendered file.
            for cond in ("If-Modified-Since", "If-None-Match"):
                while cond in self.headers:
                    del self.headers[cond]
            super().do_GET()

        def do_POST(self):
            path = self.path.split("?", 1)[0].split("#", 1)[0]
            if path == DEMO_PATH and demo_cwd is not None:
                self._serve_demo()
                return
            self.send_error(404)

        def _send_json(self, status: int, payload: dict):
            body = json.dumps(payload, ensure_ascii=False).encode("utf-8")
            self.send_response(status)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)

        def _demo_request_id(self) -> str | None:
            """The demo id in this request's body, or ``None`` if it is not ours.

            Two cheap gates before anything runs. The JSON content type is the
            load-bearing one: a form on another page can POST here uninvited, but
            it cannot set that header, and a `fetch` that can is held back by the
            preflight this server never answers. The `Origin` check then refuses
            what is left — a non-browser caller that sends a foreign one.
            """
            if "json" not in (self.headers.get("Content-Type") or ""):
                return None
            origin = self.headers.get("Origin")
            if origin and origin != f"http://{self.headers.get('Host', '')}":
                return None
            try:
                length = int(self.headers.get("Content-Length") or 0)
            except ValueError:
                return None
            if length <= 0 or length > MAX_DEMO_REQUEST_BYTES:
                return None
            try:
                payload = json.loads(self.rfile.read(length))
            except (OSError, ValueError):
                return None
            if not isinstance(payload, dict):
                return None
            demo_id = payload.get("id")
            return demo_id if isinstance(demo_id, str) else None

        def _serve_demo(self):
            demo_id = self._demo_request_id()
            if demo_id is None:
                self._send_json(400, {"error": "bad request"})
                return
            # The table is re-read per request rather than cached: every render
            # rewrites it, so this is how a demo the author just deleted stops
            # being runnable without restarting the session.
            spec = demo_module.read(directory).get(demo_id)
            if spec is None:
                self._send_json(404, {"error": "no such demo"})
                return
            print(f"\nlecturekit: demo $ {spec.command}", file=sys.stderr, flush=True)
            self._stream_demo(spec)

        def _stream_demo(self, spec):
            """Answer with one JSON object per line, flushed as the demo speaks.

            Not SSE, though the reload channel next door is: there is no
            reconnecting to do here — a demo that lost its listener is a demo
            nobody is watching — and one line per event is the smaller thing to
            parse. No Content-Length, so the browser reads to the end of the
            connection, the same way `_serve_livereload` does.

            The listener leaving *is* the stop button. Its socket then refuses
            the next write, and closing the generator on the way out kills the
            command's whole process group — so a `Tick` every second is not
            only a clock, it is how a silent command finds out it is alone.
            """
            self.send_response(200)
            self.send_header("Content-Type", "application/x-ndjson; charset=utf-8")
            self.end_headers()
            events = demo_module.stream(
                spec.command,
                cwd=demo_cwd,
                timeout_s=demo_module.timeout_for(spec, demo_timeout_s),
            )
            try:
                for event in events:
                    if self._listener_gone():
                        break
                    # Touched per event, not just at the ends: a demo that
                    # compiles for a minute keeps writing files for a minute,
                    # and every one of those would otherwise reload the deck out
                    # from under its own output.
                    if demo_quiet is not None:
                        demo_quiet.touch()
                    self._send_event(event)
            except (BrokenPipeError, ConnectionResetError, ValueError):
                pass
            finally:
                events.close()
                if demo_quiet is not None:
                    demo_quiet.touch()

        def _listener_gone(self) -> bool:
            """Has the browser dropped this connection?

            Asked of the *read* side, because a write to a socket the peer
            closed succeeds once — the failure only arrives with the reset that
            follows — and a demo the room stopped watching should not get a
            second free second. The client said everything it had to say in its
            request, so anything readable here is the end of the stream.
            """
            try:
                ready, _, _ = select.select([self.connection], [], [], 0)
                if not ready:
                    return False
                return self.connection.recv(1, socket.MSG_PEEK) == b""
            except (OSError, ValueError):
                return True

        def _send_event(self, event):
            if isinstance(event, demo_module.Chunk):
                payload = {"t": "out", "d": event.text}
            elif isinstance(event, demo_module.Tick):
                payload = {"t": "tick", "elapsed": round(event.elapsed_s, 3)}
            else:
                payload = {
                    "t": "end",
                    "exit": event.exit_code,
                    "timedOut": event.timed_out,
                    "duration": round(event.duration_s, 3),
                }
            line = json.dumps(payload, ensure_ascii=False) + "\n"
            self.wfile.write(line.encode("utf-8"))
            self.wfile.flush()

        def _serve_index(self):
            index = directory / "index.html"
            try:
                html = index.read_text(encoding="utf-8")
            except OSError:
                self.send_error(404)
                return
            body = inject_livereload(html).encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)

        def _serve_slides(self):
            slides = directory / "slides.html"
            try:
                html = slides.read_text(encoding="utf-8")
            except OSError:
                self.send_error(404)
                return
            # `marp --watch` owns slides.html on disk and rewrites it on every
            # rebuild, so both controllers are injected into the response body
            # rather than the file. The SVG scoper rides every deck; the reveal
            # controller is the preview feature `--no-reveal` turns off.
            html = inject_svg_scope(html)
            if reveal:
                html = inject_reveal(html)
            if demo_cwd is not None:
                # Take the deck off marp's own reload channel and onto ours,
                # which can tell a demo's build artifacts from an edit. Without
                # the swap a demo that compiles anything refreshes the page a
                # second later and takes its own output with it.
                html = inject_livereload(strip_watch_client(html))
                html = inject_demo(html)
            body = html.encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)

        def _serve_livereload(self):
            self.send_response(200)
            self.send_header("Content-Type", "text/event-stream")
            self.send_header("Connection", "keep-alive")
            self.end_headers()
            q = broadcaster.subscribe()
            try:
                self.wfile.write(b": connected\n\n")
                self.wfile.flush()
                while True:
                    try:
                        q.get(timeout=15)
                        self.wfile.write(f"data: {RELOAD_MESSAGE}\n\n".encode())
                    except queue.Empty:
                        self.wfile.write(b": ping\n\n")  # heartbeat
                    self.wfile.flush()
            except (BrokenPipeError, ConnectionResetError, ValueError):
                pass
            finally:
                broadcaster.unsubscribe(q)

    return LiveReloadHandler


class QuietHTTPServer(ThreadingHTTPServer):
    """A dev server that ignores routine client disconnects.

    Browsers drop SSE and keep-alive connections constantly (every reload, every
    tab close), which the stdlib server would otherwise dump as a traceback. Real
    errors still surface.
    """

    daemon_threads = True

    def handle_error(self, request, client_address):
        exc = sys.exc_info()[1]
        if isinstance(exc, (BrokenPipeError, ConnectionResetError, ConnectionAbortedError)):
            return
        super().handle_error(request, client_address)


def _is_relative_to(path: Path, root: Path) -> bool:
    try:
        path.relative_to(root)
    except ValueError:
        return False
    return True


class LectureWatchFilter:
    """Accept source changes and Marp's final HTML, ignoring heavy vendor trees.

    ``review_dirs`` are the lectures this one borrows review pages from: their
    sources are part of this deck, so editing one has to reload it, exactly as
    editing the lecture's own files does.
    """

    def __init__(
        self,
        lecture_dir: Path,
        output_dir: Path,
        review_dirs: Iterable[Path] = (),
    ) -> None:
        self.lecture_dir = lecture_dir.resolve()
        self.output_dir = output_dir.resolve()
        self.slides_html = self.output_dir / "slides.html"
        self.review_dirs = tuple(path.resolve() for path in review_dirs)

    def __call__(self, change: Change, path: str) -> bool:
        candidate = Path(path).resolve()
        if candidate == self.slides_html:
            return True
        root = self._root_of(candidate)
        if root is None:
            return False
        if _is_relative_to(candidate, self.output_dir):
            return False
        relative = candidate.relative_to(root)
        if any(part in _IGNORED_WATCH_DIRS for part in relative.parts):
            return False
        return candidate.suffix != ".pyc"

    def _root_of(self, candidate: Path) -> Path | None:
        for root in (self.lecture_dir, *self.review_dirs):
            if _is_relative_to(candidate, root):
                return root
        return None


def handle_changes(
    changes: Iterable[tuple[Change, str]],
    lecture_dir: Path,
    output_dir: Path,
    broadcaster: ReloadBroadcaster,
    reveal: bool = True,
    review_dirs: tuple[Path, ...] = (),
    lang: str | None = None,
    strict: bool = False,
    demo_quiet: "DemoQuiet | None" = None,
) -> tuple[Path, ...]:
    """Render source changes and reload after Marp emits ``slides.html``.

    Returns the review sources the render found — unchanged if nothing was
    re-rendered, and the previous set if the render failed (a broken edit must
    not shrink what we are watching).
    """
    slides_html = (output_dir / "slides.html").resolve()
    changed_paths = {Path(path).resolve() for _, path in changes}
    source_changed = any(path != slides_html for path in changed_paths)

    if source_changed:
        try:
            review_dirs = render_once(
                lecture_dir, output_dir, reveal=reveal, lang=lang, strict=strict
            )
        except Exception as exc:
            print("lecturekit: render failed, showing error preview:", file=sys.stderr)
            traceback.print_exc()
            render_error_preview(output_dir, exc, reveal=reveal)

    if slides_html in changed_paths:
        if demo_quiet is not None and demo_quiet.active():
            # A demo's own build artifacts came back around as a rebuild. Not an
            # edit, so not a reload — see `DemoQuiet`.
            return review_dirs
        broadcaster.broadcast()
        # One self-overwriting status line (no newline): the last reload time is
        # the only per-rebuild signal worth showing, and \r keeps it from
        # scrolling the console the way the dropped marp INFO lines used to.
        print(
            f"\rlecturekit: reloaded at {time.strftime('%H:%M:%S')}",
            end="",
            file=sys.stderr,
            flush=True,
        )
    return review_dirs


def _watch_paths(
    lecture_dir: Path, output_dir: Path, review_dirs: Iterable[Path] = ()
) -> tuple[Path, ...]:
    lecture_dir = lecture_dir.resolve()
    output_dir = output_dir.resolve()
    roots = [lecture_dir]
    if not _is_relative_to(output_dir, lecture_dir):
        roots.append(output_dir)
    for path in review_dirs:
        path = path.resolve()
        # A review source nested inside the lecture (or already listed) is
        # covered by a root we are watching anyway.
        if not any(_is_relative_to(path, root) for root in roots):
            roots.append(path)
    return tuple(roots)


def render_once(
    lecture_dir: Path,
    output_dir: Path,
    reveal: bool = True,
    lang: str | None = None,
    strict: bool = False,
) -> tuple[Path, ...]:
    """Re-load the lecture source and render the static viewer into ``output_dir``.

    Returns the directories of the lectures this one borrows review pages from,
    so the caller can watch them too (see ``serve``).

    Sibling modules are purged first so edits to imported files are reflected.
    A watch session always renders the whole lecture: a ``--pages`` selection
    would be re-resolved here on every save, so renaming or deleting the
    selected page wedged the session (see ``cli``, which refuses the pair).
    ``reveal`` enables the reveal-on-Enter block wrappers (paired with the
    controller injection in ``make_handler``); pass ``False`` for a plain live
    preview. ``lang`` applies that translation overlay on every render, so
    editing ``i18n/<lang>.toml`` reloads the deck exactly as editing a page
    does. Marp is not invoked here; the persistent ``marp --watch`` process
    owns ``slides.html``.
    """
    from .cli import load_lecture
    from .renderers.viewer import StaticViewerRenderer

    purge_lecture_modules(lecture_dir)
    lecture = load_lecture(lecture_dir)
    if lang:
        from . import i18n

        lecture = i18n.apply(lecture, lecture_dir, lang, strict=strict)
    StaticViewerRenderer(asset_root=lecture_dir, reveal=reveal).render(lecture, output_dir)
    return tuple(Path(entry.directory) for entry in lecture.borrowed)


def render_error_preview(output_dir: Path, error: BaseException, reveal: bool = True) -> None:
    """Write a valid viewer bundle showing a source-load/render failure."""
    from . import model
    from .renderers.viewer import StaticViewerRenderer

    trace = "".join(traceback.format_exception(type(error), error, error.__traceback__))
    if not trace:
        trace = "".join(traceback.format_exception_only(type(error), error))
    page = model.Page(
        id="lecturekit-source-error",
        title="Lecture source error",
        blocks=[
            model.Block(
                kind="slide",
                content=(
                    "# Lecture source error\n\n"
                    "Fix the saved DSL file and the live preview will recover.\n\n"
                    "```text\n"
                    f"{trace.rstrip()}\n"
                    "```"
                ),
            )
        ],
    )
    lecture = model.Lecture(
        id="lecturekit-error",
        title="Lecture source error",
        children=[page],
    )
    StaticViewerRenderer(reveal=reveal).render(lecture, output_dir)


def pump_marp_output(stream, out=sys.stderr) -> None:
    """Forward marp's output line by line, dropping its repetitive INFO chatter.

    marp ``--watch`` re-emits the same ``[ INFO ]`` build line on every rebuild;
    keeping it would bury the one line that matters — a real build error.
    """
    for line in stream:
        if _MARP_INFO_LINE.search(line):
            continue
        out.write(line)
        out.flush()


def watch_marp_health(
    proc,
    slides_html: Path,
    started_at: float,
    *,
    grace_s: float = MARP_GRACE_S,
    poll_s: float = 0.5,
    out=sys.stderr,
) -> None:
    """Complain if marp never produces a deck; return once it does.

    ``marp --watch`` is the only writer of ``slides.html``, and a reload is only
    broadcast when that file changes — so when marp fails to start, every save
    renders fine and the browser silently keeps showing the previous deck. The
    common cause is the ``npx`` fallback in ``marp_command`` reaching for the npm
    registry on a machine whose network is up but unreachable, where it hangs for
    minutes rather than failing. Left unsaid, that is indistinguishable from a
    slow live reload; said out loud, it points straight at the fix.
    """
    # `started_at` is wall clock (it is compared against a file mtime); the
    # deadline is monotonic, so a clock adjustment cannot stretch or skip it.
    deadline = time.monotonic() + grace_s
    warned = False
    while True:
        status = proc.poll()
        if status is not None:
            print(
                f"\nlecturekit: marp exited ({status}) — the deck will not "
                "rebuild. Fix the cause and restart.",
                file=out,
                flush=True,
            )
            return
        try:
            fresh = slides_html.stat().st_mtime >= started_at
        except OSError:
            fresh = False
        if fresh:
            return
        if not warned and time.monotonic() > deadline:
            warned = True
            hint = (
                "npx is resolving marp against the npm registry; offline that can "
                "hang for minutes. Run scripts/prepare.sh once (with network) to "
                "vendor marp locally."
                if marp_command()[0].endswith("npx")
                else "check the marp output above."
            )
            print(
                f"\nlecturekit: no deck from marp after {grace_s:.0f}s — the browser "
                f"is showing the previous build. {hint}",
                file=out,
                flush=True,
            )
        time.sleep(poll_s)


def inject_livereload(html: str) -> str:
    """Return ``html`` with the SSE live-reload client inserted before </body>."""
    marker = "</body>"
    idx = html.rfind(marker)
    if idx == -1:
        return html + LIVERELOAD_CLIENT
    return html[:idx] + LIVERELOAD_CLIENT + html[idx:]


def _terminate_process_group(proc) -> None:
    """Stop ``proc`` and every process it spawned, by signalling its group.

    ``proc`` (the marp watcher) is launched with ``start_new_session=True``, so
    it leads its own process group and ``os.getpgid(proc.pid) == proc.pid``.
    Signalling the group reaches any wrapper (an ``npx`` fallback spawns the real
    ``node`` marp worker as a grandchild) *and* the worker that binds marp's
    live-reload port, so nothing is orphaned and the port is released. SIGTERM
    first, then SIGKILL if it does not exit promptly.
    """
    if proc.poll() is not None:
        return
    try:
        pgid = os.getpgid(proc.pid)
    except (ProcessLookupError, OSError):
        return
    try:
        os.killpg(pgid, signal.SIGTERM)
    except (ProcessLookupError, OSError):
        return
    try:
        proc.wait(timeout=5)
    except subprocess.TimeoutExpired:
        try:
            os.killpg(pgid, signal.SIGKILL)
        except (ProcessLookupError, OSError):
            pass


def serve(
    lecture_dir: Path,
    output_dir: Path,
    *,
    port: int = 3030,
    open_browser: bool = True,
    reveal: bool = True,
    debounce_ms: int = DEFAULT_DEBOUNCE_MS,
    lang: str | None = None,
    strict: bool = False,
    demo: bool = False,
    demo_timeout_s: float | None = demo_module.DEFAULT_TIMEOUT_S,
) -> None:
    """Run the live-reload dev server until interrupted.

    Wires together the tested pieces: an initial render, a persistent
    ``marp --watch`` subprocess, a static HTTP server with SSE live reload, and a
    native filesystem event loop. The whole lecture is rendered every time; there
    is no page subset to keep in sync (see ``render_once``). ``reveal`` toggles
    the reveal-on-Enter preview (block wrappers + injected controller); pass
    ``False`` for a plain live preview. ``lang``/``strict`` carry a translation
    overlay into every render (an ``i18n/<lang>.toml`` edit is a source edit, so
    it reloads like any other). ``debounce_ms`` is the quiet window: a
    batch of file changes renders once, ``debounce_ms`` after the last change
    (see ``DEFAULT_DEBOUNCE_MS``). ``demo`` arms the lecture's ``p.demo(...)``
    chips so a press runs the command in ``lecture_dir``; it is off by default
    because a deck that can run commands should be something you asked for.
    Ctrl-C tears the subprocess and server down.
    """
    review_dirs: tuple[Path, ...] = ()
    try:
        review_dirs = render_once(
            lecture_dir, output_dir, reveal=reveal, lang=lang, strict=strict
        )
    except Exception as exc:
        print("lecturekit: initial render failed, showing error preview:", file=sys.stderr)
        traceback.print_exc()
        render_error_preview(output_dir, exc, reveal=reveal)

    marp_started_at = time.time()
    marp = subprocess.Popen(
        watch_command(output_dir),
        cwd=output_dir,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        # Own session/process group: marp is spawned via `npx`, so the real
        # `node .../marp` worker (which binds marp's fixed live-reload port) is a
        # grandchild. A plain terminate() would signal only the npx wrapper and
        # orphan that worker, leaking the port; a process group lets us take the
        # whole subtree down at shutdown. See _terminate_process_group.
        start_new_session=True,
    )
    threading.Thread(target=pump_marp_output, args=(marp.stdout,), daemon=True).start()
    threading.Thread(
        target=watch_marp_health,
        args=(marp, output_dir / "slides.html", marp_started_at),
        daemon=True,
    ).start()
    broadcaster = ReloadBroadcaster()
    quiet = DemoQuiet() if demo else None
    server = QuietHTTPServer(
        ("127.0.0.1", port),
        make_handler(
            output_dir,
            broadcaster,
            reveal=reveal,
            demo_cwd=lecture_dir.resolve() if demo else None,
            demo_timeout_s=demo_timeout_s,
            demo_quiet=quiet,
        ),
    )
    server_thread = threading.Thread(target=server.serve_forever, daemon=True)
    server_thread.start()

    url = f"http://127.0.0.1:{port}/"
    print(f"lecturekit: live viewer on {url} (Ctrl-C to stop)")
    if demo:
        print(
            f"lecturekit: demo chips armed — a press runs its command in "
            f"{lecture_dir} (timeout {demo_timeout_s:g}s)"
        )
    if open_browser:
        webbrowser.open(url)

    try:
        # The watched roots are fixed when `watch()` is called, but a review
        # source can be added or dropped by an edit to the lecture itself. When
        # the set moves, break out and start a fresh watch over the new roots.
        while True:
            watched = review_dirs
            paths = _watch_paths(lecture_dir, output_dir, watched)
            for path in watched:
                print(f"lecturekit: also watching review source {path}", file=sys.stderr)
            change_filter = LectureWatchFilter(lecture_dir, output_dir, watched)
            for changes in watch(
                *paths,
                watch_filter=change_filter,
                step=debounce_ms,
                # Cap the grouping window well above `step` so a long, uninterrupted
                # write stream still coalesces (it only yields once quiet), while a
                # never-quiet pathological stream can't stall updates indefinitely.
                debounce=max(1600, debounce_ms * 25),
            ):
                review_dirs = handle_changes(
                    changes, lecture_dir, output_dir, broadcaster,
                    reveal=reveal, review_dirs=watched,
                    lang=lang, strict=strict, demo_quiet=quiet,
                )
                if set(review_dirs) != set(watched):
                    break
            else:
                break
    except KeyboardInterrupt:
        print("\nlecturekit: shutting down")
    finally:
        _terminate_process_group(marp)
        server.shutdown()
        server.server_close()
