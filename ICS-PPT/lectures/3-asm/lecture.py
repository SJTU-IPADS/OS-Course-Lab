"""ICS 第 3 章：程序的机器级表示与执行。

以向量内积 dot_product 为唯一主线，按 PPTContents.md 的大纲分四部分推进：
单次乘加（寄存器、寻址、数据传送与算术指令），循环控制（%rip、标志位、条件跳转、循环结构），
过程调用（运行时栈与栈帧、call/ret、ABI、缓冲区溢出、金丝雀），向量化（YMM、AVX2、perf 实测、OpenMP、CUDA）。
页面文案逐字取自 PPTContents.md，见 pages.py 开头的说明。
"""

from lecturekit.dsl import Lecture

import pages


lecture = Lecture(
    id="ics-asm",
    title="第 3 章 程序的机器级表示与执行",
    subtitle="一行大模型代码的硬件之旅",
    ratio="16:9",
)

lecture.cover(
    "第 3 章 程序的机器级表示与执行",
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
    s.page("isa-contract-2", body=pages.isa_contract_2)
    s.page("isa-contract-fig", body=pages.isa_contract_fig)

lecture.bridge('第一部分：执行单次计算\n运算只能在寄存器上进行', id="bridge-part1")


with lecture.section('第一部分：执行单次计算——运算只能在寄存器上进行', id="part1") as s:
    s.page("single-mac", body=pages.single_mac)
    s.page("single-mac-fig", body=pages.single_mac_fig)

    s.bridge('数据在内存里，如何参与 CPU 的运算？', id="bridge-part1-operands")
    with s.section('数据在内存里，如何参与 CPU 的运算？', id="part1-operands") as ss:
        ss.page("von-neumann", body=pages.von_neumann)
        ss.page("visible-state", body=pages.visible_state)
        ss.page("visible-state-fig", body=pages.visible_state_fig)
        ss.page("visible-state-2", body=pages.visible_state_2)
        ss.page("register-slices", body=pages.register_slices)
        ss.page("register-slices-2", body=pages.register_slices_2)
        ss.page("register-slices-fig", body=pages.register_slices_fig)

    s.bridge('CPU 如何知晓数据的位置？', id="bridge-part1-address")
    with s.section('CPU 如何知晓数据的位置？', id="part1-address") as ss:
        ss.page("effective-address", body=pages.effective_address)
        ss.page("addressing-modes", body=pages.addressing_modes)
        ss.page("addressing-modes-2", body=pages.addressing_modes_2)
        ss.page("addressing-modes-fig", body=pages.addressing_modes_fig)
        ss.page("lea-agu", body=pages.lea_agu)
        ss.page("lea-agu-fig", body=pages.lea_agu_fig)

    s.bridge('CPU 如何搬运数据，如何执行运算？', id="bridge-part1-execute")
    with s.section('CPU 如何搬运数据，如何执行运算？', id="part1-execute") as ss:
        ss.page("movl-load", body=pages.movl_load)
        ss.page("movl-load-2", body=pages.movl_load_2)
        ss.page("width-write", body=pages.width_write)
        ss.page("load-example", body=pages.load_example)
        ss.page("load-example-2", body=pages.load_example_2)
        ss.page("load-example-3", body=pages.load_example_3)
        ss.page("extension", body=pages.extension)
        ss.page("extension-fig", body=pages.extension_fig)
        ss.page("int-arith", body=pages.int_arith)
        ss.page("shift-ops", body=pages.shift_ops)
        ss.page("shift-ops-3", body=pages.shift_ops_3)
        ss.page("shift-ops-fig", body=pages.shift_ops_fig)
        ss.page("xor-strength", body=pages.xor_strength)
        ss.page("xor-strength-2", body=pages.xor_strength_2)
        ss.page("mac-exercise", body=pages.mac_exercise)
        ss.page("mac-exercise-2", body=pages.mac_exercise_2)
        ss.page("memory-operand", body=pages.memory_operand)
        ss.page("memory-operand-2", body=pages.memory_operand_2)

lecture.bridge('第二部分：循环控制与状态机推进', id="bridge-part2")


with lecture.section('第二部分：循环控制与状态机推进', id="part2") as s:
    s.page("loop-need", body=pages.loop_need)
    s.page("pc-update", body=pages.pc_update)
    s.page("jump-encoding", body=pages.jump_encoding)
    s.page("jump-encoding-2", body=pages.jump_encoding_2)
    s.page("rflags", body=pages.rflags)
    s.page("rflags-2", body=pages.rflags_2)
    s.page("cmp-test", body=pages.cmp_test)
    s.page("cmp-test-2", body=pages.cmp_test_2)
    s.page("cond-jump", body=pages.cond_jump)
    s.page("loop-forms-for", body=pages.loop_forms_for)
    s.page("loop-forms", body=pages.loop_forms)
    s.page("loop-forms-fig", body=pages.loop_forms_fig)
    s.page("cmov", body=pages.cmov)
    s.page("cmov-fig", body=pages.cmov_fig)
    s.page("type-neutral", body=pages.type_neutral)
    s.page("type-neutral-fig", body=pages.type_neutral_fig)

lecture.bridge('第三部分：函数的硬件实现\n从代码组织到过程调用', id="bridge-part3")


with lecture.section('第三部分：函数的硬件实现——从代码组织到过程调用', id="part3") as s:
    s.page("procedure-need", body=pages.procedure_need)

    s.bridge('内存中局部状态的布局', id="bridge-part3-layout")
    with s.section('内存中局部状态的布局', id="part3-layout") as ss:
        ss.page("local-state", body=pages.local_state)
        ss.page("local-state-2", body=pages.local_state_2)
        ss.page("runtime-stack", body=pages.runtime_stack)
        ss.page("stack-frame", body=pages.stack_frame)
        ss.page("stack-frame-2", body=pages.stack_frame_2)

    s.bridge('函数调用产生的局部状态', id="bridge-part3-call")
    with s.section('函数调用产生的局部状态', id="part3-call") as ss:
        ss.page("return-address", body=pages.return_address)
        ss.page("call-ret", body=pages.call_ret)
        ss.page("param-state", body=pages.param_state)
        ss.page("sysv-abi", body=pages.sysv_abi)
        ss.page("sysv-abi-fig", body=pages.sysv_abi_fig)
        ss.page("dot-params", body=pages.dot_params)
        ss.page("dot-params-fig", body=pages.dot_params_fig)
        ss.page("param-clobber", body=pages.param_clobber)
        ss.page("param-clobber-fig", body=pages.param_clobber_fig)

    s.bridge('寄存器的调用约定，及与内存的取舍', id="bridge-part3-abi")
    with s.section('寄存器的调用约定，及与内存的取舍', id="part3-abi") as ss:
        ss.page("saved-regs", body=pages.saved_regs)
        ss.page("saved-regs-fig", body=pages.saved_regs_fig)
        ss.page("push-pop", body=pages.push_pop)
        ss.page("push-pop-fig", body=pages.push_pop_fig)
        ss.page("callee-example", body=pages.callee_example)
        ss.page("soft-hard", body=pages.soft_hard)
        ss.page("win-abi", body=pages.win_abi)
        ss.page("buffer-overflow", body=pages.buffer_overflow)
        ss.page("buffer-overflow-fig", body=pages.buffer_overflow_fig)
        ss.page("canary", body=pages.canary)
        ss.page("canary-fig", body=pages.canary_fig)

lecture.bridge('第四部分：性能瓶颈与向量化\n从标量计算到向量与并发', id="bridge-part4")


with lecture.section('第四部分：性能瓶颈与向量化——从标量计算到向量与并发', id="part4") as s:
    s.page("flops-estimate", body=pages.flops_estimate)
    s.page("hw-peak", body=pages.hw_peak)
    s.page("hw-peak-2", body=pages.hw_peak_2)
    s.page("test-program", body=pages.test_program)
    s.page("test-program-2", body=pages.test_program_2)
    s.page("scalar-gap", body=pages.scalar_gap)
    s.page("scalar-gap-fig", body=pages.scalar_gap_fig)
    s.page("insn-mix", body=pages.insn_mix)
    s.page("insn-mix-2", body=pages.insn_mix_2)
    s.page("insn-mix-fig", body=pages.insn_mix_fig)
    s.page("datapath-width", body=pages.datapath_width)
    s.page("datapath-width-fig", body=pages.datapath_width_fig)
    s.page("simd-history", body=pages.simd_history)
    s.page("simd-history-fig", body=pages.simd_history_fig)
    s.page("ymm", body=pages.ymm)
    s.page("ymm-2", body=pages.ymm_2)
    s.page("vex-naming", body=pages.vex_naming)
    s.page("vex-naming-2", body=pages.vex_naming_2)
    s.page("vex-naming-fig", body=pages.vex_naming_fig)
    s.page("vector-arith", body=pages.vector_arith)
    s.page("vector-arith-2", body=pages.vector_arith_2)
    s.page("vector-arith-fig", body=pages.vector_arith_fig)
    s.page("vector-entry", body=pages.vector_entry)
    s.page("vector-entry-2", body=pages.vector_entry_2)
    s.page("vector-entry-fig", body=pages.vector_entry_fig)
    s.page("vector-loop", body=pages.vector_loop)
    s.page("vector-loop-2", body=pages.vector_loop_2)
    s.page("vector-loop-3", body=pages.vector_loop_3)
    s.page("intrinsics", body=pages.intrinsics)
    s.page("intrinsics-2", body=pages.intrinsics_2)
    s.page("intrinsics-fig", body=pages.intrinsics_fig)
    s.page("reduction", body=pages.reduction)
    s.page("reduction-2", body=pages.reduction_2)
    s.page("reduction-fig", body=pages.reduction_fig)
    s.page("perf", body=pages.perf)
    s.page("perf-2", body=pages.perf_2)
    s.page("perf-3", body=pages.perf_3)
    s.page("perf-fig", body=pages.perf_fig)
    s.page("openmp", body=pages.openmp)
    s.page("openmp-2", body=pages.openmp_2)
    s.page("openmp-fig", body=pages.openmp_fig)
    s.page("memory-wall", body=pages.memory_wall)
    s.page("memory-wall-2", body=pages.memory_wall_2)
    s.page("memory-wall-3", body=pages.memory_wall_3)
    s.page("memory-wall-4", body=pages.memory_wall_4)
    s.page("memory-wall-fig", body=pages.memory_wall_fig)
    s.page("memory-wall-5", body=pages.memory_wall_5)
    s.page("simt", body=pages.simt)
    s.page("simt-2", body=pages.simt_2)
    s.page("simt-fig", body=pages.simt_fig)
    s.page("cuda", body=pages.cuda)
    s.page("cuda-2", body=pages.cuda_2)
    s.page("cuda-fig", body=pages.cuda_fig)
    s.page("cpu-dispatch", body=pages.cpu_dispatch)
    s.page("cpu-dispatch-2", body=pages.cpu_dispatch_2)
    s.page("cpu-dispatch-3", body=pages.cpu_dispatch_3)
    s.page("cpu-dispatch-4", body=pages.cpu_dispatch_4)

lecture.bridge('全节收尾：\n总结与实验任务', id="bridge-wrapup")


with lecture.section('全节收尾：总结与实验任务', id="wrapup") as s:
    s.page("insn-summary", body=pages.insn_summary)
    s.page("insn-summary-2", body=pages.insn_summary_2)
    s.page("insn-summary-3", body=pages.insn_summary_3)
    s.page("insn-summary-fig", body=pages.insn_summary_fig)
    s.page("summary", body=pages.summary)
    s.page("summary-2", body=pages.summary_2)
    s.page("lab", body=pages.lab)
    s.page("lab-2", body=pages.lab_2)
    s.page("lab-3", body=pages.lab_3)
    s.page("lab-fig", body=pages.lab_fig)
