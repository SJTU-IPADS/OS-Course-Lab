import os
import socket
import sys
import tempfile
import textwrap
import threading
import unittest
import urllib.request
from contextlib import closing
from http.server import ThreadingHTTPServer
from pathlib import Path
from unittest.mock import patch

from lecturekit import dev_server


class PurgeLectureModulesTest(unittest.TestCase):
    def test_removes_only_modules_under_lecture_dir(self):
        from types import ModuleType

        with tempfile.TemporaryDirectory() as tmp:
            lecture_dir = Path(tmp)
            inside = ModuleType("_fake_inside")
            inside.__file__ = str(lecture_dir / "pages.py")
            outside = ModuleType("_fake_outside")
            outside.__file__ = str(Path(tempfile.gettempdir()) / "elsewhere.py")
            builtin_like = ModuleType("_fake_builtin")  # no __file__

            sys.modules["_fake_inside"] = inside
            sys.modules["_fake_outside"] = outside
            sys.modules["_fake_builtin"] = builtin_like
            try:
                dev_server.purge_lecture_modules(lecture_dir)

                self.assertNotIn("_fake_inside", sys.modules)
                self.assertIn("_fake_outside", sys.modules)
                self.assertIn("_fake_builtin", sys.modules)
            finally:
                for name in ("_fake_inside", "_fake_outside", "_fake_builtin"):
                    sys.modules.pop(name, None)


class RenderOnceTest(unittest.TestCase):
    def setUp(self):
        # Other tests (e.g. test_cli loading lec01) leave a "pages" module
        # cached under this generic name; purge before loading our fixtures.
        self._purge_leaked_modules()

    def tearDown(self):
        self._purge_leaked_modules()

    @staticmethod
    def _purge_leaked_modules():
        # The loader caches sibling imports under a generic name ("pages");
        # clear leaked modules so each test loads fresh source.
        for name in list(sys.modules):
            if name == "pages" or name.startswith("_lecturekit_"):
                del sys.modules[name]

    def _write_lecture(self, lecture_dir: Path, page_title: str) -> None:
        (lecture_dir / "pages.py").write_text(
            textwrap.dedent(
                f"""
                PAGE_TITLE = {page_title!r}

                def body(p):
                    p.title(PAGE_TITLE)
                    p.slide("hello")
                """
            ),
            encoding="utf-8",
        )
        (lecture_dir / "lecture.py").write_text(
            textwrap.dedent(
                """
                from lecturekit.dsl import Lecture
                import pages

                lecture = Lecture(id="t", title="T")
                lecture.page("p1", body=pages.body)
                """
            ),
            encoding="utf-8",
        )

    def _write_single_file_lecture(self, lecture_dir: Path, page_title: str) -> None:
        (lecture_dir / "lecture.py").write_text(
            textwrap.dedent(
                f"""
                from lecturekit.dsl import Lecture

                def body(p):
                    p.title({page_title!r})
                    p.slide("hello")

                lecture = Lecture(id="t", title="T")
                lecture.page("p1", body=body)
                """
            ),
            encoding="utf-8",
        )

    def test_writes_viewer_files(self):
        with tempfile.TemporaryDirectory() as tmp:
            lecture_dir = Path(tmp, "src")
            lecture_dir.mkdir()
            out = Path(tmp, "out")
            self._write_lecture(lecture_dir, "First Title")

            dev_server.render_once(lecture_dir, out)

            for name in ("index.html", "slides.md", "lecture.json"):
                self.assertTrue((out / name).exists(), f"missing {name}")
            self.assertIn("First Title", (out / "lecture.json").read_text("utf-8"))

    def test_reflects_sibling_module_edits_on_second_call(self):
        with tempfile.TemporaryDirectory() as tmp:
            lecture_dir = Path(tmp, "src")
            lecture_dir.mkdir()
            out = Path(tmp, "out")
            self._write_lecture(lecture_dir, "First Title")
            dev_server.render_once(lecture_dir, out)

            self._write_lecture(lecture_dir, "Second Title")
            dev_server.render_once(lecture_dir, out)

            data = (out / "lecture.json").read_text("utf-8")
            self.assertIn("Second Title", data)
            self.assertNotIn("First Title", data)

    # CPython validates a cached .pyc against the source's (mtime, size) at
    # one-second resolution, so an edit that keeps the byte count and lands in
    # the same second as the last compile would replay the old bytecode. Pinning
    # both writes to one mtime makes that window deterministic instead of a race
    # against the clock.
    def _freeze_mtimes(self, lecture_dir: Path, stamp: float = 1_000_000_000.0) -> None:
        for source in lecture_dir.glob("*.py"):
            os.utime(source, (stamp, stamp))

    def test_same_size_edit_within_one_second_is_not_masked_by_bytecode_cache(self):
        with tempfile.TemporaryDirectory() as tmp:
            lecture_dir = Path(tmp, "src")
            lecture_dir.mkdir()
            out = Path(tmp, "out")
            self._write_single_file_lecture(lecture_dir, "Alpha Title")
            self._freeze_mtimes(lecture_dir)
            dev_server.render_once(lecture_dir, out)

            self._write_single_file_lecture(lecture_dir, "Bravo Title")
            self._freeze_mtimes(lecture_dir)
            dev_server.render_once(lecture_dir, out)

            data = (out / "lecture.json").read_text("utf-8")
            self.assertIn("Bravo Title", data)
            self.assertNotIn("Alpha Title", data)

    def test_same_size_sibling_edit_within_one_second_is_not_masked_by_cache(self):
        with tempfile.TemporaryDirectory() as tmp:
            lecture_dir = Path(tmp, "src")
            lecture_dir.mkdir()
            out = Path(tmp, "out")
            self._write_lecture(lecture_dir, "Alpha Title")
            self._freeze_mtimes(lecture_dir)
            dev_server.render_once(lecture_dir, out)

            self._write_lecture(lecture_dir, "Bravo Title")
            self._freeze_mtimes(lecture_dir)
            dev_server.render_once(lecture_dir, out)

            data = (out / "lecture.json").read_text("utf-8")
            self.assertIn("Bravo Title", data)
            self.assertNotIn("Alpha Title", data)

    def test_render_once_emits_reveal_wrappers(self):
        with tempfile.TemporaryDirectory() as tmp:
            lecture_dir = Path(tmp, "src")
            lecture_dir.mkdir()
            out = Path(tmp, "out")
            self._write_lecture(lecture_dir, "Reveal Title")

            dev_server.render_once(lecture_dir, out)

            slides_md = (out / "slides.md").read_text(encoding="utf-8")
            self.assertIn('<div class="reveal-block" data-reveal="0">', slides_md)

    def test_render_once_omits_wrappers_when_reveal_off(self):
        with tempfile.TemporaryDirectory() as tmp:
            lecture_dir = Path(tmp, "src")
            lecture_dir.mkdir()
            out = Path(tmp, "out")
            self._write_lecture(lecture_dir, "Plain Title")

            dev_server.render_once(lecture_dir, out, reveal=False)

            slides_md = (out / "slides.md").read_text(encoding="utf-8")
            self.assertNotIn("reveal-block", slides_md)
            self.assertNotIn("data-reveal", slides_md)


class HandleChangesTest(unittest.TestCase):
    def tearDown(self):
        for name in list(sys.modules):
            if name == "pages" or name.startswith("_lecturekit_"):
                del sys.modules[name]

    def _lecture(self, lecture_dir: Path, title: str) -> None:
        (lecture_dir / "lecture.py").write_text(
            textwrap.dedent(
                f"""
                from lecturekit.dsl import Lecture
                lecture = Lecture(id="t", title={title!r})
                lecture.page("p1", body=lambda p: (p.title("P"), p.slide("x")))
                """
            ),
            encoding="utf-8",
        )

    def _setup(self):
        tmp = tempfile.mkdtemp()
        self.addCleanup(lambda: __import__("shutil").rmtree(tmp, ignore_errors=True))
        lecture_dir = Path(tmp, "src")
        lecture_dir.mkdir()
        out = Path(tmp, "out")
        self._lecture(lecture_dir, "First")
        dev_server.render_once(lecture_dir, out)
        (out / "slides.html").write_text("<html></html>", encoding="utf-8")
        bc = dev_server.ReloadBroadcaster()
        return lecture_dir, out, bc

    def test_source_change_triggers_rerender(self):
        lecture_dir, out, bc = self._setup()
        self._lecture(lecture_dir, "Second")

        changes = {(dev_server.Change.modified, str(lecture_dir / "lecture.py"))}
        dev_server.handle_changes(changes, lecture_dir, out, bc)

        self.assertIn("Second", (out / "lecture.json").read_text("utf-8"))

    def test_slides_html_change_broadcasts_reload(self):
        lecture_dir, out, bc = self._setup()
        sub = bc.subscribe()

        changes = {(dev_server.Change.modified, str(out / "slides.html"))}
        dev_server.handle_changes(changes, lecture_dir, out, bc)

        self.assertEqual(sub.get_nowait(), dev_server.RELOAD_MESSAGE)

    def test_broken_source_does_not_raise_or_broadcast(self):
        lecture_dir, out, bc = self._setup()
        sub = bc.subscribe()
        (lecture_dir / "lecture.py").write_text("syntax ((( error", encoding="utf-8")

        import io
        from contextlib import redirect_stderr

        err = io.StringIO()
        with redirect_stderr(err):  # the failure is logged, not raised
            changes = {(dev_server.Change.modified, str(lecture_dir / "lecture.py"))}
            dev_server.handle_changes(changes, lecture_dir, out, bc)

        self.assertTrue(sub.empty())
        self.assertIn("render failed", err.getvalue())
        self.assertIn("Lecture source error", (out / "lecture.json").read_text("utf-8"))
        self.assertIn("syntax", (out / "slides.md").read_text("utf-8"))


class RenderErrorPreviewTest(unittest.TestCase):
    def test_writes_error_viewer_bundle(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp, "out")
            err = SyntaxError("unterminated string literal")

            dev_server.render_error_preview(out, err)

            for name in ("index.html", "viewer.css", "viewer.js", "lecture.json", "slides.md"):
                self.assertTrue((out / name).exists(), f"missing {name}")
            self.assertIn("Lecture source error", (out / "lecture.json").read_text("utf-8"))
            self.assertIn("unterminated string literal", (out / "slides.md").read_text("utf-8"))

    def test_initial_render_failure_still_starts_live_server(self):
        import io

        class FakeMarp:
            stdout = []
            pid = -1

            def poll(self):
                # Report "already exited" so _terminate_process_group
                # short-circuits without touching os.killpg.
                return 0

            def wait(self, timeout=None):
                return 0

        class FakeServer:
            def __init__(self, *args, **kwargs):
                pass

            def serve_forever(self):
                pass

            def shutdown(self):
                pass

            def server_close(self):
                pass

        with tempfile.TemporaryDirectory() as tmp:
            lecture_dir = Path(tmp, "src")
            lecture_dir.mkdir()
            (lecture_dir / "lecture.py").write_text("syntax ((( error", encoding="utf-8")
            out = Path(tmp, "out")

            with (
                patch("lecturekit.dev_server.render_once", side_effect=SyntaxError("bad dsl")),
                patch("lecturekit.dev_server.subprocess.Popen", return_value=FakeMarp()),
                patch("lecturekit.dev_server.QuietHTTPServer", FakeServer),
                patch("lecturekit.dev_server.webbrowser.open"),
                patch("lecturekit.dev_server.watch", side_effect=KeyboardInterrupt),
                patch("sys.stdout", io.StringIO()),
                patch("sys.stderr", io.StringIO()),
            ):
                dev_server.serve(lecture_dir, out, port=0)

            self.assertTrue((out / "index.html").exists())
            self.assertIn("bad dsl", (out / "slides.md").read_text("utf-8"))


class MarpHealthTest(unittest.TestCase):
    """marp is the only writer of slides.html; if it never runs, say so."""

    def test_warns_when_no_deck_arrives(self):
        import io
        import time

        class StuckMarp:
            def poll(self):
                return None

        out = io.StringIO()
        started = time.time()
        thread = threading.Thread(
            target=dev_server.watch_marp_health,
            args=(StuckMarp(), Path("/nonexistent/slides.html"), started),
            kwargs={"grace_s": 0.05, "poll_s": 0.01, "out": out},
            daemon=True,
        )
        thread.start()
        deadline = time.time() + 5
        while "no deck from marp" not in out.getvalue() and time.time() < deadline:
            time.sleep(0.01)

        self.assertIn("no deck from marp", out.getvalue())
        self.assertIn("previous build", out.getvalue())

    def test_reports_a_marp_that_exited(self):
        import io
        import time

        class DeadMarp:
            def poll(self):
                return 1

        out = io.StringIO()
        dev_server.watch_marp_health(
            DeadMarp(), Path("/nonexistent/slides.html"), time.time(), out=out
        )

        self.assertIn("marp exited (1)", out.getvalue())

    def test_returns_quietly_once_the_deck_is_fresh(self):
        import io
        import time

        class LiveMarp:
            def poll(self):
                return None

        with tempfile.TemporaryDirectory() as tmp:
            slides = Path(tmp, "slides.html")
            started = time.time()
            slides.write_text("<html></html>", encoding="utf-8")
            out = io.StringIO()
            dev_server.watch_marp_health(
                LiveMarp(), slides, started, grace_s=0.05, poll_s=0.01, out=out
            )

        self.assertEqual(out.getvalue(), "")


class LectureWatchFilterTest(unittest.TestCase):
    def test_accepts_sources_assets_and_slides_html(self):
        with tempfile.TemporaryDirectory() as tmp:
            lecture = Path(tmp, "lecture")
            out = Path(tmp, "out")
            watch_filter = dev_server.LectureWatchFilter(lecture, out)

            self.assertTrue(
                watch_filter(dev_server.Change.modified, str(lecture / "lecture.py"))
            )
            self.assertTrue(
                watch_filter(dev_server.Change.added, str(lecture / "assets" / "a.svg"))
            )
            self.assertTrue(
                watch_filter(dev_server.Change.modified, str(out / "slides.html"))
            )

    def test_ignores_generated_vendor_cache_and_external_files(self):
        with tempfile.TemporaryDirectory() as tmp:
            lecture = Path(tmp, "lecture")
            out = lecture / "build" / "viewer"
            watch_filter = dev_server.LectureWatchFilter(lecture, out)

            ignored = (
                lecture / ".venv" / "lib.py",
                lecture / "third_party" / "source.cc",
                lecture / "__pycache__" / "lecture.pyc",
                out / "lecture.json",
                Path(tmp, "external.txt"),
            )
            for path in ignored:
                self.assertFalse(
                    watch_filter(dev_server.Change.modified, str(path)), str(path)
                )

    def test_nested_output_needs_only_one_watch_root(self):
        lecture = Path("/tmp/lecture")
        self.assertEqual(
            dev_server._watch_paths(lecture, lecture / "build" / "viewer"),
            (lecture.resolve(),),
        )


class QuietServerTest(unittest.TestCase):
    def _server(self):
        server = dev_server.QuietHTTPServer(
            ("127.0.0.1", 0), dev_server.make_handler(Path("."), dev_server.ReloadBroadcaster())
        )
        self.addCleanup(server.server_close)
        return server

    def test_disconnect_errors_are_swallowed(self):
        import io
        from contextlib import redirect_stderr

        server = self._server()
        err = io.StringIO()
        with redirect_stderr(err):
            try:
                raise ConnectionResetError("peer reset")
            except ConnectionResetError:
                server.handle_error(None, ("127.0.0.1", 0))

        self.assertEqual(err.getvalue(), "")

    def test_real_errors_still_surface(self):
        import io
        from contextlib import redirect_stderr

        server = self._server()
        err = io.StringIO()
        with redirect_stderr(err):
            try:
                raise ValueError("boom")
            except ValueError:
                server.handle_error(None, ("127.0.0.1", 0))

        self.assertIn("boom", err.getvalue())


class MarpWatchCommandTest(unittest.TestCase):
    def test_includes_watch_and_copies_theme(self):
        from lecturekit.renderers.viewer.marp import watch_command

        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp, "out")
            out.mkdir()
            theme_dir = Path(tmp, "themes")
            theme_dir.mkdir()
            (theme_dir / "basic-office.css").write_text("/* css */", encoding="utf-8")

            command = watch_command(out, theme_dir=theme_dir)

            self.assertIn("--watch", command)
            self.assertIn("slides.md", command)
            self.assertTrue((out / "theme.css").exists())
            self.assertIn("--theme", command)

    def test_omits_theme_when_missing(self):
        from lecturekit.renderers.viewer.marp import watch_command

        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp, "out")
            out.mkdir()
            theme_dir = Path(tmp, "themes")  # no css inside
            theme_dir.mkdir()

            command = watch_command(out, theme_dir=theme_dir)

            self.assertIn("--watch", command)
            self.assertNotIn("--theme", command)


class PumpMarpOutputTest(unittest.TestCase):
    def test_drops_info_keeps_warnings_and_errors(self):
        import io

        lines = [
            "[  INFO ] Watching directory: /tmp/out\n",
            "[  INFO ] slides.md => slides.html\n",
            "[  WARN ] something looks off\n",
            "[ ERROR ] Failed to parse slides.md\n",
            "stray line without a tag\n",
        ]
        out = io.StringIO()

        dev_server.pump_marp_output(iter(lines), out=out)

        result = out.getvalue()
        self.assertNotIn("INFO", result)
        self.assertNotIn("slides.html", result)
        self.assertIn("WARN", result)
        self.assertIn("Failed to parse", result)
        self.assertIn("stray line", result)


class ReloadBroadcasterTest(unittest.TestCase):
    def test_broadcast_reaches_all_subscribers(self):
        bc = dev_server.ReloadBroadcaster()
        a = bc.subscribe()
        b = bc.subscribe()

        bc.broadcast()

        self.assertEqual(a.get_nowait(), dev_server.RELOAD_MESSAGE)
        self.assertEqual(b.get_nowait(), dev_server.RELOAD_MESSAGE)

    def test_unsubscribe_stops_delivery(self):
        bc = dev_server.ReloadBroadcaster()
        a = bc.subscribe()
        bc.unsubscribe(a)

        bc.broadcast()

        self.assertTrue(a.empty())


class HandlerTest(unittest.TestCase):
    def _server(self, directory: Path, broadcaster):
        handler = dev_server.make_handler(directory, broadcaster)
        server = ThreadingHTTPServer(("127.0.0.1", 0), handler)
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        self.addCleanup(server.server_close)
        self.addCleanup(server.shutdown)
        return server.server_address[1]

    def test_index_is_served_with_injected_client(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "index.html").write_text(
                "<html><body>hi</body></html>", encoding="utf-8"
            )
            port = self._server(root, dev_server.ReloadBroadcaster())

            for path in ("/", "/index.html"):
                with closing(
                    urllib.request.urlopen(f"http://127.0.0.1:{port}{path}", timeout=5)
                ) as resp:
                    body = resp.read().decode("utf-8")
                self.assertIn("EventSource", body)
                self.assertIn("hi", body)

    def test_other_files_are_served_unmodified(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "slides.md").write_text("# raw markdown", encoding="utf-8")
            port = self._server(root, dev_server.ReloadBroadcaster())

            with closing(
                urllib.request.urlopen(f"http://127.0.0.1:{port}/slides.md", timeout=5)
            ) as resp:
                body = resp.read().decode("utf-8")
            self.assertEqual(body, "# raw markdown")
            self.assertNotIn("EventSource", body)

    def test_static_files_are_never_cached(self):
        # Regression: with stdlib caching, a rebuild within the same wall-clock
        # second as a cached copy returns 304 (Last-Modified has 1s resolution),
        # so the live viewer keeps showing stale content (e.g. a deleted page).
        # Note: slides.html is now served through _serve_slides (reveal bundle
        # injected), so body checks use assertIn rather than assertEqual.
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "slides.html").write_text("v1", encoding="utf-8")
            port = self._server(root, dev_server.ReloadBroadcaster())

            with closing(
                urllib.request.urlopen(f"http://127.0.0.1:{port}/slides.html", timeout=5)
            ) as resp:
                body1 = resp.read().decode("utf-8")
                self.assertIn("v1", body1)
                self.assertEqual(resp.headers.get("Cache-Control"), "no-store")
                last_modified = resp.headers.get("Last-Modified")

            # New content; a conditional GET must still return 200 with v2.
            (root / "slides.html").write_text("v2", encoding="utf-8")
            headers = {"If-Modified-Since": last_modified} if last_modified else {}
            req = urllib.request.Request(
                f"http://127.0.0.1:{port}/slides.html", headers=headers
            )
            with closing(urllib.request.urlopen(req, timeout=5)) as resp:
                self.assertEqual(resp.getcode(), 200)
                self.assertIn("v2", resp.read().decode("utf-8"))

    def test_livereload_endpoint_streams_reload_event(self):
        with tempfile.TemporaryDirectory() as tmp:
            broadcaster = dev_server.ReloadBroadcaster()
            port = self._server(Path(tmp), broadcaster)

            with closing(socket.create_connection(("127.0.0.1", port), timeout=5)) as sock:
                sock.sendall(
                    f"GET {dev_server.LIVERELOAD_PATH} HTTP/1.1\r\n"
                    f"Host: 127.0.0.1\r\n\r\n".encode()
                )
                # let the handler subscribe before we broadcast
                header = sock.recv(4096).decode("utf-8", "replace")
                self.assertIn("text/event-stream", header)

                # broadcast until the event lands (handler subscribe may race)
                sock.settimeout(5)
                deadline = threading.Event()

                def pump():
                    while not deadline.is_set():
                        broadcaster.broadcast()
                        deadline.wait(0.1)

                pumper = threading.Thread(target=pump, daemon=True)
                pumper.start()
                try:
                    data = b""
                    while b"data: reload" not in data:
                        chunk = sock.recv(4096)
                        if not chunk:
                            break
                        data += chunk
                finally:
                    deadline.set()
                    pumper.join(timeout=2)

                self.assertIn("data: reload", data.decode("utf-8", "replace"))


class InjectLiveReloadTest(unittest.TestCase):
    def test_inserts_event_source_client_before_body_close(self):
        html = "<html><body><div id='app'></div></body></html>"

        result = dev_server.inject_livereload(html)

        self.assertIn("EventSource", result)
        self.assertIn(dev_server.LIVERELOAD_PATH, result)
        # client goes before the closing body tag, not after it
        self.assertLess(result.index("EventSource"), result.index("</body>"))

    def test_appends_client_when_no_body_close(self):
        html = "<div id='app'></div>"

        result = dev_server.inject_livereload(html)

        self.assertIn("EventSource", result)
        self.assertTrue(result.startswith(html))


class InjectRevealTest(unittest.TestCase):
    def test_inject_reveal_inserts_bundle_before_body_close(self):
        from lecturekit.dev_server import inject_reveal

        out = inject_reveal("<html><body><p>x</p></body></html>")
        self.assertLess(out.index("reveal-dim"), out.index("</body>"))
        self.assertIn("addEventListener", out)  # the controller script is present
        self.assertTrue(out.endswith("</body></html>"))

    def test_inject_reveal_appends_when_no_body(self):
        from lecturekit.dev_server import inject_reveal

        out = inject_reveal("<section>only</section>")
        self.assertIn("reveal-dim", out)
        self.assertTrue(out.startswith("<section>only</section>"))


class ServeSlideTest(unittest.TestCase):
    def _server(self, directory: Path, broadcaster, reveal: bool = True):
        handler = dev_server.make_handler(directory, broadcaster, reveal=reveal)
        server = ThreadingHTTPServer(("127.0.0.1", 0), handler)
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        self.addCleanup(server.server_close)
        self.addCleanup(server.shutdown)
        return server.server_address[1]

    def test_slides_html_is_served_with_reveal_bundle(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "slides.html").write_text(
                "<html><body><section>slide</section></body></html>",
                encoding="utf-8",
            )
            port = self._server(root, dev_server.ReloadBroadcaster())

            with closing(
                urllib.request.urlopen(f"http://127.0.0.1:{port}/slides.html", timeout=5)
            ) as resp:
                body = resp.read().decode("utf-8")
            self.assertIn("reveal-dim", body)
            self.assertIn("addEventListener", body)
            self.assertIn("slide", body)

    def test_reveal_off_drops_reveal_but_keeps_svg_scope(self):
        # `--no-reveal` turns off the preview feature, not the SVG scoper: the
        # polyfill it tames costs Safari a full-document layout per slide per
        # frame, which is not a preview nicety to opt out of.
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "slides.html").write_text(
                "<html><body><section>slide</section></body></html>",
                encoding="utf-8",
            )
            port = self._server(root, dev_server.ReloadBroadcaster(), reveal=False)

            with closing(
                urllib.request.urlopen(f"http://127.0.0.1:{port}/slides.html", timeout=5)
            ) as resp:
                body = resp.read().decode("utf-8")
            self.assertIn("slide", body)
            self.assertNotIn("reveal-dim", body)
            self.assertIn("data-lk-marpit-svg", body)

    def test_slides_html_is_served_with_svg_scope(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "slides.html").write_text(
                "<html><body><section>slide</section></body></html>",
                encoding="utf-8",
            )
            port = self._server(root, dev_server.ReloadBroadcaster())

            with closing(
                urllib.request.urlopen(f"http://127.0.0.1:{port}/slides.html", timeout=5)
            ) as resp:
                body = resp.read().decode("utf-8")
            self.assertIn("data-lk-marpit-svg", body)
            self.assertIn("bespoke-marp-active", body)

    def test_slides_html_missing_returns_404(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            port = self._server(root, dev_server.ReloadBroadcaster())

            try:
                urllib.request.urlopen(f"http://127.0.0.1:{port}/slides.html", timeout=5)
                self.fail("Expected HTTPError 404")
            except urllib.error.HTTPError as e:
                self.assertEqual(e.code, 404)
