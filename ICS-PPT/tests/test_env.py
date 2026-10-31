"""The project's .env: read for the demos, below the process environment."""

from pathlib import Path

from lecturekit import demo, env


def _write(root: Path, text: str) -> None:
    (root / ".env").write_text(text, encoding="utf-8")


def test_no_file_is_no_variables(tmp_path):
    assert env.read(tmp_path) == {}


def test_the_usual_syntax_is_read(tmp_path):
    _write(tmp_path, '# a comment\n\nA=1\nexport B="two words"\nC=\'$A\'\nD=$A\nE\n')
    assert env.read(tmp_path) == {"A": "1", "B": "two words", "C": "$A", "D": "$A"}


def test_the_file_is_added_to_the_environment(tmp_path):
    _write(tmp_path, "TOKEN=abc\n")
    result = env.demo_environment(tmp_path.as_posix(), {"PATH": "/bin"})
    assert result == {"PATH": "/bin", "TOKEN": "abc", "LECTUREKIT_ROOT": tmp_path.as_posix()}


def test_a_variable_in_the_environment_wins_over_the_file(tmp_path):
    _write(tmp_path, "TOKEN=from-file\n")
    result = env.demo_environment(tmp_path.as_posix(), {"TOKEN": "from-shell"})
    assert result["TOKEN"] == "from-shell"


def test_the_file_cannot_move_the_project_root(tmp_path):
    _write(tmp_path, "LECTUREKIT_ROOT=/elsewhere\n")
    assert env.demo_environment(tmp_path.as_posix(), {})["LECTUREKIT_ROOT"] == tmp_path.as_posix()


def test_a_demo_sees_the_variables_of_the_file(tmp_path, monkeypatch):
    _write(tmp_path, "LK_TEST_TOKEN=s3cret\n")
    monkeypatch.setenv("LECTUREKIT_ROOT", tmp_path.as_posix())
    monkeypatch.delenv("LK_TEST_TOKEN", raising=False)
    assert demo.run('echo "$LK_TEST_TOKEN"', cwd=tmp_path).output == "s3cret\n"

