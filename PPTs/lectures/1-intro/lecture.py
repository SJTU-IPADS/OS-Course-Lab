"""ICS 第一讲：以一次 `ollama run` 请求为例，自底向上考察系统各层。

结构安排：引子将一次请求分解为五个层次，随后每一节考察一层。
第 3 层（操作系统）为本讲重点，篇幅最长，并贯穿使用同一组具体数据
（三个进程、4.7 GB 权重、缺页装入、页缓存）。
"""

from lecturekit.dsl import Lecture

import pages


lecture = Lecture(
    id="ics-intro",
    title="计算机系统导论",
    subtitle="以一次 ollama 请求为例，自底向上",
    ratio="16:9",
)

lecture.cover(
    "计算机系统导论",
    author="古金宇 · 臧斌宇",
    time="上海交通大学 IPADS",
)

with lecture.section("课程概览", id="course-overview") as s:
    s.page("staff-and-textbooks", body=pages.staff_and_textbooks)
    s.page("course-goals", body=pages.course_goals)

with lecture.section("问题的提出：从 AI 应用到一次推理请求", id="framing") as s:
    s.page("ai-app-request", body=pages.ai_app_request)
    s.page("openai-api", body=pages.openai_api)
    s.page("ollama-intro", body=pages.ollama_intro)
    s.page("one-command", body=pages.one_command)
    s.page("three-processes", body=pages.three_processes)
    s.page("weights-are-data", body=pages.weights_are_data)
    s.page("five-layers", body=pages.five_layers)

lecture.bridge("第 1 层 · 硬件\n处理器、内存、总线与加速器")

with lecture.section("第 1 层 · 硬件：算力、内存与带宽", id="hardware") as s:
    s.page("stored-program", body=pages.stored_program)
    s.page("machine-parts", body=pages.machine_parts)
    s.page("cpu-vs-gpu", body=pages.cpu_vs_gpu)

lecture.bridge("第 2 层 · 指令集\n指令、寄存器与特权级")

with lecture.section("第 2 层 · 指令集：软硬件接口规范", id="isa") as s:
    s.page("isa-contract", body=pages.isa_contract)
    s.page("x86-lineage", body=pages.x86_lineage)

lecture.bridge("第 3 层 · 操作系统\n进程、虚拟内存、文件与设备")

with lecture.section("第 3 层 · 操作系统：资源管理与保护", id="os") as s:
    s.page("why-os", body=pages.why_os)
    s.page("os-services", body=pages.os_services)
    s.page("process-isolation", body=pages.process_isolation)
    s.page("virtual-address-space", body=pages.virtual_address_space)
    s.page("loading-problem", body=pages.loading_problem)
    s.page("model-loading", body=pages.model_loading)
    s.page("mmap-in-practice", body=pages.mmap_in_practice)
    s.page("page-cache", body=pages.page_cache)
    s.page("devices-and-drivers", body=pages.devices_and_drivers)
    s.page("scheduling", body=pages.scheduling)
    s.page("os-evolution", body=pages.os_evolution)
    s.page("os-evolution-people", body=pages.os_evolution_people)

lecture.bridge("第 4 层 · 工具链与运行时\n编译、链接与语言库")

with lecture.section("第 4 层 · 工具链与运行时：机器指令的生成", id="toolchain") as s:
    s.page("source-is-bytes", body=pages.source_is_bytes)
    s.page("why-c", body=pages.why_c)
    s.page("mini-ollama", body=pages.mini_ollama)
    s.page("mini-ollama-boundaries", body=pages.mini_ollama_boundaries)
    s.page("compile-pipeline", body=pages.compile_pipeline)
    s.page("machine-code", body=pages.machine_code)
    s.page("runtime-libraries", body=pages.runtime_libraries)
    s.page("python-and-pytorch", body=pages.python_and_pytorch)
    s.page("cuda-kernel", body=pages.cuda_kernel)
    s.page("nvcc-compilation", body=pages.nvcc_compilation)

lecture.bridge("第 5 层 · 应用与 Agent\n对话界面、工具调用与推理服务")

with lecture.section("第 5 层 · 应用与 Agent", id="application") as s:
    s.page("agent-loop", body=pages.agent_loop)
    s.page("ai-os-challenges", body=pages.ai_os_challenges)

with lecture.section("总结与课程路线", id="synthesis") as s:
    s.page("request-recap", body=pages.request_recap)
    s.page("failures-between-layers", body=pages.failures_between_layers)
    s.page("four-themes", body=pages.four_themes)
    s.page("course-map", body=pages.course_map)

lecture.close("course-goal", body=pages.course_goal)
