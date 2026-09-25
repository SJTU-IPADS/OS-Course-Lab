from __future__ import annotations

from dotenv import load_dotenv

from ..core import Agent, AgentConfig, TraceRecorder
from ..llm import LLMTransport, OpenRouterChatTransport
from ..memory import MemoryLoader
from ..skills import SkillLoader
from ..tools import build_default_tools
from .config import RuntimeConfig
from .runtime import AgentRuntime


def build_runtime(
    config: RuntimeConfig,
    llm: LLMTransport | None = None,
) -> AgentRuntime:
    load_dotenv(override=True)
    workspace = config.workspace
    skill_loader = SkillLoader(config.skills_dir)
    memory_loader = MemoryLoader(workspace / ".agent_memory")
    llm = llm or build_llm(config)
    trace = TraceRecorder()

    def run_subagent(task: str) -> str:
        sub_tools = build_default_tools(
            workspace,
            skill_loader=skill_loader,
            memory_loader=memory_loader,
            extra_tool_dirs=config.extra_tool_dirs,
        )
        subagent = Agent(
            llm=llm,
            tools=sub_tools,
            config=AgentConfig(
                max_steps=config.subagent_max_steps,
            ),
            trace=trace,
            skill_docs=skill_loader.descriptions(),
            memory_docs=memory_loader.descriptions(),
            name="subagent",
        )
        return subagent.run(task)

    tools = build_default_tools(
        workspace,
        subagent_runner=run_subagent,
        skill_loader=skill_loader,
        memory_loader=memory_loader,
        extra_tool_dirs=config.extra_tool_dirs,
    )
    agent = Agent(
        llm=llm,
        tools=tools,
        trace=trace,
        skill_docs=skill_loader.descriptions(),
        memory_docs=memory_loader.descriptions(),
        name="main",
    )
    return AgentRuntime(
        agent=agent,
        llm=llm,
        skill_loader=skill_loader,
        memory_loader=memory_loader,
        config=config,
    )


def build_llm(config: RuntimeConfig) -> LLMTransport:
    if config.provider == "openrouter":
        return OpenRouterChatTransport(
            model=config.model,
            seed=config.seed,
            temperature=config.temperature,
        )
    raise ValueError(f"Unknown provider: {config.provider}")
