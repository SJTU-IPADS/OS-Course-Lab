from __future__ import annotations

from . import model


def lecture_to_dict(lecture: model.Lecture) -> dict:
    """The full, faithful AST as a JSON-ready dict (every block, with overrides)."""
    return {
        "id": lecture.id,
        "title": lecture.title,
        "subtitle": lecture.subtitle,
        "ratio": lecture.ratio,
        "lang": lecture.lang,
        "untranslated": list(lecture.untranslated),
        "borrowed": [
            {"lecture_id": entry.lecture_id, "directory": entry.directory}
            for entry in lecture.borrowed
        ],
        "children": [_node(child) for child in lecture.children],
    }


def _node(node: model.Section | model.Page) -> dict:
    if isinstance(node, model.Page):
        return {
            "type": "page",
            "id": node.id,
            "title": node.title,
            "tags": sorted(node.tags),
            "show_annotations": node.show_annotations,
            "news": [_news_item(item) for item in node.news],
            "citations": [_citation(item) for item in node.citations],
            "gap": _gap(node.gap),
            "frame_group": _frame_group(node.frame_group),
            "blocks": [_block(block) for block in node.blocks],
        }
    return {
        "type": "section",
        "id": node.id,
        "title": node.title,
        "collapsed": node.collapsed,
        "children": [_node(child) for child in node.children],
    }


def _block(block: model.Block) -> dict:
    return {
        "kind": block.kind,
        "content": block.content,
        "key": block.key,
        "untranslated": block.untranslated,
        "only": sorted(block.only) if block.only is not None else None,
        "except": sorted(block.except_) if block.except_ is not None else None,
        "disabled": block.disabled,
        "footnotes": list(block.footnotes),
        "annotations": [_annotation(note) for note in block.annotations],
        "float_image": block.float_image,
        "reveal": block.reveal,
        "autobold": block.autobold,
        "after_frames": block.after_frames,
    }


def _annotation(note: model.Annotation) -> dict:
    return {"text": note.text, "at": note.at, "dx": note.dx, "dy": note.dy}


def _gap(gap: model.PageGap | None) -> dict | None:
    if gap is None:
        return None
    return {"mode": gap.mode, "min_px": gap.min_px, "max_px": gap.max_px}


def _frame_group(group: model.FrameGroup | None) -> dict | None:
    if group is None:
        return None
    return {"id": group.id, "index": group.index, "total": group.total}


def _citation(item: model.Citation) -> dict:
    return {
        "title": item.title,
        "author": item.author,
        "year": item.year,
        "venue": item.venue,
        "url": item.url,
        "key": item.key,
    }


def _news_item(item: model.NewsItem) -> dict:
    return {
        "title": item.title,
        "url": item.url,
        "source": item.source,
        "date": item.date,
        "kind": item.kind,
        "why": item.why,
        "tags": list(item.tags),
        "image": item.image,
        "archived_url": item.archived_url,
    }
