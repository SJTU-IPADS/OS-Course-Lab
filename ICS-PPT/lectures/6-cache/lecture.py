"""局部性与缓存。

六个部分：存储技术与访存延迟，局部性与存储器层次结构，缓存模型及其访问，
写策略、多级缓存与性能指标，缓存友好的代码，大模型推理中的缓存。
问题从访存延迟开始，回答是局部性与缓存；先给出缓存的概念，再说明硬件怎样定位一个块，
最后说明程序怎样利用缓存。贯穿全讲的程序是向量内积 dot_product 与按行存储的矩阵 W[M][N]。
以 lectures/ICS2/2-5-ram、2-6-locality、2-7-cache 为素材；取舍记在 README.md。

页面函数在 pages.py 中，顺序与本文件中页面的顺序相同；函数名把下划线换成连字符就是页面的 id。
"""

from lecturekit.dsl import Lecture

import pages


lecture = Lecture(
    id="ics-ex-cache",
    title="局部性与缓存",
    subtitle="从访存延迟到缓存友好的代码",
    ratio="16:9",
)

lecture.cover(
    "局部性与缓存",
    author="古金宇 · 廖承凡",
    time="上海交通大学 IPADS",
)
lecture.page("outline", body=pages.outline)

lecture.bridge(
    "第一部分：存储技术与访存延迟",
    id="bridge-part1",
)

with lecture.section("第一部分：存储技术与访存延迟", id="part1") as s:
    s.page("dot-memory", body=pages.dot_memory)
    s.page("sram-dram", body=pages.sram_dram)
    s.page("memory-naive", body=pages.memory_naive)
    s.page("dram-read", body=pages.dram_read)
    s.page("dram-module", body=pages.dram_module)
    s.page("memory-row", body=pages.memory_row)
    s.page("read-transaction", body=pages.read_transaction)
    s.page("write-transaction", body=pages.write_transaction)
    s.page("storage-compare", body=pages.storage_compare)
    s.page("exercise-tech", body=pages.exercise_tech)
    s.page("exercise-tech-2", body=pages.exercise_tech_2)

lecture.bridge(
    "第二部分：局部性与存储器层次结构",
    id="bridge-part2",
)

with lecture.section("第二部分：局部性与存储器层次结构", id="part2") as s:
    with s.section("局部性", id="locality-section") as ss:
        ss.page("locality", body=pages.locality)
        ss.page("locality-dot", body=pages.locality_dot)
        ss.page("traverse-row", body=pages.traverse_row)
        ss.page("traverse-col", body=pages.traverse_col)
        ss.page("rowcol-run", body=pages.rowcol_run)
    with s.section("缓存的概念", id="cache-concept-section") as ss:
        ss.page("storage-levels", body=pages.storage_levels)
        ss.page("cache-concept", body=pages.cache_concept)
        ss.page("software-layers", body=pages.software_layers)
        ss.page("resource-cache", body=pages.resource_cache)
        ss.page("file-cache", body=pages.file_cache)
    with s.section("命中与不命中", id="hit-miss-section") as ss:
        ss.page("cache-hit-miss", body=pages.cache_hit_miss)
        ss.page("stride-1", body=pages.stride_1)
        ss.page("stride-k", body=pages.stride_k)
        ss.page("miss-types", body=pages.miss_types)
    s.page("cache-examples", body=pages.cache_examples)
    s.page("hierarchy-machine", body=pages.hierarchy_machine)
    s.page("exercise-locality", body=pages.exercise_locality)
    s.page("exercise-locality-2", body=pages.exercise_locality_2)

lecture.bridge(
    "第三部分：缓存模型及其访问",
    id="bridge-part3",
)

with lecture.section("第三部分：缓存模型及其访问", id="part3") as s:
    s.page("cache-between", body=pages.cache_between)
    s.page("cache-organization", body=pages.cache_organization)
    s.page("cache-address", body=pages.cache_address)
    with s.section("直接映射缓存", id="direct-mapped-section") as ss:
        ss.page("direct-mapped", body=pages.direct_mapped)
        ss.page("dm-access", body=pages.dm_access)
        ss.page("dm-trace", body=pages.dm_trace)
        ss.page("dm-replacement", body=pages.dm_replacement)
        ss.page("dm-conflict", body=pages.dm_conflict)
        ss.page("dm-index-bits", body=pages.dm_index_bits)
    with s.section("组相联缓存", id="set-associative-section") as ss:
        ss.page("sa-cache", body=pages.sa_cache)
        ss.page("sa-access", body=pages.sa_access)
        ss.page("sa-example", body=pages.sa_example)
        ss.page("sa-replacement", body=pages.sa_replacement)
    with s.section("全相联缓存", id="fully-associative-section") as ss:
        ss.page("fa-cache", body=pages.fa_cache)
    s.page("cache-compare", body=pages.cache_compare)
    s.page("linesize-method", body=pages.linesize_method)
    s.page("linesize-run", body=pages.linesize_run)
    s.page("exercise-organization", body=pages.exercise_organization)
    s.page("exercise-organization-2", body=pages.exercise_organization_2)
    s.page("exercise-organization-3", body=pages.exercise_organization_3)
    s.page("exercise-organization-4", body=pages.exercise_organization_4)

lecture.bridge(
    "第四部分：写策略、多级缓存与性能指标",
    id="bridge-part4",
)

with lecture.section("第四部分：写策略、多级缓存与性能指标", id="part4") as s:
    s.page("write-problem", body=pages.write_problem)
    s.page("write-through", body=pages.write_through)
    s.page("write-back", body=pages.write_back)
    s.page("write-allocate", body=pages.write_allocate)
    s.page("no-write-allocate", body=pages.no_write_allocate)
    s.page("icache-dcache", body=pages.icache_dcache)
    s.page("multilevel", body=pages.multilevel)
    s.page("multilevel-params", body=pages.multilevel_params)
    s.page("metrics", body=pages.metrics)
    s.page("design-factors", body=pages.design_factors)

lecture.bridge(
    "第五部分：缓存友好的代码",
    id="bridge-part5",
)

with lecture.section("第五部分：缓存友好的代码", id="part5") as s:
    s.page("friendly-rules", body=pages.friendly_rules)
    s.page("friendly-miss-rate", body=pages.friendly_miss_rate)
    s.page("matmul-def", body=pages.matmul_def)
    s.page("matmul-ijk", body=pages.matmul_ijk)
    s.page("matmul-kij", body=pages.matmul_kij)
    s.page("matmul-jki", body=pages.matmul_jki)
    s.page("matmul-run", body=pages.matmul_run)
    s.page("mountain-def", body=pages.mountain_def)
    s.page("mountain-image", body=pages.mountain_image)
    s.page("mountain-ridges", body=pages.mountain_ridges)
    s.page("mountain-slopes", body=pages.mountain_slopes)
    s.page("exercise-friendly", body=pages.exercise_friendly)
    s.page("exercise-friendly-2", body=pages.exercise_friendly_2)

lecture.bridge(
    "第六部分：大模型推理中的缓存",
    id="bridge-part6",
)

with lecture.section("第六部分：大模型推理中的缓存", id="part6") as s:
    s.page("kv-generate", body=pages.kv_generate)
    s.page("kv-attention", body=pages.kv_attention)
    s.page("kv-naive", body=pages.kv_naive)
    s.page("kv-cache", body=pages.kv_cache)
    s.page("kv-run", body=pages.kv_run)
    s.page("kv-layout", body=pages.kv_layout)
    s.page("kv-code", body=pages.kv_code)
    s.page("kv-size", body=pages.kv_size)
    s.page("kv-bandwidth", body=pages.kv_bandwidth)
    s.page("kv-gqa", body=pages.kv_gqa)
    s.page("kv-quant", body=pages.kv_quant)
    s.page("kv-evict", body=pages.kv_evict)
    s.page("kv-fragment", body=pages.kv_fragment)
    s.page("kv-paged", body=pages.kv_paged)
    s.page("kv-prefix", body=pages.kv_prefix)
    s.page("exercise-kv", body=pages.exercise_kv)
    s.page("exercise-kv-2", body=pages.exercise_kv_2)
