"""The file channel: `p.demo(..., files=[...])` blocks that show their source.

The properties worth protecting are the demo channel's, one level quieter. A
request names a file by id and the server resolves it against what the author
wrote, so no path travels from the browser to the file system; the table is a
file rewritten on every render, so a path dropped from a block stops resolving;
and the content is read per request, so what the room sees is the file as it is
on disk rather than as it was when the deck was built.
"""

import json
import tempfile
import threading
import unittest
import urllib.error
import urllib.request
from contextlib import closing
from http.server import ThreadingHTTPServer
from pathlib import Path

from lecturekit import dev_server, model, source
from lecturekit.dsl import Lecture
from lecturekit.renderers.viewer import StaticViewerRenderer
from lecturekit.renderers.viewer.blocks import render_block


def _demo_block(lecture: model.Lecture):
    """The one demo block in a lecture built by `_lecture`."""
    return next(
        b for b in lecture.children[0].blocks if b.kind == "demo"
    )


def _lecture(*files: str, disabled: bool = False) -> model.Lecture:
    lecture = Lecture(id="lec", title="L")

    def body(p):
        p.title("P")
        p.slide("body")
        handle = p.demo("build it", "make", files=list(files))
        if disabled:
            handle.disable()

    lecture.page("p1", body=body)
    return lecture.build()


class SourceAuthoringTest(unittest.TestCase):
    def test_files_land_on_the_block(self):
        block = _demo_block(_lecture("examples/a.c"))
        self.assertEqual(block.content["files"], ["examples/a.c"])

    def test_a_block_without_files_carries_an_empty_list(self):
        lecture = Lecture(id="lec", title="L")

        def body(p):
            p.title("P")
            p.demo("run", "echo hi")

        lecture.page("p1", body=body)
        block = _demo_block(lecture.build())
        self.assertEqual(block.content["files"], [])

    def test_a_single_string_is_refused(self):
        with self.assertRaises(model.ValidationError):
            source.normalize("examples/a.c")

    def test_a_path_that_leaves_the_lecture_is_refused(self):
        for path in ("../secrets.txt", "/etc/passwd", "examples/../../x"):
            with self.subTest(path=path):
                with self.assertRaises(model.ValidationError):
                    source.normalize([path])

    def test_a_repeated_path_is_named_once(self):
        self.assertEqual(source.normalize(["a.c", "a.c"]), ["a.c"])


class SourceIdTest(unittest.TestCase):
    def test_id_is_stable_and_path_specific(self):
        self.assertEqual(source.source_id("a.c"), source.source_id("a.c"))
        self.assertNotEqual(source.source_id("a.c"), source.source_id("b.c"))

    def test_collect_maps_id_to_the_authored_path(self):
        table = source.collect(_lecture("examples/a.c", "examples/b.c"))
        self.assertEqual(
            {key: entry.path for key, entry in table.items()},
            {
                source.source_id("examples/a.c"): "examples/a.c",
                source.source_id("examples/b.c"): "examples/b.c",
            },
        )

    def test_a_disabled_block_shows_nothing(self):
        self.assertEqual(source.collect(_lecture("a.c", disabled=True)), {})


class SourceTableFileTest(unittest.TestCase):
    def test_round_trips_through_the_bundle(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            source.write(_lecture("examples/a.c"), out)
            self.assertEqual(
                source.read(out),
                {source.source_id("examples/a.c"): source.Source("examples/a.c")},
            )

    def test_a_dropped_file_stops_resolving_after_a_re_render(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            source.write(_lecture("examples/a.c"), out)
            source.write(_lecture(), out)
            self.assertEqual(source.read(out), {})

    def test_a_render_writes_the_table_beside_the_deck(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            StaticViewerRenderer().render(_lecture("examples/a.c"), out)
            data = json.loads(
                Path(out, source.SOURCES_FILENAME).read_text(encoding="utf-8")
            )
        self.assertEqual(
            data, {source.source_id("examples/a.c"): {"path": "examples/a.c"}}
        )

    def test_a_broken_table_resolves_nothing(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            Path(out, source.SOURCES_FILENAME).write_text("[]", encoding="utf-8")
            self.assertEqual(source.read(out), {})


class SourceReadTest(unittest.TestCase):
    def test_resolve_finds_the_file_under_the_lecture(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            Path(root, "examples").mkdir()
            Path(root, "examples", "a.c").write_text("int main;\n", encoding="utf-8")
            path = source.resolve(source.Source("examples/a.c"), root)
        self.assertEqual(path.name, "a.c")

    def test_a_symlink_that_leaves_the_tree_resolves_to_nothing(self):
        # The path was checked when the deck was built, but a lecture directory
        # is a working tree: the link can appear between the two.
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as away:
            root = Path(tmp)
            Path(away, "secret.txt").write_text("s", encoding="utf-8")
            Path(root, "link.txt").symlink_to(Path(away, "secret.txt"))
            self.assertIsNone(source.resolve(source.Source("link.txt"), root))

    def test_a_directory_is_not_a_file(self):
        with tempfile.TemporaryDirectory() as tmp:
            Path(tmp, "examples").mkdir()
            self.assertIsNone(source.resolve(source.Source("examples"), Path(tmp)))

    def test_a_long_file_is_truncated_with_a_marker(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp, "big.txt")
            path.write_bytes(b"x" * (source.MAX_SOURCE_BYTES + 10))
            text = source.load(path)
        self.assertIn("not shown", text)
        self.assertLess(len(text), source.MAX_SOURCE_BYTES + 200)

    def test_a_stray_byte_does_not_stop_the_file_being_read(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp, "a.c")
            path.write_bytes(b"int a;\n\xff\n")
            self.assertIn("int a;", source.load(path))


class SourceButtonTest(unittest.TestCase):
    def _html(self, *files: str) -> str:
        block = _demo_block(_lecture(*files))
        return "\n".join(render_block(block, None))

    def test_a_button_carries_the_id_and_the_file_name(self):
        html = self._html("examples/sizes.c")
        self.assertIn(f'data-lk-source="{source.source_id("examples/sizes.c")}"', html)
        self.assertIn(">sizes.c<", html)
        self.assertNotIn("examples/sizes.c<", html)  # the path is not the label

    def test_a_button_ships_disabled(self):
        # Nothing to show without a server behind the page, the way the run
        # button is nothing to press.
        self.assertIn('class="lk-demo-file" type="button" disabled', self._html("a.c"))

    def test_a_block_without_files_grows_no_buttons(self):
        self.assertNotIn("lk-demo-file", self._html())


class SourceEndpointTest(unittest.TestCase):
    def _server(self, directory: Path, demo_cwd=None):
        handler = dev_server.make_handler(
            directory, dev_server.ReloadBroadcaster(), demo_cwd=demo_cwd
        )
        server = ThreadingHTTPServer(("127.0.0.1", 0), handler)
        threading.Thread(target=server.serve_forever, daemon=True).start()
        self.addCleanup(server.server_close)
        self.addCleanup(server.shutdown)
        return server.server_address[1]

    def _get(self, port, query, headers=None):
        request = urllib.request.Request(
            f"http://127.0.0.1:{port}{dev_server.SOURCE_PATH}?{query}",
            headers=headers or {},
        )
        with closing(urllib.request.urlopen(request, timeout=20)) as resp:
            return resp.status, json.loads(resp.read().decode("utf-8"))

    def _deck(self, tmp, *files: str) -> Path:
        out = Path(tmp)
        source.write(_lecture(*files), out)
        return out

    def test_a_known_id_answers_with_the_file(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = self._deck(tmp, "examples/a.c")
            Path(out, "examples").mkdir()
            Path(out, "examples", "a.c").write_text("int main;\n", encoding="utf-8")
            port = self._server(out, demo_cwd=out)
            status, payload = self._get(
                port, "id=" + source.source_id("examples/a.c")
            )
        self.assertEqual(status, 200)
        self.assertEqual(payload["path"], "examples/a.c")
        self.assertEqual(payload["text"], "int main;\n")

    def test_the_file_is_read_per_request(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = self._deck(tmp, "a.c")
            path = Path(out, "a.c")
            path.write_text("before\n", encoding="utf-8")
            port = self._server(out, demo_cwd=out)
            self._get(port, "id=" + source.source_id("a.c"))
            path.write_text("after\n", encoding="utf-8")
            _, payload = self._get(port, "id=" + source.source_id("a.c"))
        self.assertEqual(payload["text"], "after\n")

    def test_an_unknown_id_reads_nothing(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = self._deck(tmp, "a.c")
            port = self._server(out, demo_cwd=out)
            with self.assertRaises(urllib.error.HTTPError) as caught:
                self._get(port, "id=deadbeefcafe")
        self.assertEqual(caught.exception.code, 404)

    def test_a_caller_cannot_ask_for_a_path_of_its_own(self):
        # The mapping is one-way by construction: the query has nowhere to put a
        # path, and an id that is not in the table resolves to nothing.
        with tempfile.TemporaryDirectory() as tmp:
            out = self._deck(tmp, "a.c")
            Path(out, "a.c").write_text("mine\n", encoding="utf-8")
            port = self._server(out, demo_cwd=out)
            with self.assertRaises(urllib.error.HTTPError):
                self._get(port, "id=x&path=/etc/passwd")

    def test_a_missing_file_is_reported_rather_than_guessed_at(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = self._deck(tmp, "gone.c")
            port = self._server(out, demo_cwd=out)
            with self.assertRaises(urllib.error.HTTPError) as caught:
                self._get(port, "id=" + source.source_id("gone.c"))
        self.assertEqual(caught.exception.code, 404)

    def test_a_foreign_origin_is_refused(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = self._deck(tmp, "a.c")
            Path(out, "a.c").write_text("x\n", encoding="utf-8")
            port = self._server(out, demo_cwd=out)
            with self.assertRaises(urllib.error.HTTPError) as caught:
                self._get(
                    port,
                    "id=" + source.source_id("a.c"),
                    headers={"Origin": "http://evil.example"},
                )
        self.assertEqual(caught.exception.code, 403)

    def test_the_endpoint_is_absent_unless_demos_are_armed(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = self._deck(tmp, "a.c")
            Path(out, "a.c").write_text("x\n", encoding="utf-8")
            port = self._server(out, demo_cwd=None)
            with self.assertRaises(urllib.error.HTTPError) as caught:
                self._get(port, "id=" + source.source_id("a.c"))
        self.assertEqual(caught.exception.code, 404)


class SourceInjectionTest(unittest.TestCase):
    def test_the_controller_rides_the_response_only_when_armed(self):
        armed = _slides(Path("."))
        self.assertIn(dev_server.SOURCE_PATH, armed)
        self.assertIn("lk-file", armed)
        self.assertNotIn("lk-file", _slides(None))

    def test_the_file_panel_is_asked_before_the_demo_drawer(self):
        # Both controllers take Escape and an outside click in the capture
        # phase and stop them dead; the one on top has to be listening first.
        armed = _slides(Path("."))
        self.assertLess(armed.index("lk-file"), armed.index("lk-drawer"))

    def test_injection_survives_a_body_with_no_closing_tag(self):
        self.assertIn("lk-file", dev_server.inject_source("<p>no body tag</p>"))


def _slides(demo_cwd):
    """The slides.html the dev server answers with, armed or not."""
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        (root / "slides.html").write_text(
            "<html><body>deck</body></html>", encoding="utf-8"
        )
        handler = dev_server.make_handler(
            root, dev_server.ReloadBroadcaster(), demo_cwd=demo_cwd
        )
        server = ThreadingHTTPServer(("127.0.0.1", 0), handler)
        threading.Thread(target=server.serve_forever, daemon=True).start()
        try:
            port = server.server_address[1]
            with closing(
                urllib.request.urlopen(f"http://127.0.0.1:{port}/slides.html", timeout=5)
            ) as resp:
                return resp.read().decode("utf-8")
        finally:
            server.shutdown()
            server.server_close()


if __name__ == "__main__":
    unittest.main()
