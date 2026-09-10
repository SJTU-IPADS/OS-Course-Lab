from __future__ import annotations

import argparse
import os
import shutil
import sys
import textwrap
from pathlib import Path

from prompt_toolkit.application import Application
from prompt_toolkit.formatted_text import HTML
from prompt_toolkit.key_binding import KeyBindings
from prompt_toolkit.layout import Layout
from prompt_toolkit.layout.dimension import Dimension
from prompt_toolkit.styles import Style
from prompt_toolkit.widgets import Box, TextArea

from .runtime import RuntimeConfig, build_runtime

RESET = "\033[0m"
BOLD = "\033[1m"
DIM = "\033[2m"
CYAN = "\033[36m"
GREEN = "\033[32m"
MAGENTA = "\033[35m"


def _configure_utf8_stdio() -> None:
    """Prefer UTF-8 streams so Chinese prompts survive redirected terminals."""
    for stream_name in ("stdin", "stdout", "stderr"):
        stream = getattr(sys, stream_name)
        reconfigure = getattr(stream, "reconfigure", None)
        if reconfigure is None:
            continue
        try:
            reconfigure(encoding="utf-8", errors="replace")
        except (OSError, ValueError):
            pass


def _enable_windows_ansi() -> None:
    if os.name != "nt":
        return
    try:
        import ctypes
    except ImportError:
        return

    kernel32 = ctypes.windll.kernel32
    handle = kernel32.GetStdHandle(-11)
    mode = ctypes.c_uint()
    if not kernel32.GetConsoleMode(handle, ctypes.byref(mode)):
        return
    kernel32.SetConsoleMode(handle, mode.value | 0x0004)


def _replace_input(text_area: TextArea, text: str) -> None:
    text_area.buffer.text = text
    text_area.buffer.cursor_position = len(text)


def _input_key_bindings(text_area: TextArea, history: list[str]) -> KeyBindings:
    bindings = KeyBindings()
    state: dict[str, int | str | None] = {
        "index": len(history),
        "draft": None,
    }

    @bindings.add("enter")
    def _(event) -> None:
        event.app.exit(result=text_area.text)

    @bindings.add("escape", "enter")
    def _(event) -> None:
        text_area.buffer.insert_text("\n")

    @bindings.add("up")
    def _(event) -> None:
        buffer = text_area.buffer
        document = buffer.document
        if document.line_count == 1 or document.cursor_position_row == 0:
            if not history:
                return
            if state["index"] == len(history):
                state["draft"] = text_area.text
            state["index"] = max(0, int(state["index"]) - 1)
            _replace_input(text_area, history[int(state["index"])])
        else:
            buffer.cursor_up()

    @bindings.add("down")
    def _(event) -> None:
        buffer = text_area.buffer
        document = buffer.document
        if (
            document.line_count == 1
            or document.cursor_position_row >= document.line_count - 1
        ):
            if not history:
                return
            index = int(state["index"])
            if index < len(history) - 1:
                state["index"] = index + 1
                _replace_input(text_area, history[int(state["index"])])
            elif index < len(history):
                state["index"] = len(history)
                _replace_input(text_area, str(state["draft"] or ""))
        else:
            buffer.cursor_down()

    @bindings.add("c-c")
    def _(event) -> None:
        event.app.exit(exception=KeyboardInterrupt)

    @bindings.add("c-d")
    def _(event) -> None:
        if text_area.text:
            event.app.exit(result=text_area.text)
        else:
            event.app.exit(exception=EOFError)

    return bindings


class Console:
    def __init__(self) -> None:
        _configure_utf8_stdio()
        _enable_windows_ansi()
        self.use_color = (
            sys.stdout.isatty()
            and os.environ.get("NO_COLOR") is None
            and os.environ.get("TERM") != "dumb"
        )
        self.width = shutil.get_terminal_size((88, 24)).columns
        self.history: list[str] = []

    def style(self, text: str, *codes: str) -> str:
        if not self.use_color:
            return text
        return "".join(codes) + text + RESET

    def rule(self) -> None:
        print(self.style("─" * self.width, DIM))

    def banner(self, *, model: str | None, workspace: Path, trace: Path) -> None:
        if not sys.stdout.isatty():
            return
        title = self.style("ICS Agent Lab", BOLD, CYAN)
        subtitle = self.style("交互式 JSON 工具调用实验台", DIM)
        print()
        self.rule()
        print(f"{title}  {subtitle}")
        print(
            self.style("model", MAGENTA)
            + f" {model or 'default'}  "
            + self.style("workspace", MAGENTA)
            + f" {workspace}  "
            + self.style("trace", MAGENTA)
            + f" {trace}"
        )
        print(self.style("输入 /help 查看命令，/exit 退出。支持中文 UTF-8 输入。", DIM))
        print(self.style("Enter 发送，Esc+Enter 换行；粘贴多行会作为同一条消息。", DIM))
        self.rule()

    def prompt(self) -> str:
        if not sys.stdin.isatty():
            marker = self.style("❯", BOLD, CYAN)
            label = self.style(" 你", BOLD)
            return input(f"\n{marker}{label} ")

        print()
        text_area = TextArea(
            multiline=True,
            wrap_lines=True,
            height=Dimension(min=1, max=8),
            dont_extend_height=True,
            prompt=HTML(
                "<prompt.marker>❯</prompt.marker>" "<prompt.label> 你 </prompt.label>"
            ),
            style="class:input-box",
        )
        app = Application(
            layout=Layout(
                Box(
                    body=text_area,
                    padding_left=1,
                    padding_right=1,
                    padding_top=1,
                    padding_bottom=1,
                    style="class:input-box",
                )
            ),
            key_bindings=_input_key_bindings(text_area, self.history),
            style=self._prompt_style(),
            full_screen=False,
        )
        query = app.run() or ""
        if query.strip() and (not self.history or self.history[-1] != query):
            self.history.append(query)
        print()
        return query

    def assistant(self, text: str) -> None:
        print()
        print(self.style("● agent", BOLD, GREEN))
        print(self._indent_wrapped(text.rstrip()))

    def info(self, text: str) -> None:
        print(self.style(text, DIM))

    def trace_saved(self, path: Path) -> None:
        self.info(f"trace saved: {path}")

    def help(self) -> None:
        print()
        print(self.style("可用命令", BOLD, CYAN))
        print("  /help        显示帮助")
        print("  /new         开始新的 agent session")
        print("  /exit        退出交互模式")
        print("  /quit        退出交互模式")
        print("  Esc+Enter    在输入框里插入换行")

    def _prompt_style(self) -> Style:
        if not self.use_color:
            return Style([])
        return Style.from_dict(
            {
                "input-box": "bg:#2b2b2b #d4d4d4",
                "prompt.marker": "bg:#2b2b2b #67e8f9 bold",
                "prompt.label": "bg:#2b2b2b #d4d4d4 bold",
            }
        )

    def _indent_wrapped(self, text: str) -> str:
        if not text:
            return self.style("(empty response)", DIM)

        wrapper = textwrap.TextWrapper(
            width=max(40, min(self.width, 120) - 4),
            initial_indent="  ",
            subsequent_indent="  ",
            replace_whitespace=False,
            drop_whitespace=False,
        )
        lines: list[str] = []
        for paragraph in text.splitlines():
            if not paragraph:
                lines.append("")
                continue
            lines.extend(wrapper.wrap(paragraph) or ["  "])
        return "\n".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser(description="Run the ICS manual JSON agent lab.")
    parser.add_argument("prompt", nargs="*")
    parser.add_argument("--provider", choices=["openrouter"], default="openrouter")
    parser.add_argument("--model", default=None)
    parser.add_argument("--seed", type=int, default=None)
    parser.add_argument("--temperature", type=float, default=None)
    parser.add_argument("--workspace", default="workspace")
    parser.add_argument("--skills-dir", default="skills")
    parser.add_argument("--trace", default="traces/latest.jsonl")
    args = parser.parse_args()

    console = Console()
    workspace = Path(args.workspace)
    skills_dir = Path(args.skills_dir)
    trace = Path(args.trace)

    runtime = build_runtime(
        RuntimeConfig(
            provider=args.provider,
            workspace=workspace,
            skills_dir=skills_dir,
            model=args.model,
            seed=args.seed,
            temperature=args.temperature,
        )
    )
    runtime.ensure_workspace()
    agent = runtime.agent
    if args.prompt:
        answer = agent.run(" ".join(args.prompt))
        agent.save_trace(trace)
        if sys.stdout.isatty():
            console.assistant(answer)
            console.trace_saved(trace)
        else:
            print(answer)
        return

    console.banner(model=args.model, workspace=workspace, trace=trace)
    messages = agent.new_session()
    while True:
        try:
            query = console.prompt()
        except (EOFError, KeyboardInterrupt):
            print()
            break

        command = query.strip().lower()
        if command in {"/quit", "/exit"}:
            break
        if command == "/help":
            console.help()
            continue
        if command == "/new":
            messages = agent.new_session()
            console.info("new session started")
            continue
        if not query.strip():
            continue

        answer = agent.run_turn(messages, query)
        agent.save_trace(trace)
        console.assistant(answer)
        console.trace_saved(trace)


if __name__ == "__main__":
    main()
