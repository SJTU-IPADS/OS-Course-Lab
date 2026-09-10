from __future__ import annotations

DIFF_TEXT = """diff --git a/ics_agent_lab/tools/read_file.py b/ics_agent_lab/tools/read_file.py
new file mode 100644
--- /dev/null
+++ b/ics_agent_lab/tools/read_file.py
@@
+from pathlib import Path
+
+from ics_agent_lab.tools import Tool, Workspace, json_result
+
+
+def make_tool(workspace: Workspace) -> Tool:
+    def handler(arguments):
+        path = Path(arguments["path"])
+        content = path.read_text(encoding="utf-8")
+        return json_result(ok=True, content=content)
+
+    return Tool(
+        name="read_file",
+        description="Read a file.",
+        schema={
+            "type": "object",
+            "required": ["path"],
+            "properties": {"path": {"type": "string"}},
+        },
+        handler=handler,
+    )
"""

FILES = {
    "ics_agent_lab/tools/read_file.py": (
        "from pathlib import Path\n\n"
        "from ics_agent_lab.tools import Tool, Workspace, json_result\n\n\n"
        "def make_tool(workspace: Workspace) -> Tool:\n"
        "    def handler(arguments):\n"
        '        path = Path(arguments["path"])\n'
        '        content = path.read_text(encoding="utf-8")\n'
        "        return json_result(ok=True, content=content)\n\n"
        "    return Tool(\n"
        '        name="read_file",\n'
        '        description="Read a file.",\n'
        "        schema={\n"
        '            "type": "object",\n'
        '            "required": ["path"],\n'
        '            "properties": {"path": {"type": "string"}},\n'
        "        },\n"
        "        handler=handler,\n"
        "    )\n"
    )
}

_submitted_review: dict[str, str] | None = None


def read_diff() -> str:
    return DIFF_TEXT


def read_patch_file(path: str) -> str | None:
    return FILES.get(path)


def submit_review(verdict: str, comments: str) -> str:
    text = f"{verdict}\n{comments}".lower()
    if verdict != "request_changes":
        return "rejected: verdict should request changes"
    if "path traversal" not in text and "workspace" not in text:
        return "rejected: review misses workspace path traversal risk"
    if "workspace.resolve" not in text:
        return "rejected: review should recommend workspace.resolve"
    if "test" not in text:
        return "rejected: review should mention a regression test"

    global _submitted_review
    _submitted_review = {
        "verdict": verdict,
        "comments": comments,
    }
    return "REVIEW SUBMITTED"


def submitted_review() -> dict[str, str] | None:
    return _submitted_review
