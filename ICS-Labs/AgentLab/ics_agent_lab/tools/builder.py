from __future__ import annotations

import importlib
import importlib.util
import inspect
import pkgutil
import sys
from pathlib import Path
from types import ModuleType
from typing import Any, Callable

from ..memory import MemoryLoader
from ..skills import SkillLoader
from .base import Tool, ToolRegistry, Workspace

SKIP_MODULES = {"base", "builder"}


def build_default_tools(
    workspace: Path,
    subagent_runner: Callable[[str], str] | None = None,
    skill_loader: SkillLoader | None = None,
    memory_loader: MemoryLoader | None = None,
    extra_tool_dirs: tuple[Path, ...] = (),
) -> ToolRegistry:
    workspace_context = Workspace(workspace)
    registry = ToolRegistry()
    context = {
        "workspace": workspace_context,
        "subagent_runner": subagent_runner,
        "skill_loader": skill_loader,
        "memory_loader": memory_loader,
    }

    for module in discover_tool_modules(extra_tool_dirs):
        registry.register(make_tool_from_module(module, context))

    return registry


def discover_tool_modules(extra_tool_dirs: tuple[Path, ...] = ()) -> list[ModuleType]:
    modules = []
    package_name = __package__
    if package_name is None:
        raise RuntimeError("Tool builder must be imported as a package module.")

    for module_info in sorted(
        pkgutil.iter_modules(package_paths()), key=lambda item: item.name
    ):
        if (
            module_info.ispkg
            or module_info.name.startswith("_")
            or module_info.name in SKIP_MODULES
        ):
            continue
        module = importlib.import_module(f"{package_name}.{module_info.name}")
        if hasattr(module, "make_tool"):
            modules.append(module)

    for tool_dir in extra_tool_dirs:
        modules.extend(discover_tool_modules_from_dir(tool_dir))
    return modules


def discover_tool_modules_from_dir(tool_dir: Path) -> list[ModuleType]:
    tool_dir = tool_dir.resolve()
    if not tool_dir.exists():
        return []

    parent = str(tool_dir.parent)
    if parent not in sys.path:
        sys.path.insert(0, parent)

    modules = []
    for path in sorted(tool_dir.glob("*.py")):
        if path.name.startswith("_"):
            continue
        module_name = f"ics_agent_lab_external_tool_{tool_dir.name}_{path.stem}"
        spec = importlib.util.spec_from_file_location(module_name, path)
        if spec is None or spec.loader is None:
            continue
        module = importlib.util.module_from_spec(spec)
        sys.modules[module_name] = module
        spec.loader.exec_module(module)
        if hasattr(module, "make_tool"):
            modules.append(module)
    return modules


def make_tool_from_module(module: ModuleType, context: dict[str, Any]) -> Tool:
    make_tool = getattr(module, "make_tool", None)
    if make_tool is None or not callable(make_tool):
        raise TypeError(f"{module.__name__} does not define callable make_tool().")

    kwargs = {}
    for name in inspect.signature(make_tool).parameters:
        if name not in context:
            raise TypeError(
                f"{module.__name__}.make_tool() asks for unknown dependency `{name}`."
            )
        kwargs[name] = context[name]

    tool = make_tool(**kwargs)
    if not isinstance(tool, Tool):
        raise TypeError(f"{module.__name__}.make_tool() must return Tool.")
    return tool


def package_paths() -> list[str]:
    package = importlib.import_module(__package__ or "ics_agent_lab.tools")
    return list(package.__path__)
