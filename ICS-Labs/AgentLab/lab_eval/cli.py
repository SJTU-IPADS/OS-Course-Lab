from __future__ import annotations

import argparse
from pathlib import Path

from ics_agent_lab.runtime import RuntimeConfig

from .runner import run_scenario
from .scenario import EvalScenario


def main() -> None:
    parser = argparse.ArgumentParser(description="Run an ICS Agent Lab eval scenario.")
    parser.add_argument("scenario", type=Path)
    parser.add_argument("--provider", choices=["openrouter"], default="openrouter")
    parser.add_argument("--model", default=None)
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--temperature", type=float, default=0.0)
    parser.add_argument("--extra-tool-dir", action="append", default=[])
    parser.add_argument("--keep-workspace", action="store_true", default=True)
    args = parser.parse_args()

    scenario = EvalScenario.from_file(args.scenario)
    result = run_scenario(
        scenario,
        RuntimeConfig(
            provider=args.provider,
            model=args.model,
            seed=args.seed,
            temperature=args.temperature,
            extra_tool_dirs=tuple(Path(path) for path in args.extra_tool_dir),
        ),
        keep_workspace=args.keep_workspace,
    )
    print(result.to_json())
    raise SystemExit(0 if result.passed else 1)


if __name__ == "__main__":
    main()
