"""ICS 第 4 章：指令如何在 CPU 中重叠执行。

一讲四段，例子是第 3 讲的 dot_product 循环体：
五个阶段（取指、译码、执行、访存、写回），流水线的吞吐与它的两处限制，
程序员能看见的两类冒险（可转发的相关、load 后立即使用、分支），
现代核在这五级之上做的三件事，以及为什么这仍解释不了带宽墙。

不讲 Y86、HCL、顺序实现的电路和五级转发通路。页面函数在 pages.py。
"""

from lecturekit.dsl import Lecture

import pages


lecture = Lecture(
    id="ics-cpu",
    title="指令如何在 CPU 中重叠执行",
    subtitle="从一条 movl 到流水线",
    ratio="16:9",
)

lecture.cover(
    "指令如何在 CPU 中重叠执行",
    author="古金宇 · 陈榕",
    time="上海交通大学 IPADS",
)

with lecture.section('第 3 讲留下的问题', id="opening") as s:
    s.page("question", body=pages.question)
    s.page("loop-body", body=pages.loop_body)

lecture.bridge('一条指令的五个阶段', id="bridge-stages")

with lecture.section('一条指令的五个阶段', id="part-stages") as s:
    s.page("stages", body=pages.stages)
    s.page("stages-fig", body=pages.stages_fig)

lecture.bridge('流水线改变的是吞吐，不是一条指令的延迟', id="bridge-pipeline")

with lecture.section('流水线', id="part-pipeline") as s:
    s.page("pipeline", body=pages.pipeline)
    s.page("pipeline-fig", body=pages.pipeline_fig)
    s.page("pipe-limit", body=pages.pipe_limit)

lecture.bridge('重叠之后，后一条指令会等前一条', id="bridge-hazard")

with lecture.section('两类冒险', id="hazard") as s:
    s.page("forward", body=pages.forward)
    s.page("forward-fig", body=pages.forward_fig)
    s.page("load-use", body=pages.load_use)
    s.page("load-use-fig", body=pages.load_use_fig)
    s.page("branch", body=pages.branch)
    s.page("branch-fig", body=pages.branch_fig)

lecture.bridge('现代的核在这五级之上还做三件事', id="bridge-modern")

with lecture.section('现代的核', id="part-modern") as s:
    s.page("modern", body=pages.modern)
    s.page("bandwidth", body=pages.bandwidth)

lecture.bridge('下一讲：存储层次', id="bridge-next")

with lecture.section('收束', id="close") as s:
    s.page("next-lecture", body=pages.next_lecture)
