"""The viewer / notebook render path must not require the optional python-pptx.

pptx export is an optional target (it needs the third-party ``python-pptx``),
so importing the renderers package — which importing ``lecturekit.notebook`` or
any ``lecturekit.renderers.viewer`` submodule triggers — must work in an
environment where ``python-pptx`` is not installed. These tests simulate that
environment by blocking the top-level ``pptx`` module.
"""

import importlib
import sys


def _reimport_without_pptx(monkeypatch):
    # `None` in sys.modules makes `import pptx` raise ModuleNotFoundError, exactly
    # as if python-pptx were not installed.
    monkeypatch.setitem(sys.modules, "pptx", None)
    for name in list(sys.modules):
        if name.startswith("lecturekit.renderers") or name == "lecturekit.notebook":
            monkeypatch.delitem(sys.modules, name, raising=False)


def test_renderers_import_without_pptx(monkeypatch):
    _reimport_without_pptx(monkeypatch)
    renderers = importlib.import_module("lecturekit.renderers")
    assert renderers.get_renderer("viewer").__name__ == "StaticViewerRenderer"


def test_notebook_import_without_pptx(monkeypatch):
    _reimport_without_pptx(monkeypatch)
    # Must not raise ModuleNotFoundError: No module named 'pptx'.
    importlib.import_module("lecturekit.notebook")
