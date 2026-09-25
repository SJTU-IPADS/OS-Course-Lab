from __future__ import annotations

import json
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Any


@dataclass
class TraceEvent:
    step: int
    event: str
    data: dict[str, Any]


class TraceRecorder:
    def __init__(self) -> None:
        self.events: list[TraceEvent] = []

    def add(self, step: int, event: str, **data: Any) -> None:
        self.events.append(TraceEvent(step=step, event=event, data=data))

    def save_jsonl(self, path: Path) -> None:
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open("w", encoding="utf-8") as handle:
            for event in self.events:
                handle.write(json.dumps(asdict(event), ensure_ascii=False) + "\n")
