from __future__ import annotations

import argparse
import json
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Any, Iterable

from ics_agent_lab.runtime import RuntimeConfig

from .runner import EvalResult, run_scenario
from .scenario import EvalScenario


@dataclass(frozen=True)
class GradeCase:
    name: str
    path: str
    points: int


@dataclass
class GradeCaseResult:
    case: GradeCase
    passed: bool
    failures: list[str]
    result: EvalResult | None = None
    error: str | None = None

    @property
    def earned_points(self) -> int:
        return self.case.points if self.passed else 0

    def to_dict(self) -> dict[str, Any]:
        payload = asdict(self)
        payload["earned_points"] = self.earned_points
        if self.result is not None:
            payload["result"] = self.result.to_dict()
        return payload


@dataclass
class GradeReport:
    results: list[GradeCaseResult]

    @property
    def score(self) -> int:
        return sum(result.earned_points for result in self.results)

    @property
    def max_score(self) -> int:
        return sum(result.case.points for result in self.results)

    @property
    def passed(self) -> bool:
        return all(result.passed for result in self.results)

    @property
    def passed_count(self) -> int:
        return sum(1 for result in self.results if result.passed)

    def to_dict(self) -> dict[str, Any]:
        return {
            "score": self.score,
            "max_score": self.max_score,
            "passed": self.passed,
            "passed_count": self.passed_count,
            "total_count": len(self.results),
            "results": [result.to_dict() for result in self.results],
        }

    def to_json(self) -> str:
        return json.dumps(self.to_dict(), ensure_ascii=False, indent=2)


DEFAULT_GRADE_CASES: tuple[GradeCase, ...] = (
    GradeCase("read_file_efficiency", "evals/read_file_efficiency.json", 8),
    GradeCase("list_files_nested", "evals/list_files_nested.json", 8),
    GradeCase("edit_file_replace", "evals/edit_file_replace.json", 8),
    GradeCase("bash_workspace", "evals/bash_workspace.json", 8),
    GradeCase("data_redaction", "assignments/data_redaction/eval.json", 12),
    GradeCase("ephemeral_dispatch", "assignments/ephemeral_dispatch/eval.json", 12),
    GradeCase("patch_review", "assignments/patch_review/eval.json", 12),
    GradeCase("memory_persistent_recall", "evals/memory_persistent_recall.json", 8),
    GradeCase("memory_persistent_update", "evals/memory_persistent_update.json", 7),
)


def run_grade_suite(
    runtime_config: RuntimeConfig,
    *,
    cases: Iterable[GradeCase] = DEFAULT_GRADE_CASES,
    keep_workspace: bool = True,
) -> GradeReport:
    results: list[GradeCaseResult] = []
    for case in cases:
        try:
            scenario = EvalScenario.from_file(Path(case.path))
            result = run_scenario(
                scenario,
                runtime_config,
                keep_workspace=keep_workspace,
            )
            results.append(
                GradeCaseResult(
                    case=case,
                    passed=result.passed,
                    failures=result.failures,
                    result=result,
                )
            )
        except Exception as exc:
            results.append(
                GradeCaseResult(
                    case=case,
                    passed=False,
                    failures=[f"{type(exc).__name__}: {exc}"],
                    error=str(exc),
                )
            )
    return GradeReport(results=results)


def format_text_report(report: GradeReport) -> str:
    lines = [
        "ICS Agent Lab grade report",
        f"Score: {report.score}/{report.max_score}",
        f"Passed: {report.passed_count}/{len(report.results)}",
        "",
        "CASES",
    ]
    for result in report.results:
        status = "PASS" if result.passed else "FAIL"
        lines.append(
            f"[{status}] {result.case.name:<22} "
            f"{result.earned_points:>3}/{result.case.points:<3} "
            f"{result.case.path}"
        )

    failed = [result for result in report.results if not result.passed]
    if failed:
        lines.extend(["", "FAILED CASES"])
        for result in failed:
            lines.append(
                f"- {result.case.name} "
                f"({result.earned_points}/{result.case.points})"
            )
            for failure in result.failures:
                lines.append(f"  - {failure}")
            if result.result is not None:
                lines.append(f"  - workspace: {result.result.workspace}")

    return "\n".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Run all ICS Agent Lab evals and score them."
    )
    parser.add_argument("--provider", choices=["openrouter"], default="openrouter")
    parser.add_argument("--model", default=None)
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--temperature", type=float, default=0.0)
    parser.add_argument("--extra-tool-dir", action="append", default=[])
    parser.add_argument(
        "--keep-workspace",
        action=argparse.BooleanOptionalAction,
        default=True,
    )
    parser.add_argument(
        "--json",
        action="store_true",
        help="Print machine-readable JSON.",
    )
    args = parser.parse_args()

    report = run_grade_suite(
        RuntimeConfig(
            provider=args.provider,
            model=args.model,
            seed=args.seed,
            temperature=args.temperature,
            extra_tool_dirs=tuple(Path(path) for path in args.extra_tool_dir),
        ),
        keep_workspace=args.keep_workspace,
    )
    print(report.to_json() if args.json else format_text_report(report))
    raise SystemExit(0 if report.passed else 1)


if __name__ == "__main__":
    main()
