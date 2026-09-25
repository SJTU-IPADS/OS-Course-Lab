import importlib.util
import os
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory


ROOT = Path(__file__).resolve().parent
ANSWER_PATH = ROOT / "tool-use.py"


def load_module(module_name: str, file_path: Path):
    os.environ.setdefault("OPENROUTER_API_KEY", "test-key")
    spec = importlib.util.spec_from_file_location(module_name, file_path)
    if spec is None:
        raise ImportError(f"Could not load module {module_name} from {file_path}")
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module


class ToolUseAnswerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.answer = load_module("tool_use_answer", ANSWER_PATH)

    def setUp(self):
        self.tempdir = TemporaryDirectory()
        self.workdir = Path(self.tempdir.name)
        self.original_answer_workdir = getattr(self.answer, "WORKDIR")
        setattr(self.answer, "WORKDIR", self.workdir)

    def tearDown(self):
        setattr(self.answer, "WORKDIR", self.original_answer_workdir)
        self.tempdir.cleanup()

    def test_answer_exposes_the_expected_dispatch_surface(self):
        self.assertEqual(
            set(self.answer.TOOL_HANDLERS),
            {"bash", "read_file", "write_file", "edit_file"},
        )

    def test_answer_declares_the_expected_tool_definitions(self):
        answer_schemas = {
            tool["function"]["name"]: tool["function"] for tool in self.answer.TOOLS
        }

        self.assertEqual(
            set(answer_schemas),
            {"bash", "read_file", "write_file", "edit_file"},
        )
        self.assertEqual(set(answer_schemas), set(self.answer.TOOL_HANDLERS))

        expected_required = {
            "bash": ["command"],
            "read_file": ["path"],
            "write_file": ["path", "content"],
            "edit_file": ["path", "old_text", "new_text"],
        }
        for name, required in expected_required.items():
            with self.subTest(tool=name):
                self.assertEqual(answer_schemas[name]["parameters"]["required"], required)

    def test_run_write_and_run_read_round_trip(self):
        result = self.answer.run_write("nested/demo.txt", "alpha\nbeta")
        self.assertEqual(result, "Wrote 10 bytes to nested/demo.txt")
        self.assertEqual((self.workdir / "nested" / "demo.txt").read_text(), "alpha\nbeta")
        self.assertEqual(self.answer.run_read("nested/demo.txt"), "alpha\nbeta")

    def test_run_read_applies_line_limit(self):
        self.answer.run_write("lines.txt", "one\ntwo\nthree\nfour")
        self.assertEqual(
            self.answer.run_read("lines.txt", limit=2),
            "one\ntwo\n... (2 more lines)",
        )

    def test_run_write_rejects_workspace_escape(self):
        result = self.answer.run_write("../escape.txt", "blocked")
        self.assertIn("Path escapes workspace", result)
        self.assertFalse((self.workdir.parent / "escape.txt").exists())

    def test_run_edit_replaces_only_the_first_match(self):
        self.answer.run_write("edit.txt", "foo bar foo")
        result = self.answer.run_edit("edit.txt", "foo", "baz")
        self.assertEqual(result, "Edited edit.txt")
        self.assertEqual((self.workdir / "edit.txt").read_text(), "baz bar foo")

    def test_run_edit_reports_missing_text(self):
        self.answer.run_write("edit.txt", "hello world")
        result = self.answer.run_edit("edit.txt", "goodbye", "hi")
        self.assertEqual(result, "Error: Text not found in edit.txt")
        self.assertEqual((self.workdir / "edit.txt").read_text(), "hello world")

    def test_tool_handlers_dispatch_to_the_added_file_tools(self):
        write_result = self.answer.TOOL_HANDLERS["write_file"](
            path="dispatch.txt",
            content="hello",
        )
        edit_result = self.answer.TOOL_HANDLERS["edit_file"](
            path="dispatch.txt",
            old_text="hello",
            new_text="world",
        )
        read_result = self.answer.TOOL_HANDLERS["read_file"](
            path="dispatch.txt",
            limit=1,
        )

        self.assertEqual(write_result, "Wrote 5 bytes to dispatch.txt")
        self.assertEqual(edit_result, "Edited dispatch.txt")
        self.assertEqual(read_result, "world")


if __name__ == "__main__":
    unittest.main()
