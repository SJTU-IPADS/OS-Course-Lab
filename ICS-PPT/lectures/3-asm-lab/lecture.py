"""ICS 实验说明：课件的使用，nano-quant、bomb-lab、parallel-dot 与 mini-cpu 实验一。

结构安排：本节的安排用一张表列出四个实验。第一部分说明课件的下载、安装与使用，
此后每个实验一部分，按发布的顺序：第二部分 nano-quant（第二讲的量化格式），
第三部分 bomb-lab（第三讲，读 bomb 的机器码），第四部分 parallel-dot（第三讲
第五部分，AVX2 与 CUDA），第五部分 mini-cpu 实验一（x86-64 指令解释器）。
每个实验先讲实验框架，再课上完成一部分，最后说明其余部分与评测；各是一个小节，
以一个衔接页开头。bomb-lab 只有前两个小节：bomb 的使用与 phase 1 的课堂演示。

页面函数都在 pages.py，按部分排列；它的说明写明各部分的内容与演示输出的来源。
"""

from lecturekit.dsl import Lecture

import pages


lecture = Lecture(
    id="ics-lab-1",
    title="实验说明",
    subtitle="nano-quant · bomb-lab · parallel-dot · mini-cpu 实验一",
    ratio="16:9",
)

lecture.cover(
    "实验说明",
    author="古金宇 · 陈榕",
    time="上海交通大学 IPADS",
)

with lecture.section("本节的安排", id="framing") as s:
    s.page("four-labs", body=pages.four_labs)

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

lecture.bridge("第四部分 · parallel-dot\nAVX2 与 CUDA 的矩阵向量乘", id="bridge-pd")

with lecture.section("第四部分 · parallel-dot：AVX2 与 CUDA 的矩阵向量乘", id="pd") as s:
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

lecture.bridge("第五部分 · mini-cpu 实验一\nx86-64 指令解释器", id="bridge-minicpu")

with lecture.section("第五部分 · mini-cpu 实验一：x86-64 指令解释器", id="minicpu") as s:
    s.bridge("实验框架", id="bridge-cpu-frame")
    with s.section("实验框架", id="cpu-frame") as ss:
        ss.page("cpu-hello", body=pages.cpu_hello)
        ss.page("cpu-hello-mem", body=pages.cpu_hello_mem)
        ss.page("cpu-hello-syscall", body=pages.cpu_hello_syscall)
        ss.page("cpu-parts", body=pages.cpu_parts)
        ss.page("cpu-state", body=pages.cpu_state)
        ss.page("cpu-loop", body=pages.cpu_loop)
        ss.page("cpu-bytes", body=pages.cpu_bytes)
        ss.page("cpu-helpers", body=pages.cpu_helpers)
        ss.page("cpu-given", body=pages.cpu_given)

    s.bridge("课上完成：ModRM 与 SIB 的解析\nmodrm.c 的 modrm_decode", id="bridge-cpu-class")
    with s.section("课上完成：ModRM 与 SIB 的解析", id="cpu-class") as ss:
        ss.page("cpu-modrm-byte", body=pages.cpu_modrm_byte)
        ss.page("cpu-sib-byte", body=pages.cpu_sib_byte)
        ss.page("cpu-rex", body=pages.cpu_rex)
        ss.page("cpu-modrm-flow", body=pages.cpu_modrm_flow)
        ss.page("cpu-modrm-out", body=pages.cpu_modrm_out)
        ss.page("cpu-examples-1", body=pages.cpu_examples_1)
        ss.page("cpu-examples-2", body=pages.cpu_examples_2)
        ss.page("cpu-modrm-check", body=pages.cpu_modrm_check)

    s.bridge("其余部分\nalu.c 的标志位与 interp.c 的指令", id="bridge-cpu-rest")
    with s.section("其余部分", id="cpu-rest") as ss:
        ss.page("cpu-alu-flags", body=pages.cpu_alu_flags)
        ss.page("cpu-cf-of", body=pages.cpu_cf_of)
        ss.page("cpu-shift", body=pages.cpu_shift)
        ss.page("cpu-insn-groups", body=pages.cpu_insn_groups)
        ss.page("cpu-cond-jump", body=pages.cpu_cond_jump)
        ss.page("cpu-vector", body=pages.cpu_vector)

    s.bridge("调试、评测与提交", id="bridge-cpu-check")
    with s.section("调试、评测与提交", id="cpu-check") as ss:
        ss.page("cpu-run-fault", body=pages.cpu_run_fault)
        ss.page("cpu-trace", body=pages.cpu_trace)
        ss.page("cpu-make-check", body=pages.cpu_make_check)
        ss.page("cpu-check-fail", body=pages.cpu_check_fail)
        ss.page("cpu-rules", body=pages.cpu_rules)
        ss.page("cpu-handin", body=pages.cpu_handin)
