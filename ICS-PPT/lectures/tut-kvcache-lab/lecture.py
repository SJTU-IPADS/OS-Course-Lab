"""ICS 辅导课：kvcache 实验（mini-ollama 的 KV Cache）的说明（目录与讲义 id 是 tut-kvcache-lab）。

两个部分：KV Cache 的作用，KV Cache 的结构与访问过程，每个部分以只有标题的衔接页开始。
以为实验画的图说明实验的结构与要实现的函数，解释写在讲稿中。
2026-11-23 由 tut-quiz 的第三部分拆出，原来的前两个小节成为两个部分，页面的 id 不变。原来第二小节的
最后一页（kv-next）与第三小节「内存上限与失效」同时删去，副本在 build/removed-tut-quiz-kv-next-limit/。
同日实验改为一段多轮对话与一段连续的内存，kv-sessions 改为 kv-turns，第二部分的三页随之改写。

页面函数在 pages.py 中，顺序与本文件中页面的顺序相同。函数名把下划线换成连字符就是页面的 id。
"""

from lecturekit.dsl import Lecture

import pages


lecture = Lecture(
    id="tut-kvcache-lab",
    title="Handson-Lab KVCache",
    subtitle="kvcache 实验",
    ratio="16:9",
)

lecture.cover(
    "Handson-Lab KVCache",
    author="古金宇 · 廖承凡",
    time="上海交通大学 IPADS",
)

lecture.bridge("第一部分：KV Cache 的作用", id="bridge-kv-sec-role")

with lecture.section("第一部分：KV Cache 的作用", id="kv-sec-role") as s:
    s.page("kv-turns", body=pages.kv_turns)
    s.page("kv-step", body=pages.kv_step)
    s.page("kv-reuse", body=pages.kv_reuse)

lecture.bridge("第二部分：KV Cache 的结构与访问过程", id="bridge-kv-sec-access")

with lecture.section("第二部分：KV Cache 的结构与访问过程", id="kv-sec-access") as s:
    s.page("kv-layout", body=pages.kv_layout)
    s.page("kv-locate", body=pages.kv_locate)
    s.page("kv-first", body=pages.kv_first)
