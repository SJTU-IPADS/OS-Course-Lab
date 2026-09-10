"""ICS 习题课一：把 Q4_0 写出来。

结构安排：一次课的时间写完 `nano-quant` 的 A 部分与 Q4_0。第一部分把实验
对象打开——第二讲的正课只说过这个权重文件有多大，本节把它的 625 个张量、
一层的形状与字节账目逐项读出来，作为后面所有讨论的对象；第二部分说明框架
的分工；第三、四部分逐个写完六个函数；第五部分对自查的六行输出。

配套材料：题面 ../2-data/EXERCISE-q4_0.md，实验说明 ../2-data/LAB-quantize.md，
框架 ../2-data/nano-quant/。演示里的命令都在这两个目录下真跑。
"""

from lecturekit.dsl import Lecture

import pages


lecture = Lecture(
    id="ics-exe-quant",
    title="把 Q4_0 写出来",
    subtitle="习题课一 · nano-quant 的 A 部分与第一种块格式",
    ratio="16:9",
)

lecture.cover(
    "把 Q4_0 写出来",
    author="古金宇 · 陈榕",
    time="上海交通大学 IPADS",
)

with lecture.section("本节的安排", id="framing") as s:
    s.page("session-goal", body=pages.session_goal)
    s.page("session-scope", body=pages.session_scope)
    s.page("session-outline", body=pages.session_outline)

lecture.bridge("第一部分 · 实验对象\n这个文件里装着什么")

with lecture.section("第一部分 · 实验对象", id="model") as s:
    s.page("one-file", body=pages.one_file)
    s.page("st-layout", body=pages.st_layout)
    s.page("st-head-bytes", body=pages.st_head_bytes)
    s.page("st-record", body=pages.st_record)
    s.page("st-groups", body=pages.st_groups)
    s.page("two-towers", body=pages.two_towers)
    s.page("one-layer", body=pages.one_layer)
    s.page("shapes-to-hyperparams", body=pages.shapes_to_hyperparams)
    s.page("attention-shapes", body=pages.attention_shapes)
    s.page("mlp-shapes", body=pages.mlp_shapes)
    s.page("norm-tensors", body=pages.norm_tensors)
    s.page("account-before", body=pages.account_before)
    s.page("account-summary", body=pages.account_summary)

lecture.bridge("第二部分 · 代码框架\n三个程序，四份文件，一个接口")

with lecture.section("第二部分 · 代码框架", id="framework") as s:
    s.page("framework-three-programs", body=pages.framework_three_programs)
    s.page("framework-dirs", body=pages.framework_dirs)
    s.page("framework-interface", body=pages.framework_interface)
    s.page("framework-todo", body=pages.framework_todo)
    s.page("framework-groups", body=pages.framework_groups)
    s.page("framework-stream", body=pages.framework_stream)
    s.page("framework-rules", body=pages.framework_rules)

lecture.bridge("第三部分 · A 部分\n四个转换函数")

with lecture.section("第三部分 · 四个转换函数", id="part-a") as s:
    s.page("rd-u64le", body=pages.rd_u64le_page)
    s.page("rd-u64le-trap", body=pages.rd_u64le_trap)
    s.page("bf16", body=pages.bf16_page)
    s.page("bf16-memcpy", body=pages.bf16_memcpy)
    s.page("fp16-cases", body=pages.fp16_cases)
    s.page("fp16-code", body=pages.fp16_code)
    s.page("fp16-denormal", body=pages.fp16_denormal)
    s.page("f32-to-fp16-intro", body=pages.f32_to_fp16_intro)
    s.page("f32-to-fp16-normal", body=pages.f32_to_fp16_normal)
    s.page("f32-to-fp16-sub", body=pages.f32_to_fp16_sub)
    s.page("f32-to-fp16-checks", body=pages.f32_to_fp16_checks)

lecture.bridge("第四部分 · Q4_0\n三十二个权重压成十八个字节")

with lecture.section("第四部分 · Q4_0 的十八个字节", id="q4-0") as s:
    s.page("q4-0-layout", body=pages.q4_0_layout)
    s.page("q4-0-codes", body=pages.q4_0_codes)
    s.page("q4-0-pairing", body=pages.q4_0_pairing)
    s.page("q4-0-pairing-why", body=pages.q4_0_pairing_why)
    s.page("q4-0-steps", body=pages.q4_0_steps)
    s.page("q4-0-amax", body=pages.q4_0_amax)
    s.page("q4-0-scale", body=pages.q4_0_scale)
    s.page("q4-0-why-minus8", body=pages.q4_0_why_minus8)
    s.page("q4-0-reciprocal", body=pages.q4_0_reciprocal)
    s.page("q4-0-encode", body=pages.q4_0_encode)
    s.page("q4-0-clamp", body=pages.q4_0_clamp)
    s.page("q4-0-dequant", body=pages.q4_0_dequant)
    s.page("q4-0-dequant-uses", body=pages.q4_0_dequant_uses)

lecture.bridge("第五部分 · 自查\n六行输出各自说明了什么")

with lecture.section("第五部分 · 自查", id="check") as s:
    s.page("selftest-run", body=pages.selftest_run)
    s.page("selftest-meaning", body=pages.selftest_meaning)
    s.page("selftest-input", body=pages.selftest_input)
    s.page("three-mismatches", body=pages.three_mismatches)
    s.page("whole-model", body=pages.whole_model)
    s.page("half-bit", body=pages.half_bit)
    s.page("after-class", body=pages.after_class)

lecture.close("summary", body=pages.summary)
