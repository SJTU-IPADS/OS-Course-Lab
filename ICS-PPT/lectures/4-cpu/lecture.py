"""ICS 第 4 章：指令如何在 CPU 中重叠执行。

开场一页之后分四部分，例子是第 3 讲的 dot_product 循环体：
第一部分 CPU 的执行模式：五个阶段（取指、译码、执行、访存、写回）与顺序执行
（单周期实现），流水线执行的原理、吞吐与它的两处限制；
第二部分 流水线执行的指令冲突：data hazard（不处理时读到旧值；stall 与 bubble，
stall 到写回之后；转发后不停的、仍 stall 一个周期的）与跳转指令的 control hazard
（取指不知道取哪条，等待判定；按猜测取指的分支预测，猜对与猜错）；
第三部分 现代 CPU：现代核在这五级之上做的四件事：乱序执行（读内存要 3 个周期时
顺序流水线的等待、执行级空闲、无关指令先执行、乱序执行的时序、与顺序流水线对比），
超标量一页，超线程（一个线程用不满执行部件、硬件结构、两个线程同时执行、填上空着的
执行部件、与超标量和多核对照），硬件分支预测器与一页实测（跳转方向无规律时运行慢）；
第四部分 拓展：系统里的流水线：大模型推理在做什么（一次一个词、28 层、装不下、
按层切）；流水线并行与它的 bubble（单请求、多请求填满）；投机推理（读一遍权重、先猜、为什么能一次核对、从左到右比对、
与分支预测对照、划不划算）；
其他系统里的流水线（看视频、总结）。

不讲 Y86、HCL、顺序实现的电路和五级转发通路。页面函数在 pages.py。
"""

from lecturekit.dsl import Lecture

import pages


lecture = Lecture(
    id="ics-cpu",
    title="CPU 执行阶段与流水线",
    subtitle="从一条 movl 到流水线",
    ratio="16:9",
)

lecture.cover(
    "CPU 执行阶段与流水线",
    author="古金宇 · 廖承凡",
    time="上海交通大学 IPADS",
)

with lecture.section('问题引入', id="opening") as s:
    s.page("question", body=pages.question)

lecture.bridge('第一部分：CPU 的执行模式', id="bridge-part1")

with lecture.section('第一部分：CPU 的执行模式', id="part1") as s:
    with s.section('指令的五个阶段', id="part1-stages") as ss:
        ss.page("stages", body=pages.stages)
        ss.page("stage-units", body=pages.stage_units)
        ss.page("clock", body=pages.clock_cycle)

    s.bridge('顺序执行', id="bridge-part1-seq")
    with s.section('顺序执行', id="part1-seq") as ss:
        ss.page("insn-flow", body=pages.insn_flow)
        ss.page("op-stages", body=pages.op_stages)
        ss.page("op-table", body=pages.op_table)
        ss.page("op-example", body=pages.op_example)
        ss.page("stages-fig", body=pages.stages_fig)
        ss.page("cycle-start", body=pages.cycle_start)
        ss.page("naive-design", body=pages.naive_design)

    s.bridge('流水线执行', id="bridge-part1-pipeline")
    with s.section('流水线执行', id="part1-pipeline") as ss:
        ss.page("car-wash", body=pages.car_wash)
        ss.page("comp-example", body=pages.comp_example)
        ss.page("pipe-3way", body=pages.pipe_3way)
        ss.page("pipe-diagram", body=pages.pipe_diagram)
        ss.page("stage-hw", body=pages.stage_hardware)
        ss.page("pipeline-fig", body=pages.pipeline_fig)
        ss.page("pipe-limit", body=pages.pipe_limit)
        ss.page("pipe-practice", body=pages.pipe_practice)
        ss.page("pipe-practice-answer", body=pages.pipe_practice_answer)
        ss.page("reg-overhead-fig", body=pages.reg_overhead_fig)

lecture.bridge('第二部分：流水线执行的指令冲突', id="bridge-part2")

with lecture.section('第二部分：流水线执行的指令冲突', id="part2") as s:
    with s.section('data hazard', id="part2-data") as ss:
        ss.page("hazard", body=pages.hazard)
        ss.page("stall-bubble", body=pages.stall_bubble)
        ss.page("stall-wait", body=pages.stall_wait)
        ss.page("forward-fig", body=pages.forward_fig)
        ss.page("load-use", body=pages.load_use)
        ss.page("load-use-fig", body=pages.load_use_fig)

    s.bridge('control hazard 与分支预测', id="bridge-part2-control")
    with s.section('control hazard 与分支预测', id="part2-control") as ss:
        ss.page("branch", body=pages.branch)
        ss.page("branch-guess", body=pages.branch_guess)
        ss.page("branch-ok", body=pages.branch_ok)
        ss.page("branch-fig", body=pages.branch_fig)

lecture.bridge('第三部分：现代 CPU', id="bridge-part3")

with lecture.section('第三部分：现代 CPU', id="part3") as s:
    s.page("ooo-inorder", body=pages.ooo_inorder)
    s.page("ooo-idle", body=pages.ooo_idle)
    s.page("ooo-independent", body=pages.ooo_independent)
    s.page("out-of-order", body=pages.out_of_order)
    s.page("ooo-compare", body=pages.ooo_compare)
    s.page("superscalar", body=pages.superscalar)
    s.page("smt-idle", body=pages.smt_idle)
    s.page("smt-hw", body=pages.smt_hw)
    s.page("smt-run", body=pages.smt_run)
    s.page("smt-fill", body=pages.smt_fill)
    s.page("smt-vs", body=pages.smt_vs)
    s.page("branch-predictor", body=pages.branch_predictor)
    s.page("branch-time", body=pages.branch_time)

lecture.bridge('第四部分：拓展：系统里的流水线', id="bridge-part4")

with lecture.section('第四部分：拓展：系统里的流水线', id="part4") as s:
    with s.section('大模型推理在做什么', id="part4-llm") as ss:
        ss.page("llm-what", body=pages.llm_what)
        ss.page("llm-layers", body=pages.llm_layers)
        ss.page("llm-too-big", body=pages.llm_too_big)
        ss.page("llm-split", body=pages.llm_split)

    s.bridge('流水线并行与它的 bubble', id="bridge-part4-pp")
    with s.section('流水线并行与它的 bubble', id="part4-pp") as ss:
        ss.page("llm-pp-single", body=pages.llm_pp_single)
        ss.page("llm-pp-fill", body=pages.llm_pp_fill)

    s.bridge('投机推理：预测执行的应用', id="bridge-part4-spec")
    with s.section('投机推理', id="part4-spec") as ss:
        ss.page("llm-decode-slow", body=pages.llm_decode_slow)
        ss.page("llm-spec-idea", body=pages.llm_spec_idea)
        ss.page("llm-spec-why", body=pages.llm_spec_why)
        ss.page("llm-spec-verify", body=pages.llm_spec_verify)
        ss.page("llm-spec-vs-branch", body=pages.llm_spec_vs_branch)
        ss.page("llm-spec-gain", body=pages.llm_spec_gain)

    s.bridge('其他系统里的流水线', id="bridge-part4-others")
    with s.section('其他系统里的流水线', id="part4-others") as ss:
        ss.page("video-pipeline", body=pages.video_pipeline)
        ss.page("pipeline-summary", body=pages.pipeline_summary)
