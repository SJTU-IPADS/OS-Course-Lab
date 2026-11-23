"""ICS 辅导课：Quiz 题目（目录与讲义 id 是 tut-quiz）。

Quiz 第 1 ~ 5 题的题面。每页一个小问或几个相互引用的小问，小问用到的代码、表与图尽量放在同一页。
第 4、5 题各是一个小节。
2026-11-23 原来的第二、三部分（mini-cpu 的组相联缓存、mini-ollama 的 KV Cache）拆成讲义
tut-cache-lab 与 tut-kvcache-lab。本讲义只有一个部分，封面之后直接是第 1 题，没有部分的衔接页。

页面函数在 pages.py 中，顺序与本文件中页面的顺序相同。函数名把下划线换成连字符就是页面的 id。
"""

from lecturekit.dsl import Lecture

import pages


lecture = Lecture(
    id="tut-quiz",
    title="Quiz 题目",
    ratio="16:9",
)

lecture.cover(
    "Quiz 题目",
    author="古金宇 · 廖承凡",
    time="上海交通大学 IPADS",
)

with lecture.section("Quiz 题目", id="quiz") as s:
    s.page("q1-code", body=pages.q1_code)
    s.page("q1-push", body=pages.q1_push)
    s.page("q1-ret", body=pages.q1_ret)
    s.page("q1-input", body=pages.q1_input)
    s.page("q2-gets", body=pages.q2_gets)
    s.page("q2-frame", body=pages.q2_frame)
    s.page("q2-pass", body=pages.q2_pass)
    s.page("q2-nx", body=pages.q2_nx)
    s.page("q2-rop", body=pages.q2_rop)
    s.page("q2-canary", body=pages.q2_canary)
    s.page("q3-convention", body=pages.q3_convention)
    s.page("q3-callee", body=pages.q3_callee)
    with s.section("第 4 题：流水线与现代 CPU（20 分）", id="quiz-q4") as q:
        q.page("q4-pipe-hazard", body=pages.q4_pipe_hazard)
        q.page("q4-pipe-cycles", body=pages.q4_pipe_cycles)
        q.page("q4-rob", body=pages.q4_rob)
        q.page("q4-rob-rules", body=pages.q4_rob_rules)
        q.page("q4-rob-q1", body=pages.q4_rob_q1)
        q.page("q4-rob-answer", body=pages.q4_rob_answer)
        q.page("q4-rob-branch", body=pages.q4_rob_branch)
    with s.section("第 5 题：性能分析与加速（30 分）", id="quiz-q5") as q:
        q.page("q5-cpu-count", body=pages.q5_cpu_count)
        q.page("q5-cpu-bound", body=pages.q5_cpu_bound)
        q.page("q5-cpu-simd", body=pages.q5_cpu_simd)
        q.page("q5-gpu-pipeline", body=pages.q5_gpu_pipeline)
        q.page("q5-gpu-batch", body=pages.q5_gpu_batch)
        q.page("q5-disk", body=pages.q5_disk)
