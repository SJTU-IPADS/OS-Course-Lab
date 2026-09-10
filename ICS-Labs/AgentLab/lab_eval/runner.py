from __future__ import annotations

import json
import shutil
import tempfile
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Any

from ics_agent_lab.runtime import RuntimeConfig, build_llm, build_runtime

from .metrics import EvalMetrics, MeasuringTransport
from .scenario import EvalScenario


@dataclass
class EvalResult:
    scenario: str
    passed: bool
    answer: str
    metrics: EvalMetrics
    trace_summary: dict[str, int]
    failures: list[str]
    workspace: str

    def to_dict(self) -> dict[str, Any]:
        data = asdict(self)
        data["metrics"] = self.metrics.to_dict()
        return data

    def to_json(self) -> str:
        return json.dumps(self.to_dict(), ensure_ascii=False, indent=2)


def run_scenario(
    scenario: EvalScenario,
    runtime_config: RuntimeConfig,
    *,
    keep_workspace: bool = False,
) -> EvalResult:
    tempdir = None
    if keep_workspace:
        root = Path(tempfile.mkdtemp(prefix=f"ics-agent-eval-{scenario.name}-"))
    else:
        tempdir = tempfile.TemporaryDirectory(prefix=f"ics-agent-eval-{scenario.name}-")
        root = Path(tempdir.name)
    workspace = root / "workspace"
    skills_dir = root / "skills"
    write_files(workspace, scenario.workspace_files)
    write_files(skills_dir, scenario.skill_files)
    copy_skill_dirs(skills_dir, scenario.skill_source_dirs)

    config = RuntimeConfig(
        provider=runtime_config.provider,
        workspace=workspace,
        skills_dir=skills_dir,
        model=runtime_config.model,
        seed=runtime_config.seed,
        temperature=runtime_config.temperature,
        subagent_max_steps=runtime_config.subagent_max_steps,
        extra_tool_dirs=runtime_config.extra_tool_dirs + scenario.extra_tool_dirs,
    )
    measuring_llm = MeasuringTransport(build_llm(config))
    runtime = build_runtime(config, llm=measuring_llm)
    runtime.ensure_workspace()
    apply_agent_config(runtime.agent.config, scenario.agent_config)

    answer = run_agent_scenario(runtime.agent, scenario)
    runtime.agent.save_trace(Path("traces/latest.jsonl"))
    trace_summary = summarize_trace(runtime.agent.trace.events)
    failures = check_expectations(
        scenario,
        answer,
        workspace,
        measuring_llm.metrics,
        trace_summary,
        runtime.agent.trace.events,
    )
    result = EvalResult(
        scenario=scenario.name,
        passed=not failures,
        answer=answer,
        metrics=measuring_llm.metrics,
        trace_summary=trace_summary,
        failures=failures,
        workspace=str(workspace),
    )

    if tempdir is not None:
        tempdir.cleanup()
    return result


def write_files(root: Path, files: dict[str, str]) -> None:
    for relative_path, content in files.items():
        path = (root / relative_path).resolve()
        if root.resolve() not in path.parents and path != root.resolve():
            raise ValueError(f"Fixture path escapes root: {relative_path}")
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")


def apply_agent_config(agent_config, overrides: dict[str, Any]) -> None:
    for key, value in overrides.items():
        if not hasattr(agent_config, key):
            raise ValueError(f"Unknown agent_config field: {key}")
        setattr(agent_config, key, value)


def run_agent_scenario(agent, scenario: EvalScenario) -> str:
    answer = ""
    if scenario.sessions:
        for session in scenario.sessions:
            messages = agent.new_session()
            for turn in session:
                answer = agent.run_turn(messages, turn)
        return answer

    messages = agent.new_session()
    for turn in [scenario.prompt, *scenario.turns]:
        answer = agent.run_turn(messages, turn)
    return answer


def copy_skill_dirs(target_root: Path, source_dirs: tuple[Path, ...]) -> None:
    for source_dir in source_dirs:
        if not source_dir.exists():
            continue
        for skill_file in source_dir.rglob("SKILL.md"):
            relative_dir = skill_file.parent.relative_to(source_dir)
            target_dir = target_root / relative_dir
            target_dir.mkdir(parents=True, exist_ok=True)
            shutil.copy2(skill_file, target_dir / "SKILL.md")


def summarize_trace(events) -> dict[str, int]:
    summary = {
        "llm_responses": 0,
        "tool_calls": 0,
        "parse_errors": 0,
        "context_compactions": 0,
        "finals": 0,
    }
    for event in events:
        if event.event == "llm_response":
            summary["llm_responses"] += 1
        elif event.event == "tool_call":
            summary["tool_calls"] += 1
        elif event.event == "parse_error":
            summary["parse_errors"] += 1
        elif event.event == "context_compacted":
            summary["context_compactions"] += 1
        elif event.event == "final":
            summary["finals"] += 1
    return summary


def check_expectations(
    scenario: EvalScenario,
    answer: str,
    workspace: Path,
    metrics: EvalMetrics,
    trace_summary: dict[str, int],
    events,
) -> list[str]:
    failures = []
    for text in scenario.expect_output_contains:
        if text not in answer:
            failures.append(f"Answer does not contain expected text: {text!r}")
    for text in scenario.expect_output_not_contains:
        if text in answer:
            failures.append(f"Answer must not contain text: {text!r}")

    for relative_path, expected in scenario.expect_files_contains.items():
        path = workspace / relative_path
        if not path.exists():
            failures.append(_missing_file_failure(relative_path, events))
            continue
        content = path.read_text(encoding="utf-8")
        if not _text_contains_expected(content, expected):
            failures.append(f"File {relative_path} does not contain {expected!r}")

    tool_names = [
        event.data.get("name")
        for event in events
        if event.event == "tool_call" and isinstance(event.data.get("name"), str)
    ]
    for expected in scenario.expect_tool_calls:
        if expected not in tool_names:
            failures.append(
                f"Expected tool call {expected!r} not found. Actual tool calls: {tool_names}"
            )

    event_counts: dict[str, int] = {}
    for event in events:
        event_counts[event.event] = event_counts.get(event.event, 0) + 1
    for event_name, minimum in scenario.expect_trace_events.items():
        actual = event_counts.get(event_name, 0)
        if actual < minimum:
            failures.append(
                f"Expected at least {minimum} trace event(s) named {event_name!r}; found {actual}."
            )

    limits = scenario.limits
    if limits.max_requests is not None and metrics.request_count > limits.max_requests:
        failures.append(
            f"request_count {metrics.request_count} > {limits.max_requests}. "
            "Hint: the agent is probably retrying too much or splitting work "
            "across too many turns."
        )
    if (
        limits.max_tool_calls is not None
        and trace_summary["tool_calls"] > limits.max_tool_calls
    ):
        failures.append(
            f"tool_calls {trace_summary['tool_calls']} > {limits.max_tool_calls}. "
            "Hint: combine tightly coupled local steps into a single tool when "
            "the scenario is time-critical."
        )
    if (
        limits.max_estimated_total_tokens is not None
        and metrics.estimated_total_tokens > limits.max_estimated_total_tokens
    ):
        failures.append(
            f"estimated_total_tokens {metrics.estimated_total_tokens} > "
            f"{limits.max_estimated_total_tokens}. Hint: shorten the system "
            "prompt/skill text, load full skills on demand, and cap large tool "
            "results before sending them back to the model."
        )
    if (
        limits.max_actual_total_tokens is not None
        and metrics.actual_total_tokens
        and metrics.actual_total_tokens > limits.max_actual_total_tokens
    ):
        failures.append(
            f"actual_total_tokens {metrics.actual_total_tokens} > "
            f"{limits.max_actual_total_tokens}. Hint: prefer shorter prompts and "
            "avoid replaying long tool outputs."
        )
    if (
        limits.max_latency_ms is not None
        and metrics.total_latency_ms > limits.max_latency_ms
    ):
        failures.append(
            f"latency_ms {metrics.total_latency_ms:.1f} > {limits.max_latency_ms}"
        )
    return failures


def _missing_file_failure(relative_path: str, events) -> str:
    hint = _missing_file_hint(relative_path, events)
    message = f"Expected file does not exist: {relative_path}"
    if hint:
        return f"{message}. Hint: {hint}"
    return message


def _missing_file_hint(relative_path: str, events) -> str | None:
    tool_events = [event for event in events if event.event == "tool_call"]
    if not tool_events:
        return "no tool_call trace events were recorded"

    for event in reversed(tool_events):
        parsed = _parse_tool_result(event.data.get("result"))
        if _tool_result_failed(parsed):
            detail = _tool_failure_detail(parsed)
            tool_name = event.data.get("name", "<unknown>")
            return f"last failed tool call was `{tool_name}` ({detail})"

    tool_name = tool_events[-1].data.get("name", "<unknown>")
    return (
        f"last tool call was `{tool_name}`; if that tool should create this file, "
        f"confirm it writes `{relative_path}` inside the eval workspace"
    )


def _parse_tool_result(result: Any) -> dict[str, Any] | None:
    if isinstance(result, dict):
        return result
    if not isinstance(result, str):
        return None
    try:
        parsed = json.loads(result)
    except json.JSONDecodeError:
        return None
    return parsed if isinstance(parsed, dict) else None


def _tool_result_failed(result: dict[str, Any] | None) -> bool:
    if result is None:
        return False
    if result.get("ok") is False:
        return True
    error = result.get("error")
    status = result.get("status")
    return isinstance(error, str) or (
        isinstance(status, str) and status.lower().startswith("rejected:")
    )


def _tool_failure_detail(result: dict[str, Any] | None) -> str:
    if result is None:
        return "tool returned an unparsable result"
    error = result.get("error")
    if isinstance(error, str) and error:
        return error
    status = result.get("status")
    if isinstance(status, str) and status:
        return status
    return json.dumps(result, ensure_ascii=False)


def _text_contains_expected(content: str, expected: str) -> bool:
    if expected in content:
        return True
    # Textual eval checks should not fail solely because the model capitalized a
    # phrase like "Path Traversal" differently from the scenario string.
    return expected.casefold() in content.casefold()
