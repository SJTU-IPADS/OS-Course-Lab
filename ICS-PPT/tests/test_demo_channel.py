"""The live-demo channel: `p.demo(...)` blocks that run their command.

Two properties worth protecting. The first is the one-way mapping: a request
names a demo by id and the server resolves it against what the author wrote;
nothing a caller sends can become a command. The second is that output arrives
while the command is still running, and that a listener hanging up is what
stops it — there is no second request to send, so the connection *is* the
handle. Several tests below poke at exactly those two seams.
"""

import json
import os
import socket
import tempfile
import threading
import time
import unittest
import urllib.error
import urllib.request
from contextlib import closing
from http.server import ThreadingHTTPServer
from pathlib import Path

from lecturekit import demo, dev_server
from lecturekit.dsl import Lecture
from lecturekit.renderers.viewer import StaticViewerRenderer
from lecturekit.renderers.viewer import marp
from lecturekit.renderers.viewer.blocks import BLOCK_RENDERERS, render_block
from lecturekit import model


def _lecture(*commands: str, disabled: bool = False,
             timeout: float | None = None,
             interactive: bool = False) -> model.Lecture:
    lecture = Lecture(id="lec", title="L")

    def body(p):
        p.title("P")
        p.slide("body")  # a page needs content even when it carries no demo
        for index, command in enumerate(commands):
            handle = p.demo(
                f"demo {index}", command, timeout=timeout, interactive=interactive
            )
            if disabled:
                handle.disable()

    lecture.page("p1", body=body)
    return lecture.build()


class DemoIdTest(unittest.TestCase):
    def test_id_is_stable_and_command_specific(self):
        self.assertEqual(demo.demo_id("echo hi"), demo.demo_id("echo hi"))
        self.assertNotEqual(demo.demo_id("echo hi"), demo.demo_id("echo ho"))

    def test_collect_maps_id_to_the_authored_command(self):
        table = demo.collect(_lecture("echo hi", "ls -l"))
        self.assertEqual(
            {key: spec.command for key, spec in table.items()},
            {demo.demo_id("echo hi"): "echo hi", demo.demo_id("ls -l"): "ls -l"},
        )

    def test_a_disabled_block_is_not_runnable(self):
        # `.disable()` takes a block out of every target ahead of any other
        # control, so its command is not part of this deck at all.
        self.assertEqual(demo.collect(_lecture("rm -rf /", disabled=True)), {})

    def test_two_pages_running_the_same_command_share_one_id(self):
        table = demo.collect(_lecture("make", "make"))
        self.assertEqual([spec.command for spec in table.values()], ["make"])


class DemoTableFileTest(unittest.TestCase):
    def test_round_trips_through_the_bundle(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            demo.write(_lecture("echo hi"), out)
            self.assertEqual(
                demo.read(out), {demo.demo_id("echo hi"): demo.Spec("echo hi")}
            )

    def test_interactive_round_trips_and_is_off_unless_asked(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            demo.write(_lecture("ollama run llama3.2", interactive=True), out)
            self.assertTrue(demo.read(out)[demo.demo_id("ollama run llama3.2")].interactive)
            demo.write(_lecture("ls"), out)
            self.assertFalse(demo.read(out)[demo.demo_id("ls")].interactive)

    def test_the_blocks_own_timeout_round_trips(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            demo.write(_lecture("ollama serve", timeout=0), out)
            spec = demo.read(out)[demo.demo_id("ollama serve")]
        self.assertEqual(spec.timeout, 0)

    def test_a_deleted_demo_stops_resolving_after_a_re_render(self):
        # The whole reason the table is a file rewritten on every render rather
        # than a table built once at startup.
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            demo.write(_lecture("echo hi"), out)
            demo.write(_lecture(), out)
            self.assertEqual(demo.read(out), {})

    def test_a_missing_or_broken_table_resolves_nothing(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            self.assertEqual(demo.read(out), {})
            Path(out, demo.DEMOS_FILENAME).write_text("not json", encoding="utf-8")
            self.assertEqual(demo.read(out), {})

    def test_the_viewer_render_writes_the_table(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            StaticViewerRenderer().render(_lecture("echo hi"), out)
            self.assertEqual(
                demo.read(out), {demo.demo_id("echo hi"): demo.Spec("echo hi")}
            )


class DemoRunTest(unittest.TestCase):
    def test_captures_output_and_exit_code(self):
        with tempfile.TemporaryDirectory() as tmp:
            result = demo.run("echo hello", cwd=Path(tmp))
        self.assertEqual(result.output.strip(), "hello")
        self.assertEqual(result.exit_code, 0)
        self.assertFalse(result.timed_out)

    def test_stderr_is_merged_into_the_one_transcript(self):
        with tempfile.TemporaryDirectory() as tmp:
            result = demo.run("echo out; echo err 1>&2", cwd=Path(tmp))
        self.assertIn("out", result.output)
        self.assertIn("err", result.output)

    def test_a_failing_command_reports_its_status(self):
        with tempfile.TemporaryDirectory() as tmp:
            result = demo.run("exit 3", cwd=Path(tmp))
        self.assertEqual(result.exit_code, 3)

    def test_runs_in_the_lecture_directory(self):
        with tempfile.TemporaryDirectory() as tmp:
            Path(tmp, "beside-the-lecture.txt").write_text("x", encoding="utf-8")
            result = demo.run("ls", cwd=Path(tmp))
        self.assertIn("beside-the-lecture.txt", result.output)

    def test_a_long_run_is_killed_rather_than_waited_out(self):
        started = time.monotonic()
        with tempfile.TemporaryDirectory() as tmp:
            result = demo.run("sleep 30", cwd=Path(tmp), timeout_s=0.4)
        self.assertTrue(result.timed_out)
        self.assertIsNone(result.exit_code)
        self.assertIn("killed", result.output)
        self.assertLess(time.monotonic() - started, 10)

    def test_stdin_is_not_a_terminal_and_is_already_over(self):
        # Display-only is the default: a command that asks for input gets end
        # of file at once rather than waiting for a keyboard nobody has.
        with tempfile.TemporaryDirectory() as tmp:
            result = demo.run("[ -t 0 ] || echo not-a-tty; cat", cwd=Path(tmp))
        self.assertEqual(result.output, "not-a-tty\n")
        self.assertEqual(result.exit_code, 0)

    def test_output_is_capped(self):
        with tempfile.TemporaryDirectory() as tmp:
            result = demo.run(
                f"head -c {demo.MAX_OUTPUT_BYTES * 2} /dev/zero | tr '\\0' 'x'",
                cwd=Path(tmp),
            )
        self.assertIn("dropped", result.output)
        self.assertLess(len(result.output), demo.MAX_OUTPUT_BYTES * 1.5)


class DemoStreamTest(unittest.TestCase):
    """`stream` is the primitive: output as it happens, and a way to stop."""

    def _events(self, command, **kwargs):
        with tempfile.TemporaryDirectory() as tmp:
            return list(demo.stream(command, cwd=Path(tmp), **kwargs))

    def _after_command(self, events):
        """The first event past the announcement of the command itself."""
        return next(e for e in events if not isinstance(e, demo.Command))

    def test_a_chunk_arrives_before_the_command_is_over(self):
        started = time.monotonic()
        with tempfile.TemporaryDirectory() as tmp:
            events = demo.stream("echo first; sleep 1.5", cwd=Path(tmp))
            first = self._after_command(events)
            elapsed = time.monotonic() - started
            events.close()
        self.assertIsInstance(first, demo.Chunk)
        self.assertEqual(first.text.strip(), "first")
        self.assertLess(elapsed, 1.0)

    def test_a_quiet_command_still_says_something(self):
        # The tick is the clock the drawer counts on, and the write that tells
        # this end whether anyone is still listening.
        with tempfile.TemporaryDirectory() as tmp:
            events = demo.stream("sleep 5", cwd=Path(tmp))
            first = self._after_command(events)
            events.close()
        self.assertIsInstance(first, demo.Tick)

    def test_closing_the_stream_kills_the_command(self):
        # The whole stop mechanism: no signal to send, just stop listening.
        with tempfile.TemporaryDirectory() as tmp:
            events = demo.stream("echo $$; sleep 30", cwd=Path(tmp))
            pid = int(self._after_command(events).text.strip())
            events.close()
        with self.assertRaises(ProcessLookupError):
            os.kill(pid, 0)

    def test_it_ends_by_saying_how_it_went(self):
        events = self._events("exit 4")
        self.assertIsInstance(events[-1], demo.Done)
        self.assertEqual(events[-1].exit_code, 4)

    def test_terminal_escapes_reach_the_drawer_as_written(self):
        # The drawer is a terminal: colour, and a `\r` that redraws a progress
        # bar's line, are for it to draw rather than for this end to strip.
        events = self._events(r"printf '\033[32mgreen\033[0m\r100%%\n'")
        text = "".join(e.text for e in events if isinstance(e, demo.Chunk))
        self.assertEqual(text, "\x1b[32mgreen\x1b[0m\r100%\n")


class DemoTranscriptTest(unittest.TestCase):
    """Each authored command appears as bash reaches it, above its output."""

    def _events(self, command):
        with tempfile.TemporaryDirectory() as tmp:
            return [
                e for e in demo.stream(command, cwd=Path(tmp))
                if not isinstance(e, demo.Tick)
            ]

    def _commands(self, events):
        return [e.text for e in events if isinstance(e, demo.Command)]

    def _output(self, events):
        return "".join(e.text for e in events if isinstance(e, demo.Chunk))

    def test_each_command_comes_just_before_what_it_printed(self):
        # The bug this fixes: a multi-line demo showed its first line and "…",
        # and then output nobody could match to a line.
        events = self._events("echo one\necho two")
        self.assertEqual(
            events[:-1],
            [
                demo.Command("$ echo one"),
                demo.Chunk("one\n"),
                demo.Command("$ echo two"),
                demo.Chunk("two\n"),
            ],
        )
        self.assertIsInstance(events[-1], demo.Done)

    def test_a_command_over_several_lines_is_shown_whole_and_once(self):
        # As bash echoes it: `$` where a command starts, `>` where it goes on.
        events = self._events("for i in 1 2; do\n  echo $i\ndone\necho a \\\n  b")
        self.assertEqual(
            self._commands(events),
            ["$ for i in 1 2; do\n>   echo $i\n> done", "$ echo a \\\n>   b"],
        )
        self.assertEqual(self._output(events), "1\n2\na b\n")

    def test_a_comment_rides_along_with_the_command_after_it(self):
        events = self._events("# first, the compiler\necho cc\n\n# never run")
        self.assertEqual(
            self._commands(events), ["$ # first, the compiler", "$ echo cc"]
        )

    def test_what_never_ran_is_never_shown(self):
        events = self._events("set -e\nfalse\necho never")
        self.assertEqual(self._commands(events), ["$ set -e", "$ false"])
        self.assertEqual(events[-1].exit_code, 1)

    def test_the_shell_state_is_the_authors_own(self):
        # The markers come from a DEBUG trap; it must not show. `$?` survives
        # it, and `$LINENO` counts the author's lines, not the trap's.
        events = self._events("false\necho $?\necho $LINENO")
        self.assertEqual(self._output(events), "1\n3\n")

    def test_a_redirect_does_not_capture_a_marker(self):
        with tempfile.TemporaryDirectory() as tmp:
            command = "{ echo a; echo b; } > f.txt\nx=$(echo c)\ncat f.txt; echo $x"
            events = list(demo.stream(command, cwd=Path(tmp)))
            written = Path(tmp, "f.txt").read_text(encoding="utf-8")
        self.assertEqual(written, "a\nb\n")
        self.assertEqual(self._output(events), "a\nb\nc\n")

    def test_a_function_or_a_script_it_runs_announces_nothing_of_its_own(self):
        command = (
            "f() {\n  echo in-f\n}\n"
            "printf 'echo in-script\\necho two\\n' > s.sh\n"
            "f\nbash s.sh\nf"
        )
        events = self._events(command)
        self.assertEqual(
            self._commands(events),
            [
                "$ f() {\n>   echo in-f\n> }",
                "$ printf 'echo in-script\\necho two\\n' > s.sh",
                "$ f",
                "$ bash s.sh",
                "$ f",
            ],
        )
        self.assertEqual(self._output(events), "in-f\nin-script\ntwo\nin-f\n")

    def test_bash_decides_where_a_command_ends(self):
        self.assertEqual(
            demo.command_spans(
                "cd x\nfor i in 1; do\n  echo\ndone\ncurl a \\\n  -d b\n"
                "cat <<EOF\nbody\nEOF\necho 'two\nlines'"
            ),
            ((0, 0), (1, 3), (4, 5), (6, 8), (9, 10)),
        )

    def test_a_marker_split_across_two_reads_is_still_taken_out(self):
        markers = demo._Markers()
        self.assertEqual(markers.feed(b"out\x1b]77"), [b"out"])
        self.assertEqual(markers.feed(b"17;3\x07more"), [3, b"more"])

    def test_a_programs_own_escapes_are_left_alone(self):
        markers = demo._Markers()
        title = b"\x1b]0;title\x07\x1b[1mbold\x1b]7717;x\x07"
        self.assertEqual(b"".join(markers.feed(title)), title)


class DemoTerminalTest(unittest.TestCase):
    """An interactive run: a terminal the deck types into."""

    def _run(self, command, terminal, on_output=None, timeout_s=10):
        """Every event of ``command`` on ``terminal``; ``on_output`` sees each
        chunk's text as it arrives, and may type."""
        events = []
        with tempfile.TemporaryDirectory() as tmp:
            for event in demo.stream(
                command, cwd=Path(tmp), terminal=terminal, timeout_s=timeout_s
            ):
                events.append(event)
                if on_output and isinstance(event, demo.Chunk):
                    on_output(event.text)
        return events

    def _output(self, events):
        return "".join(e.text for e in events if isinstance(e, demo.Chunk))

    def test_stdin_is_a_terminal_of_the_size_asked_for(self):
        terminal = demo.Terminal(100, 30)
        events = self._run("[ -t 0 ] && echo tty\nstty size\necho $TERM", terminal)
        self.assertEqual(self._output(events), "tty\r\n30 100\r\nxterm-256color\r\n")

    def test_what_is_typed_reaches_the_program(self):
        terminal = demo.Terminal()

        def answer(text):
            if "name? " in text:
                self.assertTrue(terminal.write(b"bob\r"))

        events = self._run("read -p 'name? ' n\necho hello $n", terminal, answer)
        self.assertIn("hello bob", self._output(events))
        self.assertEqual(events[-1].exit_code, 0)

    def test_ctrl_c_interrupts_as_it_would_at_a_terminal(self):
        terminal = demo.Terminal()
        started = time.monotonic()

        def interrupt(text):
            if "ready" in text:
                terminal.write(b"\x03")

        events = self._run("echo ready; sleep 30\necho after", terminal, interrupt)
        self.assertEqual(events[-1].exit_code, 130)  # 128 + SIGINT, as bash says
        self.assertNotIn("after", self._output(events))
        self.assertLess(time.monotonic() - started, 10)

    def test_a_resize_reaches_the_program(self):
        terminal = demo.Terminal(100, 30)

        def resize(text):
            if "30 100" in text:
                terminal.resize(60, 12)
                terminal.write(b"\r")

        events = self._run("stty size; read x; stty size", terminal, resize)
        self.assertIn("12 60", self._output(events))

    def test_nothing_can_be_typed_once_the_run_is_over(self):
        terminal = demo.Terminal()
        self._run("true", terminal)
        self.assertFalse(terminal.write(b"x"))

    def test_a_size_is_kept_within_reason(self):
        self.assertEqual((demo.Terminal(10**6, -5).cols, demo.Terminal(10**6, -5).rows), (500, 1))
        garbled = demo.Terminal("wide", None)
        self.assertEqual((garbled.cols, garbled.rows), (80, 24))


class DemoTimeoutTest(unittest.TestCase):
    def test_the_block_wins_over_the_session(self):
        self.assertEqual(demo.timeout_for(demo.Spec("x", timeout=5), 120), 5)

    def test_the_session_applies_when_the_block_said_nothing(self):
        self.assertEqual(demo.timeout_for(demo.Spec("x"), 120), 120)

    def test_zero_means_no_limit(self):
        # How an author writes `ollama serve`: this one does not end on its own.
        self.assertIsNone(demo.timeout_for(demo.Spec("x", timeout=0), 120))
        self.assertIsNone(demo.timeout_for(demo.Spec("x"), 0))


class PromptLinesTest(unittest.TestCase):
    def test_each_command_line_gets_a_prompt(self):
        self.assertEqual(
            demo.prompt_lines("cd examples\ngcc a.c"), ["$ cd examples", "$ gcc a.c"]
        )

    def test_a_continuation_keeps_the_authors_indentation(self):
        self.assertEqual(
            demo.prompt_lines("curl x \\\n    -d '{}'"),
            ["$ curl x \\", "    -d '{}'"],
        )

    def test_an_argument_that_runs_over_several_lines_is_one_command(self):
        # A curl whose JSON body wraps: a prompt in the middle of it would be
        # saying there are three commands here, and there is one.
        command = "curl x -d '{\n  \"a\": 1\n}'"
        self.assertEqual(
            demo.prompt_lines(command),
            ["$ curl x -d '{", '  "a": 1', "}'"],
        )

    def test_an_apostrophe_in_a_comment_does_not_open_a_quote(self):
        self.assertEqual(
            demo.prompt_lines("gcc a.c   # it's the first stage\nls"),
            ["$ gcc a.c   # it's the first stage", "$ ls"],
        )


class DemoBlockRenderTest(unittest.TestCase):
    def _block(self, description=None, command="gcc -O2 -S demo.c", output=None):
        return model.Block(
            kind="demo",
            content={
                "name": "compile it",
                "command": command,
                "output": output,
                "description": description,
                "timeout": None,
            },
        )

    def test_the_deck_now_draws_a_demo(self):
        self.assertIn("demo", BLOCK_RENDERERS)

    def test_the_chip_carries_the_id_and_not_a_runnable_command_attribute(self):
        html = "".join(render_block(self._block()))
        self.assertIn(f'data-lk-demo="{demo.demo_id("gcc -O2 -S demo.c")}"', html)
        self.assertIn("compile it", html)
        self.assertIn("gcc -O2 -S demo.c", html)

    def test_the_button_ships_disabled(self):
        # A rendered bundle has no server behind it, so the resting state of the
        # control is off; only the injected controller arms it.
        html = "".join(render_block(self._block()))
        self.assertIn("<button", html)
        self.assertIn("disabled", html)

    def test_a_bare_one_liner_stays_one_row(self):
        html = "".join(render_block(self._block()))
        self.assertIn('data-lk-demo-form="inline"', html)
        self.assertNotIn("<pre", html)

    def test_a_command_with_output_becomes_a_transcript(self):
        # What keeps a page that used to hold a `p.code(...)` looking like
        # itself: the recorded output stays on the slide, in the same box.
        html = "".join(render_block(self._block(output="demo.s written")))
        self.assertIn('data-lk-demo-form="block"', html)
        self.assertIn("<pre", html)
        self.assertIn("demo.s written", html)

    def test_a_multi_line_command_is_a_transcript_too(self):
        html = "".join(render_block(self._block(command="cd examples\ngcc a.c")))
        self.assertIn('data-lk-demo-form="block"', html)
        self.assertIn("$ cd examples\n$ gcc a.c", html)

    def test_the_command_element_holds_the_command_and_nothing_else(self):
        # `demo.js` may read it to label a run, so nothing else may be in it.
        html = "".join(render_block(self._block(description="see the assembly")))
        self.assertIn('<code class="lk-demo-cmd">$ gcc -O2 -S demo.c</code>', html)

    def test_markup_in_the_author_text_is_escaped(self):
        block = model.Block(
            kind="demo",
            content={"name": "<script>x</script>", "command": "echo <b>",
                     "output": "<i>bad</i>", "description": None, "timeout": None},
        )
        html = "".join(render_block(block))
        self.assertNotIn("<script>", html)
        self.assertNotIn("<i>bad</i>", html)
        self.assertIn("&lt;script&gt;", html)


class DemoEndpointTest(unittest.TestCase):
    def _server(self, directory: Path, demo_cwd=None, timeout_s=30.0):
        handler = dev_server.make_handler(
            directory,
            dev_server.ReloadBroadcaster(),
            demo_cwd=demo_cwd,
            demo_timeout_s=timeout_s,
        )
        server = ThreadingHTTPServer(("127.0.0.1", 0), handler)
        threading.Thread(target=server.serve_forever, daemon=True).start()
        self.addCleanup(server.server_close)
        self.addCleanup(server.shutdown)
        return server.server_address[1]

    def _post(self, port, payload, content_type="application/json"):
        """POST and read the whole stream: (status, events, joined output)."""
        request = urllib.request.Request(
            f"http://127.0.0.1:{port}{dev_server.DEMO_PATH}",
            data=json.dumps(payload).encode("utf-8"),
            headers={"Content-Type": content_type},
            method="POST",
        )
        with closing(urllib.request.urlopen(request, timeout=20)) as resp:
            body = resp.read().decode("utf-8")
            status = resp.status
        events = [json.loads(line) for line in body.splitlines() if line]
        output = "".join(e["d"] for e in events if e["t"] == "out")
        return status, events, output

    def _end(self, events):
        return next(e for e in events if e["t"] == "end")

    def test_a_known_id_runs_its_command(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            demo.write(_lecture("echo ran-it"), out)
            port = self._server(out, demo_cwd=out)
            status, events, output = self._post(
                port, {"id": demo.demo_id("echo ran-it")}
            )
        self.assertEqual(status, 200)
        self.assertEqual(output.strip(), "ran-it")
        self.assertEqual(self._end(events)["exit"], 0)

    def test_an_unknown_id_runs_nothing(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            demo.write(_lecture("echo hi"), out)
            port = self._server(out, demo_cwd=out)
            with self.assertRaises(urllib.error.HTTPError) as caught:
                self._post(port, {"id": "deadbeefcafe"})
        self.assertEqual(caught.exception.code, 404)

    def test_a_caller_cannot_send_a_command_of_its_own(self):
        # The mapping is one-way by construction: the body has nowhere to put a
        # command, and an id that is not in the table resolves to nothing.
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            demo.write(_lecture("echo hi"), out)
            marker = Path(tmp, "should-not-exist")
            port = self._server(out, demo_cwd=out)
            with self.assertRaises(urllib.error.HTTPError):
                self._post(port, {"id": "x", "command": f"touch {marker}"})
        self.assertFalse(marker.exists())

    def test_a_form_content_type_is_refused(self):
        # What keeps a page on another origin from POSTing here uninvited: it
        # cannot set a JSON content type, and the fetch that could is held back
        # by a preflight this server never answers.
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            demo.write(_lecture("echo hi"), out)
            port = self._server(out, demo_cwd=out)
            with self.assertRaises(urllib.error.HTTPError) as caught:
                self._post(
                    port,
                    {"id": demo.demo_id("echo hi")},
                    content_type="application/x-www-form-urlencoded",
                )
        self.assertEqual(caught.exception.code, 400)

    def test_the_endpoint_is_absent_unless_demos_are_armed(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            demo.write(_lecture("echo hi"), out)
            port = self._server(out)  # no demo_cwd: the default
            with self.assertRaises(urllib.error.HTTPError) as caught:
                self._post(port, {"id": demo.demo_id("echo hi")})
        self.assertEqual(caught.exception.code, 404)

    def test_the_timeout_is_the_one_the_session_was_given(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            demo.write(_lecture("sleep 30"), out)
            port = self._server(out, demo_cwd=out, timeout_s=0.4)
            status, events, _ = self._post(port, {"id": demo.demo_id("sleep 30")})
        self.assertEqual(status, 200)
        self.assertTrue(self._end(events)["timedOut"])
        self.assertIsNone(self._end(events)["exit"])

    def test_a_blocks_own_timeout_overrides_the_sessions(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            demo.write(_lecture("sleep 30", timeout=0.3), out)
            port = self._server(out, demo_cwd=out, timeout_s=3600)
            started = time.monotonic()
            _, events, _ = self._post(port, {"id": demo.demo_id("sleep 30")})
        self.assertTrue(self._end(events)["timedOut"])
        self.assertLess(time.monotonic() - started, 10)

    def test_output_reaches_the_listener_before_the_command_is_over(self):
        # The point of the whole change: a model answering a token at a time is
        # watched, not waited for.
        command = "echo first; sleep 1.5; echo second"
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            demo.write(_lecture(command), out)
            port = self._server(out, demo_cwd=out)
            request = urllib.request.Request(
                f"http://127.0.0.1:{port}{dev_server.DEMO_PATH}",
                data=json.dumps({"id": demo.demo_id(command)}).encode("utf-8"),
                headers={"Content-Type": "application/json"},
                method="POST",
            )
            started = time.monotonic()
            with closing(urllib.request.urlopen(request, timeout=20)) as resp:
                announced = json.loads(resp.readline().decode("utf-8"))
                first = json.loads(resp.readline().decode("utf-8"))
                elapsed = time.monotonic() - started
                resp.read()
        self.assertEqual(announced, {"t": "cmd", "d": f"$ {command}"})
        self.assertEqual(first["t"], "out")
        self.assertEqual(first["d"].strip(), "first")
        self.assertLess(elapsed, 1.0)

    def test_the_stream_is_a_transcript(self):
        command = "echo one\necho two"
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            demo.write(_lecture(command), out)
            port = self._server(out, demo_cwd=out)
            _, events, _ = self._post(port, {"id": demo.demo_id(command)})
        self.assertEqual(
            [(e["t"], e.get("d")) for e in events if e["t"] != "tick"][:4],
            [("cmd", "$ echo one"), ("out", "one\n"), ("cmd", "$ echo two"), ("out", "two\n")],
        )

    def test_a_display_only_demo_hands_out_no_keyboard(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            demo.write(_lecture("cat"), out)
            port = self._server(out, demo_cwd=out)
            _, events, _ = self._post(port, {"id": demo.demo_id("cat")})
        self.assertNotIn("stdin", [e["t"] for e in events])
        self.assertEqual(self._end(events)["exit"], 0)  # cat met end of file

    def test_a_listener_that_hangs_up_stops_the_command(self):
        # There is no stop request: the browser aborts its fetch, the socket
        # refuses the next write, and the command's process group is killed.
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            marker = Path(tmp, "kept-running")
            command = f"echo started; sleep 2; touch {marker}"
            demo.write(_lecture(command), out)
            port = self._server(out, demo_cwd=out)
            sock = self._connect(port, demo.demo_id(command))
            self._read_until(sock, b"started")
            sock.close()
            time.sleep(3.0)
            self.assertFalse(marker.exists())

    def test_two_demos_stream_at_the_same_time(self):
        # What the drawer's run tabs stand on: pressing run again starts a
        # second command beside the first rather than behind it. `ollama serve`
        # has to hold its terminal while `ollama run` talks to it.
        service = "echo service-up; sleep 2; echo service-done"
        client = "echo client-up; sleep 0.2; echo client-done"
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            demo.write(_lecture(service, client), out)
            port = self._server(out, demo_cwd=out)
            held = self._connect(port, demo.demo_id(service))
            self._read_until(held, b"service-up")
            started = time.monotonic()
            _, _, output = self._post(port, {"id": demo.demo_id(client)})
            elapsed = time.monotonic() - started
            self._read_until(held, b"service-done")
            held.close()
        self.assertIn("client-done", output)
        self.assertLess(elapsed, 1.5)  # not queued behind the first command

    def test_hanging_up_on_one_run_leaves_the_other_alone(self):
        # Each stream owns exactly its own process group, so stopping one tab
        # is not felt by the tab beside it.
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            dropped = Path(tmp, "aborted-kept-running")
            kept = Path(tmp, "other-finished")
            first = f"echo one-up; sleep 2; touch {dropped}"
            second = f"echo two-up; sleep 2.5; touch {kept}"
            demo.write(_lecture(first, second), out)
            port = self._server(out, demo_cwd=out)
            a = self._connect(port, demo.demo_id(first))
            b = self._connect(port, demo.demo_id(second))
            self._read_until(a, b"one-up")
            self._read_until(b, b"two-up")
            a.close()
            self._read_until(b, b'"end"', limit_s=10.0)
            b.close()
            self.assertFalse(dropped.exists())
            self.assertTrue(kept.exists())

    def _open(self, port, payload):
        """A stream left open, for a test that types into it."""
        request = urllib.request.Request(
            f"http://127.0.0.1:{port}{dev_server.DEMO_PATH}",
            data=json.dumps(payload).encode("utf-8"),
            headers={"Content-Type": "application/json"},
            method="POST",
        )
        resp = urllib.request.urlopen(request, timeout=20)
        self.addCleanup(resp.close)
        return resp

    def _until(self, resp, needle):
        """Events up to and including the output that contains ``needle``."""
        events = []
        while True:
            line = resp.readline()
            self.assertTrue(line, f"stream ended before {needle!r}")
            events.append(json.loads(line.decode("utf-8")))
            if events[-1]["t"] == "out" and needle in events[-1]["d"]:
                return events

    def _send(self, port, path, payload, content_type="application/json"):
        request = urllib.request.Request(
            f"http://127.0.0.1:{port}{path}",
            data=json.dumps(payload).encode("utf-8"),
            headers={"Content-Type": content_type},
            method="POST",
        )
        try:
            with closing(urllib.request.urlopen(request, timeout=10)) as resp:
                return resp.status
        except urllib.error.HTTPError as error:
            return error.code

    def test_an_interactive_demo_is_typed_into_through_its_token(self):
        command = "read -p 'name? ' n\necho hello $n"
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            demo.write(_lecture(command, interactive=True), out)
            port = self._server(out, demo_cwd=out)
            resp = self._open(port, {"id": demo.demo_id(command), "cols": 90, "rows": 20})
            opening = json.loads(resp.readline().decode("utf-8"))
            self.assertEqual(opening["t"], "stdin")
            token = opening["run"]
            self._until(resp, "name? ")
            status = self._send(
                port, dev_server.DEMO_STDIN_PATH, {"run": token, "data": "bob\r"}
            )
            self.assertEqual(status, 204)
            rest = [json.loads(line) for line in resp.read().decode("utf-8").splitlines()]
            # The token dies with its run.
            late = self._send(
                port, dev_server.DEMO_STDIN_PATH, {"run": token, "data": "x"}
            )
        self.assertIn("hello bob", "".join(e["d"] for e in rest if e["t"] == "out"))
        self.assertEqual(self._end(rest)["exit"], 0)
        self.assertEqual(late, 404)

    def test_the_size_the_page_sent_and_later_changes_reach_the_program(self):
        command = "stty size; read x; stty size"
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            demo.write(_lecture(command, interactive=True), out)
            port = self._server(out, demo_cwd=out)
            resp = self._open(port, {"id": demo.demo_id(command), "cols": 90, "rows": 20})
            token = json.loads(resp.readline().decode("utf-8"))["run"]
            self._until(resp, "20 90")
            resized = self._send(
                port, dev_server.DEMO_WINSIZE_PATH, {"run": token, "cols": 70, "rows": 15}
            )
            self._send(port, dev_server.DEMO_STDIN_PATH, {"run": token, "data": "\r"})
            rest = resp.read().decode("utf-8")
        self.assertEqual(resized, 204)
        self.assertIn("15 70", rest)

    def test_keystrokes_for_no_run_or_from_a_form_are_refused(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            demo.write(_lecture("cat", interactive=True), out)
            port = self._server(out, demo_cwd=out)
            unknown = self._send(
                port, dev_server.DEMO_STDIN_PATH, {"run": "guess", "data": "rm -rf ~\r"}
            )
            form = self._send(
                port,
                dev_server.DEMO_STDIN_PATH,
                {"run": "guess", "data": "x"},
                content_type="application/x-www-form-urlencoded",
            )
            resize = self._send(
                port, dev_server.DEMO_WINSIZE_PATH, {"run": "guess", "cols": 1, "rows": 1}
            )
        self.assertEqual(unknown, 404)
        self.assertEqual(form, 400)
        self.assertEqual(resize, 404)

    def _connect(self, port, demo_id):
        """A raw socket mid-stream, so the test can hang up like a browser."""
        body = json.dumps({"id": demo_id}).encode("utf-8")
        sock = socket.create_connection(("127.0.0.1", port), timeout=10)
        sock.sendall(
            b"POST " + dev_server.DEMO_PATH.encode() + b" HTTP/1.0\r\n"
            b"Host: 127.0.0.1\r\n"
            b"Content-Type: application/json\r\n"
            b"Content-Length: " + str(len(body)).encode() + b"\r\n\r\n" + body
        )
        self.addCleanup(sock.close)
        return sock

    def _read_until(self, sock, needle, limit_s=10.0):
        seen = b""
        deadline = time.monotonic() + limit_s
        while needle not in seen and time.monotonic() < deadline:
            chunk = sock.recv(4096)
            if not chunk:
                break
            seen += chunk
        self.assertIn(needle, seen)


class DemoQuietTest(unittest.TestCase):
    def test_a_touch_opens_the_window_and_it_closes_on_its_own(self):
        quiet = dev_server.DemoQuiet(window_s=0.2)
        self.assertFalse(quiet.active())
        quiet.touch()
        self.assertTrue(quiet.active())
        time.sleep(0.3)
        self.assertFalse(quiet.active())

    def _changes(self, output_dir):
        return [(None, str(Path(output_dir, "slides.html").resolve()))]

    def test_a_rebuild_inside_the_window_does_not_reload(self):
        # What a demo's own build artifacts come back as: slides.html rewritten
        # by marp after a render nothing authored triggered.
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            Path(out, "slides.html").write_text("deck", encoding="utf-8")
            broadcaster = dev_server.ReloadBroadcaster()
            quiet = dev_server.DemoQuiet(window_s=5)
            quiet.touch()

            dev_server.handle_changes(
                self._changes(out), Path(tmp), out, broadcaster, demo_quiet=quiet
            )
            self.assertEqual(broadcaster.generation, 0)

    def test_a_rebuild_outside_the_window_reloads_as_always(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            Path(out, "slides.html").write_text("deck", encoding="utf-8")
            broadcaster = dev_server.ReloadBroadcaster()

            dev_server.handle_changes(
                self._changes(out), Path(tmp), out, broadcaster, demo_quiet=None
            )
            self.assertEqual(broadcaster.generation, 1)

    def test_running_a_demo_opens_the_window(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            demo.write(_lecture("echo hi"), out)
            quiet = dev_server.DemoQuiet(window_s=5)
            handler = dev_server.make_handler(
                out,
                dev_server.ReloadBroadcaster(),
                demo_cwd=out,
                demo_quiet=quiet,
            )
            server = ThreadingHTTPServer(("127.0.0.1", 0), handler)
            threading.Thread(target=server.serve_forever, daemon=True).start()
            self.addCleanup(server.server_close)
            self.addCleanup(server.shutdown)
            request = urllib.request.Request(
                f"http://127.0.0.1:{server.server_address[1]}{dev_server.DEMO_PATH}",
                data=json.dumps({"id": demo.demo_id("echo hi")}).encode("utf-8"),
                headers={"Content-Type": "application/json"},
                method="POST",
            )
            with closing(urllib.request.urlopen(request, timeout=20)) as resp:
                resp.read()
            self.assertTrue(quiet.active())


#: What `marp --watch` appends to the deck it builds, shortened.
_MARP_WATCH_CLIENT = (
    '<script>window.__marpCliWatchWS="ws://localhost:37717/abc";'
    '!function(){"use strict";const e=new WebSocket(x)}();</script>'
)


class StripWatchClientTest(unittest.TestCase):
    def test_marps_own_reload_client_is_removed_and_nothing_else(self):
        html = f"<html><body>deck<script>keep()</script>{_MARP_WATCH_CLIENT}</body></html>"
        stripped = marp.strip_watch_client(html)
        self.assertNotIn("__marpCliWatchWS", stripped)
        self.assertIn("keep()", stripped)
        self.assertIn("deck", stripped)

    def test_a_deck_without_one_is_unchanged(self):
        html = "<html><body>deck</body></html>"
        self.assertEqual(marp.strip_watch_client(html), html)


class DemoInjectionTest(unittest.TestCase):
    def _serve_slides(self, demo_cwd):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "slides.html").write_text(
                f"<html><body>deck{_MARP_WATCH_CLIENT}</body></html>", encoding="utf-8"
            )
            handler = dev_server.make_handler(
                root, dev_server.ReloadBroadcaster(), demo_cwd=demo_cwd
            )
            server = ThreadingHTTPServer(("127.0.0.1", 0), handler)
            threading.Thread(target=server.serve_forever, daemon=True).start()
            self.addCleanup(server.server_close)
            self.addCleanup(server.shutdown)
            port = server.server_address[1]
            with closing(
                urllib.request.urlopen(f"http://127.0.0.1:{port}/slides.html", timeout=5)
            ) as resp:
                return resp.read().decode("utf-8")

    def test_the_controller_rides_the_response_only_when_armed(self):
        with tempfile.TemporaryDirectory() as tmp:
            armed = self._serve_slides(Path(tmp))
        self.assertIn(dev_server.DEMO_PATH, armed)
        self.assertIn("lk-drawer", armed)
        self.assertIn("lk-drawer-tab", armed)  # one tab per run, styled and wired

        unarmed = self._serve_slides(None)
        self.assertNotIn("lk-drawer", unarmed)

    def test_arming_moves_the_deck_onto_our_own_reload_channel(self):
        # marp's WebSocket client reloads on any rebuild, including the one a
        # demo's build artifacts cause -- which would wipe the demo's output off
        # the screen. Armed, the deck polls our own channel, which knows better.
        with tempfile.TemporaryDirectory() as tmp:
            armed = self._serve_slides(Path(tmp))
        self.assertNotIn("__marpCliWatchWS", armed)
        self.assertIn(dev_server.LIVERELOAD_PATH, armed)

    def test_an_unarmed_deck_keeps_marps_channel_untouched(self):
        unarmed = self._serve_slides(None)
        self.assertIn("__marpCliWatchWS", unarmed)

    def test_the_terminal_rides_along_and_cannot_close_the_script_early(self):
        # xterm.js is inlined, so a `</script` anywhere in the vendored files
        # would end the block mid-library. One closing tag: the bundle's own.
        bundle = dev_server._demo_bundle()
        self.assertIn("xterm", bundle)
        self.assertIn("FitAddon", bundle)
        self.assertEqual(bundle.lower().count("</script"), 1)
        self.assertNotIn("sourceMappingURL", bundle)

    def test_injection_survives_a_body_with_no_closing_tag(self):
        self.assertIn("lk-drawer", dev_server.inject_demo("<p>no body tag</p>"))


if __name__ == "__main__":
    unittest.main()
