"""ICS 辅导课：mini-cpu 实验二（组相联缓存）的说明（目录与讲义 id 是 tut-cache-lab）。

三个部分：访存路径，cache.c 的函数与调用者，缓存的结构与访问过程，每个部分以只有标题的衔接页开始。
以为实验画的图说明实验的结构与要实现的函数，解释写在讲稿中。
2026-11-23 由 tut-quiz 的第二部分拆出，原来的三个小节成为三个部分，页面的 id 不变。

页面函数在 pages.py 中，顺序与本文件中页面的顺序相同。函数名把下划线换成连字符就是页面的 id。
"""

from lecturekit.dsl import Lecture

import pages


lecture = Lecture(
    id="tut-cache-lab",
    title="MiniCPU Lab2: Cache",
    subtitle="mini-cpu 实验二",
    ratio="16:9",
)

lecture.cover(
    "MiniCPU Lab2: Cache",
    author="古金宇 · 廖承凡",
    time="上海交通大学 IPADS",
)

lecture.bridge("第一部分：访存路径", id="bridge-cache-sec-path")

with lecture.section("第一部分：访存路径", id="cache-sec-path") as s:
    s.page("cache-lab1", body=pages.cache_lab1)
    s.page("cache-path", body=pages.cache_path)

lecture.bridge("第二部分：cache.c 的函数与调用者", id="bridge-cache-sec-funcs")

with lecture.section("第二部分：cache.c 的函数与调用者", id="cache-sec-funcs") as s:
    s.page("cache-funcs", body=pages.cache_funcs)
    s.page("cache-chain", body=pages.cache_chain)
    s.page("cache-walk", body=pages.cache_walk)

lecture.bridge("第三部分：缓存的结构与访问过程", id="bridge-cache-sec-access")

with lecture.section("第三部分：缓存的结构与访问过程", id="cache-sec-access") as s:
    s.page("cache-split", body=pages.cache_split)
    s.page("cache-grid", body=pages.cache_grid)
    s.page("cache-funcs-hit", body=pages.cache_funcs_hit)
    s.page("cache-hit", body=pages.cache_hit)
    s.page("cache-funcs-miss", body=pages.cache_funcs_miss)
    s.page("cache-funcs-evict", body=pages.cache_funcs_evict)
    s.page("cache-miss", body=pages.cache_miss)
