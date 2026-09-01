import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from lecturekit.cli import main
from lecturekit.demo import DEFAULT_TIMEOUT_S as DEFAULT_DEMO_TIMEOUT_S
from lecturekit.dev_server import DEFAULT_DEBOUNCE_MS

LECTURE_SOURCE = "tests/fixtures/sample"


class LectureCliTest(unittest.TestCase):
    def test_inspect_prints_tree(self):
        result = subprocess.run(
            [sys.executable, "-m", "lecturekit.cli", "inspect", LECTURE_SOURCE],
            text=True, capture_output=True, check=True,
        )
        self.assertIn("Sample Lecture", result.stdout)
        self.assertIn("Welcome", result.stdout)

    def test_build_emits_full_ast_json_to_stdout(self):
        result = subprocess.run(
            [sys.executable, "-m", "lecturekit.cli", "build", LECTURE_SOURCE],
            text=True, capture_output=True, check=True,
        )
        data = json.loads(result.stdout)
        self.assertEqual(data["id"], "sample")
        page = data["children"][0]["children"][0]
        # the full AST keeps notes (viewer would skip it)
        self.assertEqual([b["kind"] for b in page["blocks"]], ["slide", "notes"])

    def test_render_to_viewer_writes_bundle_and_builds_marp(self):
        with tempfile.TemporaryDirectory() as tmp:
            with patch("lecturekit.cli.build_deck") as build:
                exit_code = main(["render", LECTURE_SOURCE, "--to", "viewer", "--out", tmp])
            self.assertEqual(exit_code, 0)
            for name in ("index.html", "viewer.css", "viewer.js", "lecture.json", "slides.md"):
                self.assertTrue(Path(tmp, name).exists(), f"missing {name}")
            build.assert_called_once_with(Path(tmp), ("html",), name="sample-lecture")

    def test_render_pdf_flag_requests_html_and_pdf(self):
        with tempfile.TemporaryDirectory() as tmp:
            with patch("lecturekit.cli.build_deck") as build:
                main(["render", LECTURE_SOURCE, "--out", tmp, "--pdf"])
            build.assert_called_once_with(Path(tmp), ("html", "pdf"), name="sample-lecture")

    def test_render_without_pdf_builds_html_only(self):
        with tempfile.TemporaryDirectory() as tmp:
            with patch("lecturekit.cli.build_deck") as build:
                main(["render", LECTURE_SOURCE, "--out", tmp])
            build.assert_called_once_with(Path(tmp), ("html",), name="sample-lecture")

    def test_view_pdf_flag_requests_html_and_pdf(self):
        with tempfile.TemporaryDirectory() as tmp:
            with (
                patch("lecturekit.cli.build_deck") as build,
                patch("lecturekit.cli.open_in_browser"),
            ):
                main(["view", LECTURE_SOURCE, "--out", tmp, "--pdf"])
            build.assert_called_once_with(Path(tmp), ("html", "pdf"), name="sample-lecture")

    def test_render_no_build_skips_marp(self):
        with tempfile.TemporaryDirectory() as tmp:
            with patch("lecturekit.cli.build_deck") as build:
                exit_code = main(["render", LECTURE_SOURCE, "--to", "viewer", "--out", tmp, "--no-build"])
            self.assertEqual(exit_code, 0)
            self.assertTrue(Path(tmp, "slides.md").exists())
            build.assert_not_called()

    def test_render_viewer_json_skips_notes(self):
        with tempfile.TemporaryDirectory() as tmp:
            main(["render", LECTURE_SOURCE, "--to", "viewer", "--out", tmp, "--no-build"])
            data = json.loads(Path(tmp, "lecture.json").read_text("utf-8"))
        kinds = [b["kind"] for b in data["pages"][0]["blocks"]]
        self.assertEqual(kinds, ["slide"])

    def test_view_renders_builds_marp_and_opens_browser(self):
        with tempfile.TemporaryDirectory() as tmp:
            with (
                patch("lecturekit.cli.build_deck") as build,
                patch("lecturekit.cli.open_in_browser") as opener,
            ):
                exit_code = main(["view", LECTURE_SOURCE, "--out", tmp])
            self.assertEqual(exit_code, 0)
            self.assertTrue(Path(tmp, "index.html").exists())
            build.assert_called_once_with(Path(tmp), ("html",), name="sample-lecture")
            opener.assert_called_once_with(Path(tmp, "index.html"))

    def test_view_no_build_skips_marp(self):
        with tempfile.TemporaryDirectory() as tmp:
            with (
                patch("lecturekit.cli.build_deck") as build,
                patch("lecturekit.cli.open_in_browser"),
            ):
                main(["view", LECTURE_SOURCE, "--out", tmp, "--no-build"])
            build.assert_not_called()

    def test_view_watch_starts_dev_server(self):
        with tempfile.TemporaryDirectory() as tmp:
            with patch("lecturekit.cli.serve") as serve:
                exit_code = main(["view", LECTURE_SOURCE, "--watch", "--out", tmp, "--port", "4111"])
            self.assertEqual(exit_code, 0)
            serve.assert_called_once_with(
                Path(LECTURE_SOURCE), Path(tmp), port=4111, reveal=True,
                debounce_ms=DEFAULT_DEBOUNCE_MS, lang=None, strict=False,
                demo=False, demo_timeout_s=DEFAULT_DEMO_TIMEOUT_S,
            )

    def test_view_demo_is_off_unless_asked_for(self):
        with tempfile.TemporaryDirectory() as tmp:
            with patch("lecturekit.cli.serve") as serve:
                main([
                    "view", LECTURE_SOURCE, "--watch", "--out", tmp,
                    "--demo", "--demo-timeout", "5",
                ])
            self.assertEqual(serve.call_args.kwargs["demo"], True)
            self.assertEqual(serve.call_args.kwargs["demo_timeout_s"], 5.0)

    def test_view_demo_without_watch_is_refused(self):
        # A demo needs something running to answer the press; a plain `view`
        # writes files and opens them. Refused rather than silently inert.
        with tempfile.TemporaryDirectory() as tmp:
            with patch("lecturekit.cli.serve") as serve:
                exit_code = main(["view", LECTURE_SOURCE, "--demo", "--out", tmp])
            self.assertEqual(exit_code, 1)
            serve.assert_not_called()

    def test_view_watch_rejects_a_page_selection(self):
        # A watch session re-resolves --pages on every save, so renaming or
        # deleting the selected page wedged it: every later render raised and no
        # edit could recover it. The combination buys nothing -- the viewer
        # already restores the reader's page across live reloads -- so it is
        # refused outright rather than half-supported.
        with tempfile.TemporaryDirectory() as tmp:
            with patch("lecturekit.cli.serve") as serve:
                exit_code = main(
                    ["view", LECTURE_SOURCE, "--watch", "--pages", "1", "--out", tmp]
                )
            self.assertEqual(exit_code, 1)
            serve.assert_not_called()

    def test_view_watch_no_reveal_disables_reveal(self):
        with tempfile.TemporaryDirectory() as tmp:
            with patch("lecturekit.cli.serve") as serve:
                exit_code = main(
                    ["view", LECTURE_SOURCE, "--watch", "--no-reveal", "--out", tmp]
                )
            self.assertEqual(exit_code, 0)
            self.assertFalse(serve.call_args.kwargs["reveal"])

    def test_open_in_browser_uses_native_opener_on_macos(self):
        from lecturekit.cli import open_in_browser
        with tempfile.TemporaryDirectory() as tmp:
            entry = Path(tmp, "index.html")
            entry.write_text("x", encoding="utf-8")
            with (
                patch("lecturekit.cli.platform.system", return_value="Darwin"),
                patch("lecturekit.cli.subprocess.run") as run,
                patch("lecturekit.cli.webbrowser.open") as browser_open,
            ):
                open_in_browser(entry)
            run.assert_called_once_with(["open", str(entry.resolve())], check=True)
            browser_open.assert_not_called()

    def test_view_viewer_script_is_generic_and_checks_index_html(self):
        script = Path("scripts/view-viewer.sh").read_text(encoding="utf-8")
        self.assertIn("index.html", script)
        self.assertNotIn("lec01", script)

    def test_stop_watch_script_targets_lecturekit_watch_processes(self):
        script = Path("scripts/stop-watch.sh").read_text(encoding="utf-8")
        self.assertIn("lecturekit.cli view", script)
        self.assertIn("--watch", script)
        self.assertIn("kill -INT", script)
        self.assertIn("kill -TERM", script)
        self.assertIn("usage: stop-watch.sh [PORT]", script)

    def test_stop_watch_without_port_stops_all_discovered_watch_ports(self):
        with tempfile.TemporaryDirectory() as tmp:
            bin_dir = Path(tmp, "bin")
            bin_dir.mkdir()
            kill_log = Path(tmp, "kill.log")
            self._write_fake_stop_watch_tools(bin_dir, kill_log)

            env = os.environ.copy()
            env["PATH"] = f"{bin_dir}{os.pathsep}{env['PATH']}"
            env["KILL_LOG"] = str(kill_log)

            result = subprocess.run(
                ["bash", "-c", "enable -n kill; source scripts/stop-watch.sh"],
                text=True, capture_output=True, check=True, env=env,
            )

            self.assertIn("port(s) 3030, 3031", result.stdout)
            self.assertIn("-INT 111 222", kill_log.read_text(encoding="utf-8"))

    def test_stop_watch_with_port_stops_only_that_port(self):
        with tempfile.TemporaryDirectory() as tmp:
            bin_dir = Path(tmp, "bin")
            bin_dir.mkdir()
            kill_log = Path(tmp, "kill.log")
            self._write_fake_stop_watch_tools(bin_dir, kill_log)

            env = os.environ.copy()
            env["PATH"] = f"{bin_dir}{os.pathsep}{env['PATH']}"
            env["KILL_LOG"] = str(kill_log)

            result = subprocess.run(
                ["bash", "-c", "enable -n kill; set -- 3031; source scripts/stop-watch.sh"],
                text=True, capture_output=True, check=True, env=env,
            )

            self.assertIn("port(s) 3031", result.stdout)
            self.assertIn("-INT 222", kill_log.read_text(encoding="utf-8"))
            self.assertNotIn("-INT 111 222", kill_log.read_text(encoding="utf-8"))

    def _write_fake_stop_watch_tools(self, bin_dir: Path, kill_log: Path) -> None:
        ps = bin_dir / "ps"
        ps.write_text(
            "#!/usr/bin/env bash\n"
            "cat <<'EOF'\n"
            "111 python3 -m lecturekit.cli view ./lec-a --watch\n"
            "222 python3 -m lecturekit.cli view ./lec-b --watch --port 3031\n"
            "333 python3 -m other.tool --watch --port 3032\n"
            "EOF\n",
            encoding="utf-8",
        )
        kill = bin_dir / "kill"
        kill.write_text(
            "#!/usr/bin/env bash\n"
            'printf "%s\\n" "$*" >> "$KILL_LOG"\n'
            'if [ "${1:-}" = "-0" ]; then exit 1; fi\n'
            "exit 0\n",
            encoding="utf-8",
        )
        sleep = bin_dir / "sleep"
        sleep.write_text("#!/usr/bin/env bash\nexit 0\n", encoding="utf-8")
        for tool in (ps, kill, sleep):
            tool.chmod(0o755)
        kill_log.write_text("", encoding="utf-8")


class PptxCliTest(unittest.TestCase):
    def test_render_to_pptx_writes_file_and_skips_marp(self):
        with tempfile.TemporaryDirectory() as tmp:
            with patch("lecturekit.cli.build_deck") as build:
                exit_code = main(["render", LECTURE_SOURCE, "--to", "pptx", "--out", tmp])
            self.assertEqual(exit_code, 0)
            # the pptx renderer produces the deck directly; no Marp/Chrome step
            build.assert_not_called()
            self.assertTrue(Path(tmp, "sample-lecture.pptx").exists())


MULTIPAGE_SOURCE = "tests/fixtures/multipage"


class PageSelectionCliTest(unittest.TestCase):
    def _rendered_page_ids(self, tmp):
        data = json.loads(Path(tmp, "lecture.json").read_text("utf-8"))
        return [p["id"] for p in data["pages"]]

    def test_render_pages_by_index_filters_the_bundle(self):
        with tempfile.TemporaryDirectory() as tmp:
            main(["render", MULTIPAGE_SOURCE, "--out", tmp, "--no-build", "--pages", "2-3"])
            self.assertEqual(self._rendered_page_ids(tmp), ["beta", "gamma"])

    def test_render_pages_by_id_filters_the_bundle(self):
        with tempfile.TemporaryDirectory() as tmp:
            main(["render", MULTIPAGE_SOURCE, "--out", tmp, "--no-build", "--pages", "delta"])
            self.assertEqual(self._rendered_page_ids(tmp), ["delta"])

    def test_render_without_pages_keeps_every_page(self):
        with tempfile.TemporaryDirectory() as tmp:
            main(["render", MULTIPAGE_SOURCE, "--out", tmp, "--no-build"])
            self.assertEqual(
                self._rendered_page_ids(tmp), ["alpha", "beta", "gamma", "delta"]
            )

    def test_render_pages_unknown_id_exits_nonzero(self):
        with tempfile.TemporaryDirectory() as tmp:
            code = main(["render", MULTIPAGE_SOURCE, "--out", tmp, "--no-build", "--pages", "nope"])
            self.assertEqual(code, 1)

    def test_view_pages_filters_the_bundle(self):
        with tempfile.TemporaryDirectory() as tmp:
            with (
                patch("lecturekit.cli.build_deck"),
                patch("lecturekit.cli.open_in_browser"),
            ):
                main(["view", MULTIPAGE_SOURCE, "--out", tmp, "--pages", "1", "--no-build"])
            self.assertEqual(self._rendered_page_ids(tmp), ["alpha"])


class PngExportCliTest(unittest.TestCase):
    def test_render_png_flag_requests_html_and_png(self):
        with tempfile.TemporaryDirectory() as tmp:
            with patch("lecturekit.cli.build_deck") as build:
                main(["render", MULTIPAGE_SOURCE, "--out", tmp, "--png"])
            build.assert_called_once_with(Path(tmp), ("html", "png"), name="multi")

    def test_render_pdf_and_png_together(self):
        with tempfile.TemporaryDirectory() as tmp:
            with patch("lecturekit.cli.build_deck") as build:
                main(["render", MULTIPAGE_SOURCE, "--out", tmp, "--pdf", "--png"])
            build.assert_called_once_with(Path(tmp), ("html", "pdf", "png"), name="multi")


class SiblingModuleIsolationTest(unittest.TestCase):
    """Two lectures, one process, same sibling module name.

    ``import pages`` is a bare name, so without isolation whichever lecture
    loaded first would answer for both. This bites the book target (chapter
    after chapter) and review sources (loaded from inside their host).
    """

    def _lecture(self, root: Path, name: str, text: str) -> Path:
        directory = root / name
        directory.mkdir()
        (directory / "pages.py").write_text(
            f'def body(p):\n    p.title("{text}")\n    p.slide("{text}")\n'
        )
        (directory / "lecture.py").write_text(
            "from lecturekit.dsl import Lecture\n"
            "import pages\n"
            f'lecture = Lecture(id="{name}", title="{name}")\n'
            'lecture.page("only", body=pages.body)\n'
        )
        return directory

    def test_each_lecture_gets_its_own_sibling_module(self):
        from lecturekit.cli import load_lecture

        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            first = self._lecture(root, "first", "from first")
            second = self._lecture(root, "second", "from second")

            self.assertEqual(load_lecture(first).children[0].title, "from first")
            self.assertEqual(load_lecture(second).children[0].title, "from second")
            # ...and back, so the second load did not poison the first either.
            self.assertEqual(load_lecture(first).children[0].title, "from first")

    def test_a_lecture_load_leaves_no_sibling_module_cached(self):
        from lecturekit.cli import load_lecture

        with tempfile.TemporaryDirectory() as tmp:
            directory = self._lecture(Path(tmp), "solo", "solo")
            load_lecture(directory)
            self.assertNotIn("pages", sys.modules)
