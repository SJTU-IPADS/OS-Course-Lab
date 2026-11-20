"""Transformer。

三个部分：GPT 的计算流程，注意力机制，KV cache。
前两部分按 3Blue1Brown 深度学习第 5、6 章的顺序展开：第一部分从预测下一个词出发，用一张流程图给出数据流的
各步计算，再按数据流的顺序逐步说明，与主线关系不强的内容放在第一部分末尾的 sidebar 小节。第二部分说明
注意力怎样按上下文更新每个词的向量。
第三部分从生成时整段重算的重复计算出发，由因果计算下旧位置不变得到逐层的 KV cache，再把复用推广到
请求之间，得到前缀缓存。取舍记在 README.md。

页面函数在 pages.py 中，顺序与本文件中页面的顺序相同，函数名把下划线换成连字符就是页面的 id。
"""

from lecturekit.dsl import Lecture

import pages


lecture = Lecture(
    id="ics-ex-transformer",
    title="Transformer",
    subtitle="从预测下一个词到 KV cache",
    ratio="16:9",
)

# Softmax 的温度，「sidebar：Softmax 的温度」一页的滑动条调节它。图 assets/temperature.svg 按默认值画出，
# 查看器按当前的 T 重新计算图中的概率与条形（diagrams/temperature.py）。
lecture.variable("T", 1, min=0, max=10)

lecture.cover(
    "Transformer",
    author="古金宇 · 廖承凡",
    time="上海交通大学 IPADS",
)

lecture.bridge('第一部分：GPT 的计算流程', id='bridge-gpt')
with lecture.section('第一部分：GPT 的计算流程', id='gpt-part') as s:
    s.bridge('什么是 GPT', id='bridge-gpt-section')
    with s.section('什么是 GPT', id='gpt-section') as ss:
        ss.page('gpt', body=pages.gpt)
        ss.page('tasks', body=pages.tasks)
        ss.page('generation', body=pages.generation)
        ss.page('chat', body=pages.chat)
    s.bridge('Transformer 的数据流', id='bridge-dataflow-section')
    with s.section('Transformer 的数据流', id='dataflow-section') as ss:
        ss.page('dataflow-overview', body=pages.dataflow_overview)
        ss.page('tokens', body=pages.tokens)
        ss.page('embedding-matrix', body=pages.embedding_matrix)
        ss.page('embedding-direction', body=pages.embedding_direction)
        ss.page('attention-preview', body=pages.attention_preview)
        ss.page('mlp-preview', body=pages.mlp_preview)
        ss.page('layer-stack', body=pages.layer_stack)
        ss.page('softmax', body=pages.softmax)
    s.bridge('sidebar', id='bridge-sidebar-section')
    with s.section('sidebar', id='sidebar-section') as ss:
        ss.page('sidebar-tower-neighbors', body=pages.sidebar_tower_neighbors)
        ss.page('sidebar-analogies', body=pages.sidebar_analogies)
        ss.page('sidebar-context-size', body=pages.sidebar_context_size)
        ss.page('sidebar-context-overflow', body=pages.sidebar_context_overflow)
        ss.page('sidebar-unembedding', body=pages.sidebar_unembedding)
        ss.page('sidebar-softmax-behavior', body=pages.sidebar_softmax_behavior)
        ss.page('sidebar-temperature', body=pages.sidebar_temperature)
        ss.page('sidebar-temperature-stories', body=pages.sidebar_temperature_stories)

lecture.bridge('第二部分：注意力机制', id='bridge-attention')
with lecture.section('第二部分：注意力机制', id='attention-part') as s:
    s.bridge('注意力要解决的问题', id='bridge-attention-goal-section')
    with s.section('注意力要解决的问题', id='attention-goal-section') as ss:
        ss.page('attention-goal', body=pages.attention_goal)
        ss.page('mole', body=pages.mole)
        ss.page('tower-refine', body=pages.tower_refine)
    s.bridge('查询、键与注意力模式', id='bridge-qk-section')
    with s.section('查询、键与注意力模式', id='qk-section') as ss:
        ss.page('block-structure', body=pages.block_structure)
        ss.page('fluffy-example', body=pages.fluffy_example)
        ss.page('query', body=pages.query)
        ss.page('key', body=pages.key)
        ss.page('attention-scores', body=pages.attention_scores)
        ss.page('attention-pattern', body=pages.attention_pattern)
        ss.page('value', body=pages.value)
        ss.page('weighted-values', body=pages.weighted_values)
        ss.page('attention-formula', body=pages.attention_formula)
        ss.page('attention-example', body=pages.attention_example)
    s.bridge('因果掩码', id='bridge-mask-section')
    with s.section('因果掩码', id='mask-section') as ss:
        ss.page('generation-gap', body=pages.generation_gap)
        ss.page('causal-mask', body=pages.causal_mask)

lecture.bridge('第三部分：KV cache', id='bridge-kv')
with lecture.section('第三部分：KV cache', id='kv-part') as s:
    s.bridge('重复计算与前缀不变性', id='bridge-recompute-section')
    with s.section('重复计算与前缀不变性', id='recompute-section') as ss:
        ss.page('recompute', body=pages.recompute)
        ss.page('prefix-invariance', body=pages.prefix_invariance)
    s.bridge('KV cache 的建立与使用', id='bridge-kv-section')
    with s.section('KV cache 的建立与使用', id='kv-section') as ss:
        ss.page('kv-cache', body=pages.kv_cache)
        ss.page('query-lifetime', body=pages.query_lifetime)
    s.bridge('前缀缓存', id='bridge-prefix-section')
    with s.section('前缀缓存', id='prefix-section') as ss:
        ss.page('request-recompute', body=pages.request_recompute)
        ss.page('prefix-tree', body=pages.prefix_tree)
        ss.page('prompt-order', body=pages.prompt_order)
        ss.page('news-anthropic-blog', body=pages.news_anthropic_blog)
        ss.page('cache-cost', body=pages.cache_cost)

lecture.close("sources", body=pages.sources)
