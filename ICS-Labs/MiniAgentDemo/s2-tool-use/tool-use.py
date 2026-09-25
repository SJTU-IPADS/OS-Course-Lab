#!/usr/bin/env python3
# Harness: tool dispatch -- expanding what the model can reach.
"""
s02_tool_use.py - Tools

The agent loop from s01 didn't change. We just added tools to the array
and a dispatch map to route calls.

    +----------+      +-------+      +------------------+
    |   User   | ---> |  LLM  | ---> | Tool Dispatch    |
    |  prompt  |      |       |      | {                |
    +----------+      +---+---+      |   bash: run_bash |
                          ^          |   read: run_read |
                          |          |   write: run_wr  |
                          +----------+   edit: run_edit |
                          tool_result| }                |
                                     +------------------+

Key insight: "The loop didn't change at all. I just added tools."
"""

import json
import os
import subprocess
from pathlib import Path

from dotenv import load_dotenv
from openai import APIStatusError, OpenAI

load_dotenv(override=True)

WORKDIR = Path.cwd()
MODEL = os.getenv("MODEL_ID", "tencent/hy3-preview:free")
FALLBACK_MODEL = os.getenv("OPENROUTER_FALLBACK_MODEL", "openrouter/free")

client = OpenAI(
    base_url=os.getenv("OPENROUTER_BASE_URL", "https://openrouter.ai/api/v1"),
    api_key=os.environ["OPENROUTER_API_KEY"],
)

SYSTEM = f"You are a coding agent at {WORKDIR}. Use tools to solve tasks. Act, don't explain."


def safe_path(p: str) -> Path:
    path = (WORKDIR / p).resolve()
    if not path.is_relative_to(WORKDIR):
        raise ValueError(f"Path escapes workspace: {p}")
    return path


def run_bash(command: str) -> str:
    dangerous = ["rm -rf /", "sudo", "shutdown", "reboot", "> /dev/"]
    if any(d in command for d in dangerous):
        return "Error: Dangerous command blocked"
    try:
        print(f"[LOG]: Running bash command: {command}")
        r = subprocess.run(command, shell=True, cwd=WORKDIR,
                           capture_output=True, text=True, timeout=120)
        out = (r.stdout + r.stderr).strip()
        return out[:50000] if out else "(no output)"
    except subprocess.TimeoutExpired:
        return "Error: Timeout (120s)"


def run_read(path: str, limit: int | None = None) -> str:
    # TODO
    return "Error: read_file not implemented yet"


def run_write(path: str, content: str) -> str:
    # TODO
    return "Error: write_file not implemented yet"


def run_edit(path: str, old_text: str, new_text: str) -> str:
    # TODO
    return "Error: edit_file not implemented yet"


# -- The dispatch map: {tool_name: handler} --
TOOL_HANDLERS = {
    "bash":       lambda **kw: run_bash(kw["command"]),
    "read_file":  lambda **kw: run_read(kw["path"], kw.get("limit")),
    "write_file": lambda **kw: run_write(kw["path"], kw["content"]),
    "edit_file":  lambda **kw: run_edit(kw["path"], kw["old_text"], kw["new_text"]),
}

TOOLS = [
    {
        "type": "function",
        "function": {
            "name": "bash",
            "description": "Run a shell command.",
            "parameters": {
                "type": "object",
                "properties": {"command": {"type": "string"}},
                "required": ["command"],
            },
        },
    }
    # TODO: add read_file, write_file, edit_file tool definitions here
]


def get_completion(messages: list):
    request = {
        "model": MODEL,
        "messages": [{"role": "system", "content": SYSTEM}, *messages],
        "tools": TOOLS,
    }
    try:
        return client.chat.completions.create(**request)
    except APIStatusError as error:
        if error.status_code != 404 or MODEL == FALLBACK_MODEL:
            raise
        print(f"Model {MODEL} unavailable on current OpenRouter route, falling back to {FALLBACK_MODEL}.")
        request["model"] = FALLBACK_MODEL
        return client.chat.completions.create(**request)


def agent_loop(messages: list):
    while True:
        response = get_completion(messages)
        assistant_message = response.choices[0].message
        tool_calls = assistant_message.tool_calls or []

        assistant_record = {"role": "assistant", "content": assistant_message.content or ""}
        if tool_calls:
            assistant_record["tool_calls"] = [
                {
                    "id": tool_call.id,
                    "type": "function",
                    "function": {
                        "name": tool_call.function.name,
                        "arguments": tool_call.function.arguments,
                    },
                }
                for tool_call in tool_calls
            ]
        messages.append(assistant_record)

        if not tool_calls:
            return

        for tool_call in tool_calls:
            name = tool_call.function.name
            handler = TOOL_HANDLERS.get(name)
            try:
                arguments = json.loads(tool_call.function.arguments)
            except json.JSONDecodeError:
                arguments = {}
            output = handler(**arguments) if handler else f"Unknown tool: {name}"
            output = str(output)
            print(f"[LOG]: Tool call: {name}")
            print(output[:200])
            messages.append({
                "role": "tool",
                "tool_call_id": tool_call.id,
                "content": output,
            })


if __name__ == "__main__":
    history = []
    while True:
        try:
            query = input("\033[36ms2 >> \033[0m")
        except (EOFError, KeyboardInterrupt):
            break
        if query.strip().lower() in ("q", "exit", ""):
            break
        history.append({"role": "user", "content": query})
        try:
            agent_loop(history)
        except RuntimeError as error:
            print(f"[ERROR] {error}")
            continue
        response_content = history[-1].get("content")
        if isinstance(response_content, str) and response_content.strip():
            print(response_content)
        print()
