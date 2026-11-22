"""ICS 辅导课：Quiz 题目，mini-cpu 实验二（组相联缓存）与 kvcache 实验。

三个部分：Quiz 的题面（第 1、3、4、5 题），mini-cpu 的组相联缓存，mini-ollama 的 KV Cache。
第一部分每页一个小问或几个相互引用的小问，小问用到的代码、表与图尽量放在同一页。
第二、三部分以为实验画的图说明实验的结构与要实现的函数，解释写在讲稿中。

页面函数在 pages.py 中，顺序与本文件中页面的顺序相同。函数名把下划线换成连字符就是页面的 id。
"""

from lecturekit.dsl import Lecture

import pages


lecture = Lecture(
    id="tut-quiz",
    title="Quiz 与实验说明",
    subtitle="Quiz 题目 · 组相联缓存 · KV Cache",
    ratio="16:9",
)

lecture.cover(
    "Quiz 与实验说明",
    author="古金宇 · 廖承凡",
    time="上海交通大学 IPADS",
)

lecture.bridge("第一部分：Quiz 题目", id="bridge-quiz")

with lecture.section("第一部分：Quiz 题目", id="quiz") as s:
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

lecture.bridge("第二部分：mini-cpu 的组相联缓存", id="bridge-cache")

with lecture.section("第二部分：mini-cpu 的组相联缓存", id="cache") as s:
    s.bridge("访存路径", id="bridge-cache-sec-path")
    with s.section("访存路径", id="cache-sec-path") as ss:
        ss.page("cache-lab1", body=pages.cache_lab1)
        ss.page("cache-path", body=pages.cache_path)

    s.bridge("cache.c 的函数与调用者", id="bridge-cache-sec-funcs")
    with s.section("cache.c 的函数与调用者", id="cache-sec-funcs") as ss:
        ss.page("cache-funcs", body=pages.cache_funcs)
        ss.page("cache-walk", body=pages.cache_walk)

    s.bridge("缓存的结构与访问过程", id="bridge-cache-sec-access")
    with s.section("缓存的结构与访问过程", id="cache-sec-access") as ss:
        ss.page("cache-split", body=pages.cache_split)
        ss.page("cache-grid", body=pages.cache_grid)
        ss.page("cache-funcs-hit", body=pages.cache_funcs_hit)
        ss.page("cache-hit", body=pages.cache_hit)
        ss.page("cache-funcs-miss", body=pages.cache_funcs_miss)
        ss.page("cache-funcs-evict", body=pages.cache_funcs_evict)
        ss.page("cache-miss", body=pages.cache_miss)
        ss.page("cache-span", body=pages.cache_span)

lecture.bridge("第三部分：mini-ollama 的 KV Cache", id="bridge-kv")

with lecture.section("第三部分：mini-ollama 的 KV Cache", id="kv") as s:
    s.bridge("KV Cache 的作用", id="bridge-kv-sec-role")
    with s.section("KV Cache 的作用", id="kv-sec-role") as ss:
        ss.page("kv-sessions", body=pages.kv_sessions)
        ss.page("kv-step", body=pages.kv_step)
        ss.page("kv-reuse", body=pages.kv_reuse)

    s.bridge("KV Cache 的结构与访问过程", id="bridge-kv-sec-access")
    with s.section("KV Cache 的结构与访问过程", id="kv-sec-access") as ss:
        ss.page("kv-layout", body=pages.kv_layout)
        ss.page("kv-locate", body=pages.kv_locate)
        ss.page("kv-first", body=pages.kv_first)
        ss.page("kv-next", body=pages.kv_next)

    s.bridge("内存上限与失效", id="bridge-kv-sec-limit")
    with s.section("内存上限与失效", id="kv-sec-limit") as ss:
        ss.page("kv-memory", body=pages.kv_memory)
        ss.page("kv-reserve", body=pages.kv_reserve)
        ss.page("kv-policies", body=pages.kv_policies)
        ss.page("kv-run-time", body=pages.kv_run_time)
