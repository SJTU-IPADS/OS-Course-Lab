"""ICS 第 4 章：指令如何在 CPU 中重叠执行。

开场两页之后分四部分，例子是第 3 讲的 dot_product 循环体：
第一部分 CPU 的执行模式：五个阶段（取指、译码、执行、访存、写回）与顺序执行
（单周期实现），流水线执行的原理、吞吐与它的两处限制；
第二部分 流水线执行的指令冲突：data hazard（没有转发时等写回；转发后不停的、
仍停一拍的）与跳转指令的 control hazard；
第三部分 拓展：现代 CPU：现代核在这五级之上做的三件事、为什么带宽上限不变；
第四部分 拓展：系统里的流水线：大模型推理在做什么（一次一个词、28 层、装不下、
按层切）；流水线并行与它的气泡（单请求、多请求填满、两头的占比、逐词生成的依赖、
多用户轮流）；投机推理（读一遍权重、先猜、核对、与分支预测对照、划不划算）；
其他系统里的流水线（装配线、看视频、总结）。

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

with lecture.section('第 3 讲留下的问题', id="opening") as s:
    s.page("question", body=pages.question)
    s.page("loop-body", body=pages.loop_body)

lecture.bridge('第一部分：CPU 的执行模式', id="bridge-part1")

with lecture.section('第一部分：CPU 的执行模式', id="part1") as s:
    with s.section('一条指令的五个阶段与顺序执行', id="part1-stages") as ss:
        ss.page("stages", body=pages.stages)
        ss.page("stages-fig", body=pages.stages_fig)

    s.bridge('流水线执行', id="bridge-part1-pipeline")
    with s.section('流水线执行', id="part1-pipeline") as ss:
        ss.page("stage-hw", body=pages.stage_hardware)
        ss.page("pipeline-fig", body=pages.pipeline_fig)
        ss.page("pipe-limit", body=pages.pipe_limit)

lecture.bridge('第二部分：流水线执行的指令冲突', id="bridge-part2")

with lecture.section('第二部分：流水线执行的指令冲突', id="part2") as s:
    with s.section('data hazard：后一条指令要用前一条的结果', id="part2-data") as ss:
        ss.page("forward", body=pages.forward)
        ss.page("forward-fig", body=pages.forward_fig)
        ss.page("load-use", body=pages.load_use)
        ss.page("load-use-fig", body=pages.load_use_fig)

    s.bridge('跳转指令与 control hazard', id="bridge-part2-control")
    with s.section('control hazard：跳转指令', id="part2-control") as ss:
        ss.page("branch", body=pages.branch)
        ss.page("branch-fig", body=pages.branch_fig)

lecture.bridge('第三部分：拓展：现代 CPU', id="bridge-part3")

with lecture.section('第三部分：拓展：现代 CPU', id="part3") as s:
    s.page("modern", body=pages.modern)
    s.page("bandwidth", body=pages.bandwidth)

lecture.bridge('第四部分：拓展：系统里的流水线', id="bridge-part4")

with lecture.section('第四部分：拓展：系统里的流水线', id="part4") as s:
    with s.section('大模型推理在做什么', id="part4-llm") as ss:
        ss.page("llm-what", body=pages.llm_what)
        ss.page("llm-layers", body=pages.llm_layers)
        ss.page("llm-too-big", body=pages.llm_too_big)
        ss.page("llm-split", body=pages.llm_split)

    s.bridge('流水线并行与它的气泡', id="bridge-part4-pp")
    with s.section('流水线并行与它的气泡', id="part4-pp") as ss:
        ss.page("llm-pp-single", body=pages.llm_pp_single)
        ss.page("llm-pp-fill", body=pages.llm_pp_fill)
        ss.page("llm-pp-ratio", body=pages.llm_pp_ratio)
        ss.page("llm-pp-decode", body=pages.llm_pp_decode)
        ss.page("llm-pp-batch", body=pages.llm_pp_batch)

    s.bridge('投机推理：预测执行的应用', id="bridge-part4-spec")
    with s.section('投机推理', id="part4-spec") as ss:
        ss.page("llm-decode-slow", body=pages.llm_decode_slow)
        ss.page("llm-spec-idea", body=pages.llm_spec_idea)
        ss.page("llm-spec-verify", body=pages.llm_spec_verify)
        ss.page("llm-spec-vs-branch", body=pages.llm_spec_vs_branch)
        ss.page("llm-spec-gain", body=pages.llm_spec_gain)

    s.bridge('其他系统里的流水线', id="bridge-part4-others")
    with s.section('其他系统里的流水线', id="part4-others") as ss:
        ss.page("assembly-line", body=pages.assembly_line)
        ss.page("video-pipeline", body=pages.video_pipeline)
        ss.page("pipeline-summary", body=pages.pipeline_summary)
