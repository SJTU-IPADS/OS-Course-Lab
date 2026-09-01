from .viewer import StaticViewerRenderer

__all__ = [
    "RENDERERS", "StaticViewerRenderer", "PptxRenderer", "TranscriptRenderer",
    "get_renderer",
]


def _load_viewer():
    return StaticViewerRenderer


def _load_transcript():
    from .transcript import TranscriptRenderer

    return TranscriptRenderer


def _load_pptx():
    # Imported lazily: the pptx renderer needs the third-party ``python-pptx``
    # package, which only the pptx export target requires. Importing it at
    # package-import time would force ``python-pptx`` onto every render path —
    # including the viewer and the notebook inline renderer, which don't use it.
    from .pptx import PptxRenderer

    return PptxRenderer


# Known render targets -> lazy loaders. Keeping this a mapping preserves
# ``"name" in RENDERERS`` membership checks without importing optional backends
# (so importing this package never requires ``python-pptx``).
RENDERERS = {
    "viewer": _load_viewer, "pptx": _load_pptx, "transcript": _load_transcript,
}


def get_renderer(name: str):
    from lecturekit.model import ValidationError

    try:
        loader = RENDERERS[name]
    except KeyError:
        raise ValidationError(f"Unknown render target: {name}")
    return loader()


def __getattr__(name: str):
    # Lazy attribute access so ``from lecturekit.renderers import PptxRenderer``
    # (advertised in ``__all__``) still works, without importing python-pptx
    # until the name is actually used.
    if name == "PptxRenderer":
        return _load_pptx()
    if name == "TranscriptRenderer":
        return _load_transcript()
    raise AttributeError(f"module {__name__!r} has no attribute {name!r}")
