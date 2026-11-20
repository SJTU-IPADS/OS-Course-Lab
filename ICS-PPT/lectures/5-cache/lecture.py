"""局部性与缓存。

五个部分：局部性与缓存，缓存模型及其访问，
写策略与性能指标，缓存友好的代码，大模型推理中的缓存。
问题从矩阵按行与按列求和的运行时间差异开始，回答是局部性与缓存；先给出缓存的概念，再说明硬件怎样定位一个块，
最后说明程序怎样利用缓存。矩阵相关示例使用按行存储的 W[M][N]。
以 lectures/ICS2/2-6-locality、2-7-cache 为素材；取舍记在 README.md。

页面函数在 pages.py 中，顺序与本文件中页面的顺序相同；函数名把下划线换成连字符就是页面的 id。
"""

from lecturekit.dsl import Lecture

import pages


lecture = Lecture(
    id="ics-ex-cache",
    title="局部性与缓存",
    subtitle="从矩阵遍历到缓存友好的代码",
    ratio="16:9",
)

lecture.cover(
    "局部性与缓存",
    author="古金宇 · 廖承凡",
    time="上海交通大学 IPADS",
)

lecture.bridge(
    "第一部分：局部性与缓存",
    id="bridge-part2",
)

with lecture.section("第一部分：局部性与缓存", id="part2") as s:
    with s.section("局部性与缓存", id="locality-section") as ss:
        ss.page("rowcol-code", body=pages.rowcol_code)
        ss.page("rowcol-question", body=pages.rowcol_question)
        ss.page("rowcol-run", body=pages.rowcol_run)
        ss.page("traverse-row", body=pages.traverse_row)
        ss.page("traverse-col", body=pages.traverse_col)
        ss.page("spatial-cache", body=pages.spatial_cache)
        ss.page("cache-between", body=pages.cache_between)
        ss.page("cache-features", body=pages.cache_features)
    with s.section("hit 与 miss", id="hit-miss-section") as ss:
        ss.page("cache-hit-miss", body=pages.cache_hit_miss)
        ss.page("stride-1", body=pages.stride_1)
        ss.page("stride-k", body=pages.stride_k)
        ss.page("rowcol-miss-rate", body=pages.rowcol_miss_rate)

lecture.bridge(
    "第二部分：缓存模型及其访问",
    id="bridge-part3",
)

with lecture.section("第二部分：缓存模型及其访问", id="part3") as s:
    s.page("cache-model", body=pages.cache_model)
    s.page("cache-naive", body=pages.cache_naive)
    s.page("cache-organization", body=pages.cache_organization)
    s.page("cache-address", body=pages.cache_address)
    with s.section("直接映射缓存", id="direct-mapped-section") as ss:
        ss.page("direct-mapped", body=pages.direct_mapped)
        ss.page("dm-access", body=pages.dm_access)
        ss.page("dm-trace", body=pages.dm_trace)
        ss.page("index-position-question", body=pages.index_position_question)
        ss.page("dm-index-bits", body=pages.dm_index_bits)
    with s.section("全相联缓存", id="fully-associative-section") as ss:
        ss.page("cache-line-use-question", body=pages.cache_line_use_question)
        ss.page("cache-line-use", body=pages.cache_line_use)
        ss.page("fa-cache", body=pages.fa_cache)
        ss.page("fa-cache-question", body=pages.fa_cache_question)
        ss.page("cache-compare-two", body=pages.cache_compare_two)
    with s.section("组相联缓存", id="set-associative-section") as ss:
        ss.page("sa-cache", body=pages.sa_cache)
        ss.page("sa-access", body=pages.sa_access)
        ss.page("sa-example", body=pages.sa_example)
        ss.page("sa-replacement", body=pages.sa_replacement)
    s.page("cache-compare", body=pages.cache_compare)
    s.page("assoc-levels", body=pages.assoc_levels)
    with s.section("练习与小结", id="exercise-summary-section") as ss:
        ss.page("exercise-rowcol", body=pages.exercise_rowcol)
        ss.page("exercise-rowcol-answer", body=pages.exercise_rowcol_answer)
        ss.page("exercise-rowcol-summary", body=pages.exercise_rowcol_summary)
        ss.page("cache-tradeoff", body=pages.cache_tradeoff)
        ss.page("cache-prefetch-applications", body=pages.cache_prefetch_applications)

lecture.bridge(
    "第三部分：写策略与性能指标",
    id="bridge-part4",
)

with lecture.section("第三部分：写策略与性能指标", id="part4") as s:
    with s.section("缓存副本的一致性", id="write-consistency-section") as ss:
        ss.page("copy-fail", body=pages.copy_fail)
        ss.page("write-problem", body=pages.write_problem)
    with s.section("写 hit：写回与写直达", id="write-hit-section") as ss:
        ss.page("write-back", body=pages.write_back)
        ss.page("write-through", body=pages.write_through)
        ss.page("wt-wb-compare", body=pages.wt_wb_compare)
    with s.section("写 miss：写分配与非写分配", id="write-miss-section") as ss:
        ss.page("write-allocate", body=pages.write_allocate)
        ss.page("no-write-allocate", body=pages.no_write_allocate)
    with s.section("性能指标与设计参数", id="metrics-section") as ss:
        ss.page("metrics", body=pages.metrics)
        ss.page("cache-levels", body=pages.cache_levels)
        ss.page("design-factors", body=pages.design_factors)

lecture.bridge(
    "第四部分：缓存友好的代码",
    id="bridge-part5",
)

with lecture.section("第四部分：缓存友好的代码", id="part5") as s:
    with s.section("缓存友好的规则", id="friendly-section") as ss:
        ss.page("friendly-rules", body=pages.friendly_rules)
        ss.page("friendly-miss-rate", body=pages.friendly_miss_rate)
    with s.section("矩阵乘法的循环顺序", id="matmul-section") as ss:
        ss.page("matmul-def", body=pages.matmul_def)
        ss.page("matmul-ijk", body=pages.matmul_ijk)
        ss.page("matmul-kij", body=pages.matmul_kij)
        ss.page("matmul-jki", body=pages.matmul_jki)
        ss.page("matmul-run", body=pages.matmul_run)
    with s.section("存储器山", id="mountain-section") as ss:
        ss.page("mountain-def", body=pages.mountain_def)
        ss.page("mountain-image", body=pages.mountain_image)
        ss.page("mountain-ridges", body=pages.mountain_ridges)
        ss.page("mountain-slopes", body=pages.mountain_slopes)
        ss.page("mountain-prefetch", body=pages.mountain_prefetch)

lecture.bridge(
    "第五部分：大模型推理中的缓存",
    id="bridge-part6",
)

with lecture.section("第五部分：大模型推理中的缓存", id="part6") as s:
    with s.section("相亲节目", id="party-section") as ss:
        ss.page("kv-party-design", body=pages.kv_party_design)
        ss.page("kv-party-flow", body=pages.kv_party_flow)
        ss.page("kv-party-info", body=pages.kv_party_info)
        ss.page("kv-party-read", body=pages.kv_party_read)
        ss.page("kv-party-invite", body=pages.kv_party_invite)
        ss.page("kv-party-enter", body=pages.kv_party_enter)
        ss.page("kv-party-note", body=pages.kv_party_note)
    with s.section("大模型的文本生成与注意力", id="attention-section") as ss:
        ss.page("kv-generate", body=pages.kv_generate)
        ss.page("kv-encoding", body=pages.kv_encoding)
        ss.page("kv-token-vector", body=pages.kv_token_vector)
        ss.page("kv-qkv", body=pages.kv_qkv)
        ss.page("kv-qkv-steps", body=pages.kv_qkv_steps)
        ss.page("kv-predict", body=pages.kv_predict)
        ss.page("kv-attention", body=pages.kv_attention)
        ss.page("kv-features", body=pages.kv_features)
    with s.section("KV Cache", id="kv-cache-section") as ss:
        ss.page("kv-naive", body=pages.kv_naive)
        ss.page("kv-cache", body=pages.kv_cache)
        ss.page("kv-run", body=pages.kv_run)
    with s.section("显存中的 KV Cache 管理", id="kv-memory-section") as ss:
        ss.page("kv-vram", body=pages.kv_vram)
        ss.page("kv-expire", body=pages.kv_expire)
        ss.page("kv-evict", body=pages.kv_evict)
        ss.page("kv-swap", body=pages.kv_swap)
        ss.page("kv-recompute", body=pages.kv_recompute)
