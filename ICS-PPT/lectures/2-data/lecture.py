"""ICS 第二讲：数据的表示。

结构安排：以上一讲留下的模型权重文件为观察对象，把「文件里的字节表示什么」
拆成四个问题，每一节回答一个。位与字节、字节序、整数与浮点各成一节，
每一节的结论都落回同一个文件。第五部分按 part-5.md 逐字照录；最后的量化专题按 quant.md 的五幕结构展开，
页面内容在 quant_pages.py。
"""

from lecturekit.dsl import Lecture

import pages
import quant_pages


lecture = Lecture(
    id="ics-data",
    title="数据的表示",
    subtitle="从一个模型权重文件的字节开始",
    ratio="16:9",
)

lecture.cover(
    "数据的表示",
    author="古金宇 · 陈榕",
    time="上海交通大学 IPADS",
)

with lecture.section("回顾与本节的问题", id="framing") as s:
    s.page("course-info", body=pages.course_info)
    s.page("ollama-intro", body=pages.ollama_intro)
    s.page("recap-weights", body=pages.recap_weights)
    s.page("machine-model", body=pages.machine_model)
    s.page("four-questions", body=pages.four_questions)

lecture.bridge("第一部分 · 位与字节\n信息的最小单位与它的书写方式")

with lecture.section("第一部分 · 位与字节", id="bits") as s:
    s.page("hexdump", body=pages.hexdump)
    s.page("bits-to-value", body=pages.bits_to_value)
    s.page("hexadecimal", body=pages.hexadecimal)
    s.page("other-formats", body=pages.other_formats)
    s.page("c-data-sizes", body=pages.c_data_sizes)
    s.page("bool-storage", body=pages.bool_storage)

lecture.bridge("第二部分 · 字节序\n多字节对象在内存中的排列")

with lecture.section("第二部分 · 字节序", id="byte-order") as s:
    s.page("memory-as-bytes", body=pages.memory_as_bytes)
    s.page("word-size", body=pages.word_size)
    s.page("endianness", body=pages.endianness)
    s.page("read-the-field", body=pages.read_the_field)
    s.page("show-bytes", body=pages.show_bytes_page)
    s.page("endianness-visible", body=pages.endianness_visible)
    s.page("text-and-tokens", body=pages.text_and_tokens)

lecture.bridge("第三部分 · 整数\n用二进制表示整数")

with lecture.section("第三部分 · 整数", id="integers") as s:
    s.page("two-readings", body=pages.two_readings)
    s.page("numeric-range", body=pages.numeric_range)
    s.page("casting", body=pages.casting)
    s.page("comparison-trap", body=pages.comparison_trap)
    s.page("kernel-bug", body=pages.kernel_bug)
    s.page("kernel-bug-answer", body=pages.kernel_bug_answer, book="merge")
    s.page("mixed-width-comparison", body=pages.mixed_width_comparison)
    s.page("bit-operations", body=pages.bit_operations)
    s.page("bit-operations-masks", body=pages.bit_operations_masks)
    s.page("bit-operations-readonly", body=pages.bit_operations_readonly)
    s.page("shifts", body=pages.shifts)
    s.page("precedence-in-practice", body=pages.precedence_in_practice)
    s.page("operator-precedence", body=pages.operator_precedence)

lecture.bridge("第四部分 · 浮点数的编码\n有限位对实数的近似")

with lecture.section("第四部分 · 浮点数的编码", id="float") as s:
    s.page("fractional-binary", body=pages.fractional_binary)
    s.page("frac-examples", body=pages.frac_examples)
    s.page("frac-to-bits", body=pages.frac_to_bits)
    s.page("ieee-history", body=pages.ieee_history)
    s.page("ieee-form", body=pages.ieee_form)
    s.page("normalized-exp", body=pages.normalized_exp)
    s.page("normalized-frac", body=pages.normalized_frac)
    s.page("normalized-example", body=pages.normalized_example)
    s.page("denormalized", body=pages.denormalized)
    s.page("special-values", body=pages.special_values)
    s.page("float-encoding", body=pages.float_encoding)
    s.page("tiny-float", body=pages.tiny_float)
    s.page("tiny-float-answer", body=pages.tiny_float_answer)

lecture.bridge("第五部分 · 浮点数的精度特性与舍入机制")

with lecture.section("第五部分 · 浮点数的精度特性与舍入机制",
                     id="float-precision") as s:
    s.page("ulp-distribution", body=pages.ulp_distribution)
    s.page("ulp-distribution-cont", body=pages.ulp_distribution_cont)
    s.page("fp-absorption", body=pages.fp_absorption)
    s.page("fp-absorption-cont", body=pages.fp_absorption_cont)
    s.page("ieee-rounding-modes", body=pages.rounding_modes)
    s.page("ieee-rounding-modes-cont", body=pages.rounding_modes_cont)
    s.page("round-bits", body=pages.round_bits)
    s.page("round-bits-cont", body=pages.round_bits_cont)
    s.page("cancellation", body=pages.cancellation)
    s.page("cancellation-cont", body=pages.cancellation_cont)
    s.page("casts-ub", body=pages.casts_ub)
    s.page("casts-ub-cont", body=pages.casts_ub_cont)
    s.page("patriot", body=pages.patriot)
    s.page("patriot-cont", body=pages.patriot_cont)
    s.page("bf16-tradeoff", body=pages.bf16_tradeoff)
    s.page("bf16-tradeoff-cont", body=pages.bf16_tradeoff_cont)
    s.page("bf16-mapping", body=pages.bf16_mapping)

lecture.bridge("第六部分 · 大模型量化的底层系统原理\n从连续实数到离散低 Bit 网格的系统级跨越")

with lecture.section("第六部分 · 大模型量化的底层系统原理", id="quant-crisis") as s:
    s.page("quant-plain-why", body=quant_pages.quant_plain_why)
    s.page("quant-plain-ruler", body=quant_pages.quant_plain_ruler)
    s.page("quant-plain-one", body=quant_pages.quant_plain_one)
    s.page("quant-plain-speed", body=quant_pages.quant_plain_speed)
    s.page("topic-cover", body=quant_pages.topic_cover)
    s.page("topic-cover-cont", body=quant_pages.topic_cover_cont)
    s.page("fp32-myth", body=quant_pages.fp32_myth)
    s.page("roofline-basics", body=quant_pages.roofline_basics)
    s.page("bandwidth-ledger", body=quant_pages.bandwidth_ledger)
    s.page("bandwidth-ledger-cont", body=quant_pages.bandwidth_ledger_cont)
    s.page("memory-wall", body=quant_pages.memory_wall)
    s.page("memory-wall-cont", body=quant_pages.memory_wall_cont)
    s.page("alu-energy", body=quant_pages.alu_energy)
    s.page("alu-energy-cont", body=quant_pages.alu_energy_cont)
    s.page("quant-overview", body=quant_pages.quant_overview)

lecture.bridge("第七部分 · 数学映射与数据分布")

with lecture.section("第七部分 · 数学映射与数据分布", id="quant-mapping") as s:
    s.page("affine-mapping", body=quant_pages.affine_mapping)
    s.page("affine-grid", body=quant_pages.affine_grid)
    s.page("zero-point-cost", body=quant_pages.zero_point_cost)
    s.page("zero-point-compute", body=quant_pages.zero_point_compute)
    s.page("symmetric-failure", body=quant_pages.symmetric_failure)
    s.page("symmetric-failure-grid", body=quant_pages.symmetric_failure_grid)
    s.page("symmetric-failure-cont", body=quant_pages.symmetric_failure_cont)
    s.page("quant-example", body=quant_pages.quant_example)
    s.page("quant-example-cont", body=quant_pages.quant_example_cont)
    s.page("quant-example-cont2", body=quant_pages.quant_example_cont2)
    s.page("quant-example-cont3", body=quant_pages.quant_example_cont3)
    s.page("clipping-tradeoff", body=quant_pages.clipping_tradeoff)

lecture.bridge("第八部分 · GGUF 家族与二级量化")

with lecture.section("第八部分 · GGUF 家族与二级量化", id="quant-gguf") as s:
    s.page("granularity-spectrum", body=quant_pages.granularity_spectrum)
    s.page("gguf-blocks", body=quant_pages.gguf_blocks)
    s.page("gguf-blocks-cont", body=quant_pages.gguf_blocks_cont)
    s.page("kquants-superblock", body=quant_pages.kquants_superblock)
    s.page("kquants-superblock-cont", body=quant_pages.kquants_superblock_cont)
    s.page("kquants-superblock-cont2", body=quant_pages.kquants_superblock_cont2)
    s.page("scale-bits", body=quant_pages.scale_bits)
    s.page("scale-bits-cont", body=quant_pages.scale_bits_cont)

lecture.bridge("第九部分 · 前沿视野与实验")

with lecture.section("第九部分 · 前沿视野与实验", id="quant-frontier") as s:
    s.page("ai-float-formats", body=quant_pages.ai_float_formats)
    s.page("nf4-lut", body=quant_pages.nf4_lut)
    s.page("lab-release", body=quant_pages.lab_release)

lecture.close("summary", body=pages.summary)
