"""Transformer。

三个部分：GPT 的数据流与词嵌入，注意力机制，KV cache。
前两部分按 3Blue1Brown 深度学习第 5、6 章的顺序展开：第一部分从预测下一个词出发，说明文本怎样变成向量、
向量怎样变回下一个词的概率分布，第二部分说明注意力怎样按上下文更新每个词的向量。
第三部分从生成时整段重算的重复计算出发，由因果计算下旧位置不变得到逐层的 KV cache。取舍记在 README.md。

页面函数在 pages.py 中，顺序与本文件中页面的顺序相同；函数名把下划线换成连字符就是页面的 id。
"""

from lecturekit.dsl import Lecture

import pages


lecture = Lecture(
    id="ics-ex-transformer",
    title="Transformer",
    subtitle="从预测下一个词到 KV cache",
    ratio="16:9",
)

lecture.cover(
    "Transformer",
    author="古金宇 · 廖承凡",
    time="上海交通大学 IPADS",
)

lecture.bridge('第一部分：GPT 的数据流与词嵌入', id='bridge-gpt')
with lecture.section('第一部分：GPT 的数据流与词嵌入', id='gpt-part') as s:
    with s.section('预测下一个词', id='predict-section') as ss:
        ss.page('gpt', body=pages.gpt)
        ss.page('tasks', body=pages.tasks)
        ss.page('generation', body=pages.generation)
        ss.page('gpt2-gpt3', body=pages.gpt2_gpt3)
    with s.section('Transformer 的数据流', id='dataflow-section') as ss:
        ss.page('tokens', body=pages.tokens)
        ss.page('token-vectors', body=pages.token_vectors)
        ss.page('attention-preview', body=pages.attention_preview)
        ss.page('mlp-preview', body=pages.mlp_preview)
        ss.page('layer-stack', body=pages.layer_stack)
        ss.page('chat', body=pages.chat)
    with s.section('深度学习的计算形式', id='dl-form-section') as ss:
        ss.page('machine-learning', body=pages.machine_learning)
        ss.page('scale', body=pages.scale)
        ss.page('tensors', body=pages.tensors)
        ss.page('weights-data', body=pages.weights_data)
        ss.page('gpt3-matrices', body=pages.gpt3_matrices)
        ss.page('weights-vs-data', body=pages.weights_vs_data)
    with s.section('词嵌入', id='embedding-section') as ss:
        ss.page('embedding-matrix', body=pages.embedding_matrix)
        ss.page('embedding-space', body=pages.embedding_space)
        ss.page('tower-neighbors', body=pages.tower_neighbors)
        ss.page('gender-direction', body=pages.gender_direction)
        ss.page('analogies', body=pages.analogies)
        ss.page('dot-product', body=pages.dot_product)
        ss.page('plural-direction', body=pages.plural_direction)
        ss.page('embedding-params', body=pages.embedding_params)
        ss.page('contextual-king', body=pages.contextual_king)
        ss.page('context-size', body=pages.context_size)
    with s.section('解嵌入与 Softmax', id='output-section') as ss:
        ss.page('unembedding', body=pages.unembedding)
        ss.page('all-predictions', body=pages.all_predictions)
        ss.page('unembedding-params', body=pages.unembedding_params)
        ss.page('softmax', body=pages.softmax)
        ss.page('softmax-behavior', body=pages.softmax_behavior)
        ss.page('temperature', body=pages.temperature)
        ss.page('temperature-stories', body=pages.temperature_stories)
        ss.page('logits', body=pages.logits)

lecture.bridge('第二部分：注意力机制', id='bridge-attention')
with lecture.section('第二部分：注意力机制', id='attention-part') as s:
    with s.section('注意力要解决的问题', id='attention-goal-section') as ss:
        ss.page('attention-goal', body=pages.attention_goal)
        ss.page('mole', body=pages.mole)
        ss.page('tower-refine', body=pages.tower_refine)
        ss.page('long-range', body=pages.long_range)
    with s.section('查询、键与注意力模式', id='qk-section') as ss:
        ss.page('fluffy-example', body=pages.fluffy_example)
        ss.page('query', body=pages.query)
        ss.page('key', body=pages.key)
        ss.page('attention-scores', body=pages.attention_scores)
        ss.page('attention-pattern', body=pages.attention_pattern)
        ss.page('attention-formula', body=pages.attention_formula)
    with s.section('掩码与注意力模式的规模', id='mask-section') as ss:
        ss.page('training-targets', body=pages.training_targets)
        ss.page('causal-mask', body=pages.causal_mask)
        ss.page('context-cost', body=pages.context_cost)
    with s.section('值与嵌入的更新', id='value-section') as ss:
        ss.page('value', body=pages.value)
        ss.page('weighted-values', body=pages.weighted_values)
    with s.section('一个注意力头的参数', id='head-params-section') as ss:
        ss.page('head-params', body=pages.head_params)
        ss.page('value-lowrank', body=pages.value_lowrank)
    with s.section('交叉注意力与多头注意力', id='multi-head-section') as ss:
        ss.page('cross-attention', body=pages.cross_attention)
        ss.page('context-kinds', body=pages.context_kinds)
        ss.page('multi-head', body=pages.multi_head)
        ss.page('block-params', body=pages.block_params)
        ss.page('output-matrix', body=pages.output_matrix)
    with s.section('多层与规模', id='layers-section') as ss:
        ss.page('deep-layers', body=pages.deep_layers)
        ss.page('attention-total', body=pages.attention_total)
        ss.page('parallel-gpu', body=pages.parallel_gpu)

lecture.bridge('第三部分：KV cache', id='bridge-kv')
with lecture.section('第三部分：KV cache', id='kv-part') as s:
    with s.section('重复计算与前缀不变性', id='recompute-section') as ss:
        ss.page('recompute', body=pages.recompute)
        ss.page('feedforward', body=pages.feedforward)
        ss.page('full-block', body=pages.full_block)
        ss.page('prefix-invariance', body=pages.prefix_invariance)
    with s.section('KV cache 的建立与使用', id='kv-section') as ss:
        ss.page('kv-cache', body=pages.kv_cache)
        ss.page('query-lifetime', body=pages.query_lifetime)
        ss.page('prefill-decode', body=pages.prefill_decode)

lecture.close("sources", body=pages.sources)
