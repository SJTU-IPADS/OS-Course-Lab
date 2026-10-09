"""ICS 第 3 章：程序的机器级表示与执行。

以向量内积 dot_product 为唯一主线，分四部分推进：
单次乘加（寄存器、寻址、数据传送与算术指令），循环控制（%rip、跳转指令、标志位、条件跳转、跳转表与间接跳转），
函数调用（call/ret 与运行时栈、参数传递、寄存器使用惯例、局部变量与栈帧、缓冲区溢出、金丝雀），向量化（YMM、AVX2、perf 实测、CUDA）。
"""

from lecturekit.dsl import Lecture

import pages


lecture = Lecture(
    id="ics-asm",
    title="程序的机器级表示与执行",
    subtitle="一行大模型代码的硬件之旅",
    ratio="16:9",
)

lecture.cover(
    "程序的机器级表示与执行",
    author="古金宇 · 陈榕",
    time="上海交通大学 IPADS",
)

with lecture.section('前导概览：硬件系统与程序映射', id="prologue") as s:
    s.page("problem", body=pages.problem)
    s.page("system-view-fig", body=pages.system_view_fig)
    s.page("compile-mapping", body=pages.compile_mapping)
    s.page("toolchain", body=pages.toolchain)
    s.page("toolchain-2", body=pages.toolchain_2)
    s.page("toolchain-fig", body=pages.toolchain_fig)
    s.page("isa-contract", body=pages.isa_contract)
    s.page("isa-contract-fig", body=pages.isa_contract_fig)

lecture.bridge('第一部分：执行单次计算', id="bridge-part1")


with lecture.section('第一部分：执行单次计算——运算只能在寄存器上进行', id="part1") as s:
    s.page("single-mac", body=pages.single_mac)
    s.page("single-mac-fig", body=pages.single_mac_fig)

    with s.section('数据在内存里，如何参与 CPU 的运算？', id="part1-operands") as ss:
        ss.page("visible-state", body=pages.visible_state)
        ss.page("register-slices-fig", body=pages.register_slices_fig)
        ss.page("visible-state-2", body=pages.visible_state_2)

    s.bridge('内存数据被加载到CPU寄存器中参与计算，\n解决了"放到哪"的问题。\n内存数据"从哪取"：CPU 如何知晓数据的位置？', id="bridge-part1-address")
    with s.section('CPU 如何知晓数据的位置？', id="part1-address") as ss:
        ss.page("effective-address", body=pages.effective_address)
        ss.page("addressing-modes-fig", body=pages.addressing_modes_fig)
        ss.page("lea-agu", body=pages.lea_agu)
        ss.page("lea-agu-fig", body=pages.lea_agu_fig)

    s.bridge('搬运数据的CPU指令', id="bridge-part1-execute")
    with s.section('CPU 如何搬运数据，如何执行运算？', id="part1-execute") as ss:
        ss.page("movl-load", body=pages.movl_load)
        ss.page("movl-load-2", body=pages.movl_load_2)
        ss.page("width-write", body=pages.width_write)
        ss.page("load-example", body=pages.load_example)
        ss.page("load-example-2", body=pages.load_example_2)
        ss.page("load-example-3", body=pages.load_example_3)
        ss.bridge('执行运算的CPU指令', id="bridge-part1-compute")
        ss.page("extension", body=pages.extension)
        ss.page("extension-fig", body=pages.extension_fig)
        ss.page("extension-example", body=pages.extension_example)
        ss.page("int-arith", body=pages.int_arith)
        ss.page("int-arith-2", body=pages.int_arith_2)
        ss.page("insn-table", body=pages.insn_table)
        ss.page("xor-strength", body=pages.xor_strength)
        ss.page("xor-strength-2", body=pages.xor_strength_2)
        ss.page("insn-bytes", body=pages.insn_bytes)

    with s.section('小结与练习', id="part1-summary") as ss:
        ss.page("recap-part1", body=pages.recap_part1)
        ss.page("recap-part1-fig", body=pages.recap_part1_fig)
        ss.page("disasm-exercise", body=pages.disasm_exercise)
        ss.page("disasm-exercise-2", body=pages.disasm_exercise_2)
        ss.page("mac-exercise", body=pages.mac_exercise)
        ss.page("mac-exercise-2", body=pages.mac_exercise_2)

lecture.bridge('第二部分：循环控制与跳转指令', id="bridge-part2")


with lecture.section('第二部分：循环控制与跳转指令', id="part2") as s:
    s.page("loop-need", body=pages.loop_need)
    s.page("loop-need-2", body=pages.loop_need_2)
    s.page("loop-asm", body=pages.loop_asm)
    s.page("loop-asm-2", body=pages.loop_asm_2)
    s.page("jump-insn", body=pages.jump_insn)
    s.page("rflags", body=pages.rflags)
    s.page("rflags-2", body=pages.rflags_2)
    s.page("cmp-test", body=pages.cmp_test)
    s.page("cmp-test-2", body=pages.cmp_test_2)
    s.page("cond-jump", body=pages.cond_jump)
    s.page("jump-direct", body=pages.jump_direct)
    s.page("switch-table", body=pages.switch_table)
    s.page("switch-table-2", body=pages.switch_table_2)
    s.page("switch-table-3", body=pages.switch_table_3)
    s.page("cmov", body=pages.cmov)
    s.page("cmov-fig", body=pages.cmov_fig)
    s.page("cmov-timing", body=pages.cmov_timing)

lecture.bridge('第三部分：函数调用', id="bridge-part3")


with lecture.section('第三部分：函数调用', id="part3") as s:
    s.page("procedure-need", body=pages.procedure_need)
    s.page("call-vs-jump", body=pages.call_vs_jump)
    s.page("call-checklist", body=pages.call_checklist)

    with s.section('调用与返回：call 与 ret', id="part3-call") as ss:
        ss.page("return-address", body=pages.return_address)
        ss.page("runtime-stack", body=pages.runtime_stack)
        ss.page("push-pop", body=pages.push_pop)
        ss.page("call-emulate", body=pages.call_emulate)
        ss.page("call-ret", body=pages.call_ret)
        ss.page("stack-frames", body=pages.stack_frames)

    with s.section('传递数据：寄存器、栈', id="part3-data") as ss:
        ss.page("call-checklist-2", body=pages.call_checklist_2)
        ss.page("param-regs", body=pages.param_regs)
        ss.page("dot-params", body=pages.dot_params)
        ss.page("stack-args", body=pages.stack_args)
        ss.page("stack-args-2", body=pages.stack_args_2)

    with s.section('寄存器：使用惯例', id="part3-regs") as ss:
        ss.page("call-checklist-3", body=pages.call_checklist_3)
        ss.page("reg-conflict", body=pages.reg_conflict)
        ss.page("reg-conflict-2", body=pages.reg_conflict_2)
        ss.page("saved-regs", body=pages.saved_regs)
        ss.page("saved-regs-fig", body=pages.saved_regs_fig)
        ss.page("callee-example", body=pages.callee_example)
        ss.page("callee-example-why", body=pages.callee_example_why)
        ss.page("callee-example-rows", body=pages.callee_example_rows)

    with s.section('局部变量：栈', id="part3-locals") as ss:
        ss.page("call-checklist-4", body=pages.call_checklist_4)
        ss.page("local-vars", body=pages.local_vars)
        ss.page("stack-frame", body=pages.stack_frame)
        ss.page("stack-frame-2", body=pages.stack_frame_2)

    with s.section('综合起来：一次调用的完整步骤', id="part3-together") as ss:
        ss.page("call-checklist-5", body=pages.call_checklist_5)
        ss.page("call-sequence", body=pages.call_sequence)
        ss.page("abi-isa", body=pages.abi_isa)

    s.bridge('缓冲区溢出与 ROP 攻击\nReturn-Oriented Programming', id="bridge-part3-overflow")
    with s.section('缓冲区溢出', id="part3-overflow") as ss:
        ss.page("gets-no-bound", body=pages.gets_no_bound)
        ss.page("echo-buf", body=pages.echo_buf)
        ss.page("echo-frame", body=pages.echo_frame)
        ss.page("overflow-attack", body=pages.overflow_attack)
        ss.page("morris-worm", body=pages.morris_worm)
        ss.page("morris-worm-2", body=pages.morris_worm_2)

    with s.section('对抗缓冲区溢出攻击', id="part3-defense") as ss:
        ss.page("stack-random", body=pages.stack_random)
        ss.page("stack-random-range", body=pages.stack_random_range)
        ss.page("nop-sled", body=pages.nop_sled)
        ss.page("canary", body=pages.canary)
        ss.page("canary-asm", body=pages.canary_asm)
        ss.page("nx-bit", body=pages.nx_bit)
        ss.page("code-reuse", body=pages.code_reuse)
        ss.page("shadow-stack", body=pages.shadow_stack)

lecture.bridge('第四部分：性能瓶颈与向量化\n从标量计算到向量与并发', id="bridge-part4")


with lecture.section('第四部分：性能瓶颈与向量化——从标量计算到向量与并发', id="part4") as s:
    s.page("flops-estimate", body=pages.flops_estimate)
    s.page("hw-peak", body=pages.hw_peak)
    s.page("measured-speed", body=pages.measured_speed)
    s.page("system-limits", body=pages.system_limits)
    s.page("insn-mix", body=pages.insn_mix)
    s.page("speedup-plan", body=pages.speedup_plan)
    s.bridge('SIMD 思想：数据并行', id="bridge-simd")
    with s.section('SIMD 思想：数据并行', id="part4-simd") as ss:
        ss.page("simd-history", body=pages.simd_history)
        ss.page("simd-history-fig", body=pages.simd_history_fig)
        ss.page("ymm", body=pages.ymm)
        ss.page("vector-naming", body=pages.vector_naming)
        ss.page("vector-arith", body=pages.vector_arith)
        ss.page("vector-arith-fig", body=pages.vector_arith_fig)
        ss.page("intrinsics", body=pages.intrinsics)
        ss.page("vector-loop", body=pages.vector_loop)
        ss.page("perf", body=pages.perf)
        ss.page("perf-2", body=pages.perf_2)
    s.bridge('SIMT 思想：线程并行', id="bridge-simt")
    with s.section('SIMT 思想：线程并行', id="part4-simt") as ss:
        ss.page("gpu-why", body=pages.gpu_why)
        ss.page("gpu-arch", body=pages.gpu_arch)
        ss.page("gpu-arch-fig", body=pages.gpu_arch_fig)
        ss.page("cuda-kernel", body=pages.cuda_kernel)
        ss.page("cuda-host", body=pages.cuda_host)
        ss.page("cuda-build", body=pages.cuda_build)
        ss.page("cuda-remote", body=pages.cuda_remote)

lecture.bridge('总结与实验任务', id="bridge-wrapup")


with lecture.section('总结与实验任务', id="wrapup") as s:
    s.page("insn-summary", body=pages.insn_summary)
    s.page("insn-summary-2", body=pages.insn_summary_2)
    s.page("insn-summary-fig", body=pages.insn_summary_fig)
    s.page("summary", body=pages.summary)
    s.page("summary-2", body=pages.summary_2)
    s.page("exercise", body=pages.exercise)
    s.page("lab-mini-cpu", body=pages.lab_mini_cpu)
