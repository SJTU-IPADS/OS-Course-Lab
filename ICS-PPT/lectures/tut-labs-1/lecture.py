"""ICS 实验说明：课件的使用，nano-quant、bomb-lab、attack-elf、parallel-dot 与
mini-cpu 实验一。

结构安排：本节的安排用一张表列出五个实验。第一部分说明课件的下载、安装与使用，
此后每个实验一部分，按发布的顺序：第二部分 nano-quant（第二讲的量化格式），
第三部分 bomb-lab（第三讲，读 bomb 的机器码），第四部分 attack-elf（第三讲，构造
一行输入让程序调用 report_pass），第五部分 parallel-dot（第三讲第五部分，AVX2 与
CUDA），第六部分 mini-cpu 实验一（x86-64 指令解释器）。每个实验先讲实验框架，再课上
完成一部分，最后说明其余部分与评测；各是一个小节，以一个衔接页开头。bomb-lab 只有
前两个小节：bomb 的使用与 phase 1 的课堂演示；attack-elf 同样只有前两个小节：
attack 的使用与 attack1 的攻破。

页面函数都在 pages.py，按部分排列；它的说明写明各部分的内容与演示输出的来源。
"""

from lecturekit.dsl import Lecture

import pages


lecture = Lecture(
    id="ics-lab-1",
    title="实验说明",
    subtitle="nano-quant · bomb-lab · attack-elf · parallel-dot · mini-cpu 实验一",
    ratio="16:9",
)

lecture.cover(
    "实验说明",
    author="古金宇 · 廖承凡",
    time="上海交通大学 IPADS",
)

with lecture.section("本节的安排", id="framing") as s:
    s.page("five-labs", body=pages.five_labs)

lecture.bridge("第一部分 · 课件\n下载、安装与使用", id="bridge-kit")

with lecture.section("第一部分 · 课件：下载、安装与使用", id="kit") as s:
    s.page("kit-install", body=pages.kit_install)
    s.page("kit-view", body=pages.kit_view)
    s.page("kit-more", body=pages.kit_more)

lecture.bridge("第二部分 · nano-quant\n把 Qwen3-VL-2B-Instruct 量化为 4 位", id="bridge-nq")

with lecture.section("第二部分 · nano-quant：把 Qwen3-VL-2B-Instruct 量化为 4 位", id="nq") as s:
    s.bridge("实验框架", id="bridge-nq-frame")
    with s.section("实验框架", id="nq-frame") as ss:
        ss.page("nq-pipeline", body=pages.nq_pipeline)
        ss.page("nq-call-graph", body=pages.nq_call_graph)
        ss.page("nq-gguf", body=pages.nq_gguf)
        ss.page("nq-blocks", body=pages.nq_blocks)

    s.bridge("Q4_0 的量化与还原", id="bridge-nq-class")
    with s.section("课堂演示：Q4_0 的量化与还原", id="nq-class") as ss:
        ss.page("nq-q4-decl", body=pages.nq_q4_decl)
        ss.page("nq-q4-rule", body=pages.nq_q4_rule)
        ss.page("nq-q4-check", body=pages.nq_q4_check)

    s.bridge("其余部分与评测", id="bridge-nq-rest")
    with s.section("其余部分与评测", id="nq-rest") as ss:
        ss.page("nq-rest-a", body=pages.nq_rest_a)
        ss.page("nq-rest-q4-1", body=pages.nq_rest_q4_1)
        ss.page("nq-rest-q4-k", body=pages.nq_rest_q4_k)

lecture.bridge("第三部分 · bomb-lab\n从机器码推出每个阶段的输入", id="bridge-bomb")

with lecture.section("第三部分 · bomb-lab：从机器码推出每个阶段的输入", id="bomb") as s:
    s.bridge("bomb 的使用\n获取二进制、运行、计分板", id="bridge-bomb-frame")
    with s.section("bomb 的使用", id="bomb-frame") as t:
        t.page("bomb-task", body=pages.bomb_task)
        t.page("bomb-get", body=pages.bomb_get)
        t.page("bomb-screen", body=pages.bomb_screen)
        t.page("bomb-board", body=pages.bomb_board)

    s.bridge("从 phase_1 的机器码反推它接受的一行输入", id="bridge-bomb-class")
    with s.section("课堂演示：phase 1", id="bomb-class") as t:
        t.page("bomb-objdump", body=pages.bomb_objdump)
        t.page("bomb-space", body=pages.bomb_space)
        t.page("bomb-lea", body=pages.bomb_lea)

lecture.bridge("第四部分 · attack-elf\n构造一行输入，让程序调用 report_pass", id="bridge-attack")

with lecture.section("第四部分 · attack-elf：构造一行输入，让程序调用 report_pass", id="attack") as s:
    s.bridge("attack 的使用\n领取程序、运行、提交与记分", id="bridge-attack-use")
    with s.section("attack 的使用", id="attack-use") as t:
        t.page("attack-task", body=pages.attack_task)
        t.page("attack-get", body=pages.attack_get)
        t.page("attack-submit", body=pages.attack_submit)

    s.bridge("从 chat_turn 的栈帧到 report_pass", id="bridge-attack-class")
    with s.section("课堂演示：attack1 的攻破", id="attack-class") as t:
        t.page("attack-frame", body=pages.attack_frame)
        t.page("attack-payload", body=pages.attack_payload)
        t.page("attack-fill", body=pages.attack_fill)

lecture.bridge("第五部分 · parallel-dot\nAVX2 与 CUDA 的矩阵向量乘", id="bridge-pd")

with lecture.section("第五部分 · parallel-dot：AVX2 与 CUDA 的矩阵向量乘", id="pd") as s:
    s.bridge("实验框架", id="bridge-pd-frame")

    with s.section("实验框架", id="pd-frame") as ss:
        ss.page("pd-frame-ops", body=pages.pd_frame_ops)
        ss.page("pd-frame-token", body=pages.pd_frame_token)
        ss.page("pd-frame-args", body=pages.pd_frame_args)
        ss.page("pd-frame-scalar", body=pages.pd_frame_scalar)

    s.bridge("AVX2 版本的 matvec_q4_0", id="bridge-pd-class")

    with s.section("课堂演示：AVX2 版本的 matvec_q4_0", id="pd-class") as ss:
        ss.page("pd-class-plan", body=pages.pd_class_plan)
        ss.page("pd-class-decode", body=pages.pd_class_decode)
        ss.page("pd-class-mac", body=pages.pd_class_mac)
        ss.page("pd-class-check", body=pages.pd_class_check)

    s.bridge("其余部分", id="bridge-pd-rest")

    with s.section("其余部分", id="pd-rest") as ss:
        ss.page("pd-rest-grid", body=pages.pd_rest_grid)
        ss.page("pd-rest-threads", body=pages.pd_rest_threads)
        ss.page("pd-rest-fold", body=pages.pd_rest_fold)

lecture.bridge("第六部分 · mini-cpu 实验一\nx86-64 指令解释器", id="bridge-minicpu")

with lecture.section("第六部分 · mini-cpu 实验一：x86-64 指令解释器", id="minicpu") as s:
    s.bridge("实验框架", id="bridge-cpu-frame")
    with s.section("实验框架", id="cpu-frame") as ss:
        ss.page("cpu-libc", body=pages.cpu_libc)
        ss.page("cpu-hello-syscall", body=pages.cpu_hello_syscall)
        ss.page("cpu-hello-mem", body=pages.cpu_hello_mem)
        ss.page("cpu-loop", body=pages.cpu_loop)
        ss.page("cpu-insn-format", body=pages.cpu_insn_format)
        ss.page("cpu-bytes", body=pages.cpu_bytes)

    s.bridge("ModRM 与 SIB 的解析", id="bridge-cpu-class")
    with s.section("课堂演示：ModRM 与 SIB 的解析", id="cpu-class") as ss:
        ss.page("cpu-modrm-byte", body=pages.cpu_modrm_byte)
        ss.page("cpu-sib-byte", body=pages.cpu_sib_byte)
        ss.page("cpu-rex", body=pages.cpu_rex)
        ss.page("cpu-modrm-flow", body=pages.cpu_modrm_flow)
        ss.page("cpu-modrm-out", body=pages.cpu_modrm_out)
        ss.page("cpu-examples-1", body=pages.cpu_examples_1)

    s.bridge("其余部分\nalu.c 的标志位与 interp.c 的指令", id="bridge-cpu-rest")
    with s.section("其余部分", id="cpu-rest") as ss:
        ss.page("cpu-alu-flags", body=pages.cpu_alu_flags)
        ss.page("cpu-shift", body=pages.cpu_shift)
        ss.page("cpu-insn-groups", body=pages.cpu_insn_groups)
        ss.page("cpu-cond-jump", body=pages.cpu_cond_jump)
        ss.page("cpu-vector", body=pages.cpu_vector)
