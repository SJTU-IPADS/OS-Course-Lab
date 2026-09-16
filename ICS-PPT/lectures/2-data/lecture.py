"""ICS 第二讲：数据的表示。

结构安排：以上一讲留下的模型权重文件为观察对象，把「文件里的字节表示什么」
拆成四个问题，每一节回答一个。位与字节、字节序、整数、浮点与量化各成一节，
每一节的结论都落回同一个文件，最后一页收束到四个答案。
"""

from lecturekit.dsl import Lecture

import pages


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
    s.page("shifts", body=pages.shifts)
    s.page("operator-precedence", body=pages.operator_precedence)
    s.page("precedence-in-practice", body=pages.precedence_in_practice)

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

lecture.bridge("第五部分 · 浮点数的精度与低精度格式\n可表示值的分布、舍入，以及更少位的浮点格式")

with lecture.section("第五部分 · 浮点数的精度与低精度格式", id="float-formats") as s:
    s.page("float-distribution", body=pages.float_distribution)
    s.page("float-rounding", body=pages.float_rounding)
    s.page("rounding-modes", body=pages.rounding_modes)
    s.page("binary-rounding", body=pages.binary_rounding)
    s.page("patriot-missile", body=pages.patriot_missile)
    s.page("float-not-real", body=pages.float_not_real)
    s.page("float-casts", body=pages.float_casts)
    s.page("precision-formats", body=pages.precision_formats)
    s.page("bf16-truncation", body=pages.bf16_truncation)
    s.page("bf16-classes", body=pages.bf16_classes)
    s.page("range-and-precision", body=pages.range_and_precision)
    s.page("model-size", body=pages.model_size)

lecture.bridge("第六部分 · 量化\n用更少的位存放同一组权重")

with lecture.section("第六部分 · 量化：原理与 Q4_0", id="quantization") as s:
    s.page("why-quantize", body=pages.why_quantize)
    s.page("memory-bound-measured", body=pages.memory_bound_measured)
    s.page("what-to-quantize", body=pages.what_to_quantize)
    s.page("quantization-idea", body=pages.quantization_idea)
    s.page("quantization-map", body=pages.quantization_map)
    s.page("q4-block", body=pages.q4_block)
    s.page("quantize-code", body=pages.quantize_code)
    s.page("nibble-packing", body=pages.nibble_packing)
    s.page("nibble-add", body=pages.nibble_add)
    s.page("int4-hardware", body=pages.int4_hardware)
    s.page("granularity", body=pages.granularity)
    s.page("granularity-measured", body=pages.granularity_measured)

lecture.bridge("第七部分 · 量化格式\n偏移、两级缩放，以及混合量化")

with lecture.section("第七部分 · 量化格式：Q4_1 与 Q4_K", id="quant-formats") as s:
    s.page("zero-point", body=pages.zero_point)
    s.page("q4-1-measured", body=pages.q4_1_measured)
    s.page("superblock", body=pages.superblock)
    s.page("q4-k-budget", body=pages.q4_k_budget)
    s.page("k-scales-layout", body=pages.k_scales_layout)
    s.page("k-scales-code", body=pages.k_scales_code)
    s.page("q4-k-measured", body=pages.q4_k_measured)
    s.page("mixed-recipe", body=pages.mixed_recipe)
    s.page("quantization-cost", body=pages.quantization_cost)
    s.page("quantization-methods", body=pages.quantization_methods)
    s.page("lab-preview", body=pages.lab_preview)

lecture.close("summary", body=pages.summary)
