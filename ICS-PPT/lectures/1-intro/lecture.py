"""ICS 第一讲：大模型时代的课程目标，以及以一次 `ollama run` 请求为例的系统各层介绍。

结构安排：先说明学习本课程的原因与目标；再以一次推理请求说明 AI 应用仍然通过
程序执行实现，以及应用与系统的关系；中间四个部分按目标依次介绍硬件、汇编与
指令集、工具链、操作系统；最后回到这次请求，说明本课程的内容如何用于分析问题
与改进程序。
"""

from lecturekit.dsl import Lecture

import pages


lecture = Lecture(
    id="ics-intro",
    title="计算机系统基础（1）",
    subtitle="以一次 ollama 请求为例，自底向上",
    ratio="16:9",
)

lecture.cover(
    "计算机系统基础（1）",
    author="古金宇 · 陈榕",
    time="上海交通大学 IPADS",
)

with lecture.section("课程概览", id="course-overview") as s:
    s.page("staff-and-textbooks", body=pages.staff_and_textbooks)
    s.page("worth-taking", body=pages.worth_taking)
    s.page("coding-vs-constraints", body=pages.coding_vs_constraints)
    s.page("course-features", body=pages.course_features)
    s.page("course-goals", body=pages.course_goals)
    s.page("course-questions", body=pages.course_questions)

with lecture.section("AI 应用仍然通过程序执行实现", id="framing") as s:
    s.page("ai-app-request", body=pages.ai_app_request)
    s.page("agent-loop", body=pages.agent_loop)
    s.page("openai-api", body=pages.openai_api)
    s.page("service-design", body=pages.service_design)
    s.page("ollama-intro", body=pages.ollama_intro)

with lecture.section("应用与系统", id="app-and-system") as s:
    s.page("one-command", body=pages.one_command)
    s.page("three-processes", body=pages.three_processes)
    s.page("weights-are-data", body=pages.weights_are_data)
    s.page("system-abstraction", body=pages.system_abstraction)
    s.page("five-layers", body=pages.five_layers)
    s.page("layered-design", body=pages.layered_design)
    s.page("ics-scope", body=pages.ics_scope)

lecture.bridge("第 1 部分 · 硬件\n目标：计算的软件化")

with lecture.section("第 1 部分 · 硬件：计算、存储与通信", id="hardware") as s:
    s.page("stored-program", body=pages.stored_program)
    s.page("machine-parts", body=pages.machine_parts)
    s.page("cpu-vs-gpu", body=pages.cpu_vs_gpu)
    s.page("hardware-in-ics", body=pages.hardware_in_ics)

lecture.bridge("第 2 部分 · 汇编与指令集\n目标：快速开发程序")

with lecture.section("第 2 部分 · 汇编与指令集：软硬件接口规范", id="isa") as s:
    s.page("isa-contract", body=pages.isa_contract)
    s.page("x86-lineage", body=pages.x86_lineage)
    s.page("isa-in-ics", body=pages.isa_in_ics)

lecture.bridge("第 3 部分 · 工具链与运行时\n目标：消除程序绑定")

with lecture.section("第 3 部分 · 工具链与运行时：机器指令的生成", id="toolchain") as s:
    s.page("program-binding", body=pages.program_binding)
    s.page("source-is-bytes", body=pages.source_is_bytes)
    s.page("why-c", body=pages.why_c)
    s.page("mini-ollama", body=pages.mini_ollama)
    s.page("mini-ollama-boundaries", body=pages.mini_ollama_boundaries)
    s.page("machine-code", body=pages.machine_code)
    s.page("runtime-libraries", body=pages.runtime_libraries)
    s.page("python-and-pytorch", body=pages.python_and_pytorch)
    s.page("cuda-kernel", body=pages.cuda_kernel)
    s.page("nvcc-compilation", body=pages.nvcc_compilation)
    s.page("toolchain-in-ics", body=pages.toolchain_in_ics)

lecture.bridge("第 4 部分 · 操作系统\n目标：多个程序共享硬件")

with lecture.section("第 4 部分 · 操作系统：资源管理与保护", id="os") as s:
    s.page("why-os", body=pages.why_os)
    s.page("os-services", body=pages.os_services)
    s.page("exclusive-use", body=pages.exclusive_use)
    s.page("process-isolation", body=pages.process_isolation)
    s.page("mmap-in-practice", body=pages.mmap_in_practice)
    s.page("devices-and-drivers", body=pages.devices_and_drivers)
    s.page("scheduling", body=pages.scheduling)
    s.page("os-evolution", body=pages.os_evolution)
    s.page("os-evolution-people", body=pages.os_evolution_people)
    s.page("os-in-ics", body=pages.os_in_ics)

lecture.bridge("回到例子：一次模型推理\n学完本课程之后能够做什么")

with lecture.section("回到例子：分析问题与改进程序", id="synthesis") as s:
    s.page("ai-os-challenges", body=pages.ai_os_challenges)
    s.page("request-recap", body=pages.request_recap)
    s.page("failures-between-layers", body=pages.failures_between_layers)
    s.page("ai-infra-from-ics", body=pages.ai_infra_from_ics)
    s.page("four-themes", body=pages.four_themes)
    s.page("course-map", body=pages.course_map)

lecture.close("course-goal", body=pages.course_goal)
