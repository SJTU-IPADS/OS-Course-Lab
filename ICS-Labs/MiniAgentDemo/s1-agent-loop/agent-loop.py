#!/usr/bin/env python3
# Harness: the loop -- the model's first connection to the real world.
"""
s01_agent_loop.py - The Agent Loop

The entire secret of an AI coding agent in one pattern:

    while stop_reason == "tool_use":
        response = LLM(messages, tools)
        execute tools
        append results

    +----------+      +-------+      +---------+
    |   User   | ---> |  LLM  | ---> |  Tool   |
    |  prompt  |      |       |      | execute |
    +----------+      +---+---+      +----+----+
                          ^               |
                          |   tool_result |
                          +---------------+
                          (loop continues)

This is the core loop: feed tool results back to the model
until the model decides to stop. Production agents layer
policy, hooks, and lifecycle controls on top.
"""

import json
import os
import subprocess
from pathlib import Path

from dotenv import load_dotenv
from openai import APIStatusError, OpenAI

# Load env from project and local script directory; local .env can override defaults.
load_dotenv(override=True)
load_dotenv(Path(__file__).with_name(".env"), override=True)

WORKDIR = Path.cwd()
MODEL = os.getenv("MODEL_ID", "tencent/hy3-preview:free")
FALLBACK_MODEL = os.getenv("OPENROUTER_FALLBACK_MODEL", "openrouter/free")

client = OpenAI(
    base_url=os.getenv("OPENROUTER_BASE_URL", "https://openrouter.ai/api/v1"),
    api_key=os.environ["OPENROUTER_API_KEY"],
)

SYSTEM = (
    f"You are a coding agent at {WORKDIR}. "
    "Use bash to solve tasks on Linux. "
    "Write and edit files with standard POSIX shell commands. "
    "Act, don't explain."
)

TOOLS = [{
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
}]


def run_bash(command: str) -> str:
    # Lightweight denylist to block obviously destructive commands in demos.
    dangerous = ["rm -rf /", "sudo", "shutdown", "reboot", "> /dev/"]
    if any(d in command for d in dangerous):
        return "Error: Dangerous command blocked"
    try:
        # TODO: run command using subprocess.run, add timeout for 120s and limit output length
        pass # TODO: delete this line after you implement TODO above
    except subprocess.TimeoutExpired:
        return "Error: Timeout (120s)"
    except (FileNotFoundError, OSError) as e:
        return f"Error: {e}"


def get_completion(messages: list):
    # Always prepend the system instruction and include the tool schema.
    request = {
        "model": MODEL,
        "messages": [{"role": "system", "content": SYSTEM}, *messages],
        "tools": TOOLS,
    }
    try:
        return client.chat.completions.create(**request)
    except APIStatusError as error:
        # If the selected model is unavailable on this route, retry once with fallback.
        if error.status_code != 404 or MODEL == FALLBACK_MODEL:
            raise
        print(f"Model {MODEL} unavailable on current OpenRouter route, falling back to {FALLBACK_MODEL}.")
        request["model"] = FALLBACK_MODEL
        return client.chat.completions.create(**request)


# -- The core pattern: a while loop that calls tools until the model stops --
def agent_loop(messages: list):
    while True:
        # 1) Ask model what to do next.
        # TODO: Call the LLM and extract the assistant message
        assistant_record = {} # TODO: configure assistant_record
        # 2) Record tool calls in the conversation transcript.
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

        # Stop when the model no longer asks for tools.
        if not tool_calls:
            return
        # 3) Execute requested tools and feed outputs back as tool_result.
        for tool_call in tool_calls:
            if tool_call.function.name == "bash":
                arguments = json.loads(tool_call.function.arguments)
                command = arguments["command"]
                print(f"[LOG]: Tool call: {command}")
                output = run_bash(command)
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
            query = input("\033[36ms1 >> \033[0m")
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
