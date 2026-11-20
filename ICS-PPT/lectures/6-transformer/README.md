# Transformer

独立的一讲。前两部分大体按 3Blue1Brown 深度学习第 5 章（[But what is a GPT?](https://www.youtube.com/watch?v=wjZofJX0v4M)，[图文版](https://www.3blue1brown.com/lessons/gpt)）与第 6 章（[Attention in transformers, step-by-step](https://www.youtube.com/watch?v=eMlx5fFNoYc)，[图文版](https://www.3blue1brown.com/lessons/attention)）的顺序展开（少数页调整了位置，另有视频中没有的补充页），内容、例子与 GPT-3 的结构数字与视频一致。第三部分是老师要求补充的 KV cache。

中文是基线（写在 Python 里），英文是 `i18n/en.toml` 翻译覆盖层。

## 结构

封面之后分三个部分，每部分以一页 `lecture.bridge(...)` 开头，只写「第 x 部分：标题」。每个小节也以一页过渡页（`s.bridge(...)`）开头，只写小节标题。本讲不设练习页。第一部分的最后一个小节名为「sidebar」，与其他小节一样以过渡页开头，其中各页的标题写作「sidebar：…」，顺序与它们展开的主线页相同。第二、三部分以最后一个小节结束。页面函数在 `pages.py` 中，顺序与 `lecture.py` 相同，函数名把下划线换成连字符就是页面的 id。每个部分在 `pages.py` 中以一段注释开头，写明起点问题、回答与页序。每个页面函数的注释写明它怎样承接上一页。

共 87 张幻灯片：45 个页面函数，其中 14 组动画（`p.frames`），每帧一张。超过十页的部分按主要内容分成小节（`with s.section(...)`）。

| 部分 / 小节 | 页 | 页面 |
| --- | --- | --- |
| **第一部分：GPT 的计算流程** | 2–30 | |
| 什么是 GPT | 3–11 | 生成式预训练 Transformer · Transformer 的任务 · 预测、采样与追加 · 续写与对话 |
| Transformer 的数据流 | 12–20 | Transformer 的计算流程 · 什么是 token · 嵌入矩阵 · 嵌入空间中的方向与点积 · 注意力块：向量之间的信息传递 · 多层感知机：逐个向量的变换 · 多层 Transformer · Softmax |
| sidebar | 21–30 | sidebar：词向量的近邻 · sidebar：词向量的类比 · sidebar：上下文长度 · sidebar：上下文超出上限的处理方法 · sidebar：解嵌入矩阵与 logits · sidebar：Softmax 的取值特点 · sidebar：Softmax 的温度 · sidebar：温度对生成文本的影响 |
| **第二部分：注意力机制** | 31–66 | |
| 注意力要解决的问题 | 32–37 | 注意力机制的目标 · 同一个词的不同含义 · 示例：逐步具体化 tower |
| 查询、键与注意力模式 | 38–63 | 注意力块的结构 · 一个注意力头的例子 · 查询向量 · 键向量 · 键与查询的点积表 · 注意力模式 · 值向量 · 值的加权求和 · 注意力的矩阵公式 · 注意力的计算示例 |
| 因果掩码 | 64–66 | 推理中的下文缺失 · 因果掩码 |
| **第三部分：KV cache** | 67–86 | |
| 重复计算与前缀不变性 | 68–73 | 逐步生成中的重复计算 · 因果计算的前缀不变性 |
| KV cache 的建立与使用 | 74–79 | 逐层保存的 KV cache · 查询与键值的使用范围 |
| 前缀缓存 | 80–86 | 请求之间的重复计算 · 前缀缓存 · 固定前缀 · 新闻：Anthropic 博客 · 填满上下文的费用 |
| **参考资料** | 87 | |

页码随修改变化，以上是 2026-11-19 的页码。

第一部分的页序：先由预测下一个词得到生成的循环，即预测、采样、追加，续写与对话都使用这个循环。再用一页流程图给出数据流的各步与每一步的主要计算：嵌入是查表，注意力块与多层感知机交替多层，主要是矩阵乘法，其中只有注意力块在位置之间传递信息，最后是解嵌入与 Softmax。之后按数据流的顺序逐页说明各步：token、嵌入矩阵、嵌入空间中的方向与点积、注意力块、多层感知机、多层的堆叠、Softmax。与主线关系不强的内容放在本部分末尾的 sidebar 小节。

第三部分的页序：推理时每采样一个 token 就追加到输入，整段重新运行会重做旧位置在每一层的计算，由此提出问题：这些结果会因新 token 而改变吗。为回答它，逐个看一层中的运算读取哪些位置：多层感知机逐位置进行，只有注意力读取其他位置，而它受因果掩码限制。由此得到前缀不变性，旧位置各层的 K、V 可以逐层保存，即 KV cache。之后说明只缓存 K、V 的原因。最后一个小节把复用推广到请求之间：KV cache 只在一个请求之内使用，请求结束后释放，多轮对话的下一轮与使用同一段系统提示词的不同请求以相同的前缀开头，这些前缀的 K、V 被重新计算，由此提出问题：所有请求共用的系统提示词，K、V 能否只计算一次。下一页「前缀缓存」回答它，并给出前缀缓存的定义：由前缀不变性，两个输入从第一个 token 起相同的部分各层的 K、V 也相同，因此保留已结束请求的 KV cache 供后来的请求复用，复用只到第一个不同的 token 为止。这一页的图是前缀树，没有另设一页逐层画出复用的 K、V：缓存的各个序列按公共前缀组成一棵前缀树，显存放满时按 LRU 淘汰叶结点。之后「固定前缀」一页由复用只到第一个不同的 token 为止得出推论：Agent harness 的一种实现在每次请求的系统提示词中写入当前时间，时间之后的内容全部无法复用，所以设计 Agent harness 时应当尽量固定前缀，变化的内容放在末尾。之后「新闻：Anthropic 博客」一页是一则趣闻，说明这种情形在 Claude Code 中发生过：右侧是博客中 Claude Code 的提示词布局图，左侧一行引出之后引用博客的英文原文，Claude Code 曾经在静态的系统提示词中写入详细的时间戳而破坏这个顺序。讲稿补充博客的做法，即把更新的信息写进下一条用户消息或工具的结果中。小节以「填满上下文的费用」一页结束：按 Requesty 对 Claude Code 的统计（请求平均输入 84,000 个 token，缓存命中率 92%）估算每个请求约新增 6,700 个 token，填满 300,000 个 token 的上下文约需 45 个请求，再按 Claude Fable 5.2 的价格计算这 45 个请求输入的费用，没有 KV cache 时为 $55.20，每次都命中时为 $4.32，相差约 13 倍。

## 视频与页面的对应

页面上不标注视频的时间段。下表列出前两部分每页对应的时间段，`pages.py` 中每个页面函数上方的注释也写有它（`# 视频：第 x 章 mm:ss–mm:ss。`）。页码随修改变化。

| 页 | 小节 | 页面 | 视频 |
| --- | --- | --- | --- |
| 4 | 什么是 GPT | 生成式预训练 Transformer（`gpt`） | 第 5 章 00:00–00:35 |
| 5 | 什么是 GPT | Transformer 的任务（`tasks`） | 第 5 章 00:35–01:45 |
| 6–10 | 什么是 GPT | 预测、采样与追加（`generation`） | 第 5 章 01:45–02:10 |
| 11 | 什么是 GPT | 续写与对话（`chat`） | 第 5 章 05:38–06:35 |
| 13 | Transformer 的数据流 | Transformer 的计算流程（`dataflow-overview`） | 无对应片段 |
| 14 | Transformer 的数据流 | 什么是 token（`tokens`） | 第 5 章 03:03–03:37 |
| 15 | Transformer 的数据流 | 嵌入矩阵（`embedding-matrix`） | 第 5 章 03:37–03:55 与 12:27–13:27 |
| 16 | Transformer 的数据流 | 嵌入空间中的方向与点积（`embedding-direction`） | 第 5 章 14:58–15:50 与 16:34–17:06 |
| 17 | Transformer 的数据流 | 注意力块：向量之间的信息传递（`attention-preview`） | 第 5 章 03:55–04:29 |
| 18 | Transformer 的数据流 | 多层感知机：逐个向量的变换（`mlp-preview`） | 第 5 章 04:29–05:13，Michael Jordan 一例取自第 7 章 |
| 19 | Transformer 的数据流 | 多层 Transformer（`layer-stack`） | 第 5 章 05:13–05:38 |
| 20 | Transformer 的数据流 | Softmax（`softmax`） | 第 5 章 20:23–21:15 中解嵌入的一句与 22:22–23:30 |
| 22 | sidebar | sidebar：词向量的近邻（`sidebar-tower-neighbors`） | 第 5 章 14:21–14:58 |
| 23 | sidebar | sidebar：词向量的类比（`sidebar-analogies`） | 第 5 章 15:50–16:34 |
| 24 | sidebar | sidebar：上下文长度（`sidebar-context-size`） | 第 5 章 19:24–20:23 |
| 25 | sidebar | sidebar：上下文超出上限的处理方法（`sidebar-context-overflow`） | 无对应片段 |
| 26 | sidebar | sidebar：解嵌入矩阵与 logits（`sidebar-unembedding`） | 第 5 章 20:23–21:15，logits 的名称取自 25:33–26:03 |
| 27–28 | sidebar | sidebar：Softmax 的取值特点（`sidebar-softmax-behavior`） | 第 5 章 23:30–23:55 |
| 29 | sidebar | sidebar：Softmax 的温度（`sidebar-temperature`） | 第 5 章 23:55–24:33 |
| 30 | sidebar | sidebar：温度对生成文本的影响（`sidebar-temperature-stories`） | 第 5 章 24:33–25:33 |
| 33 | 注意力要解决的问题 | 注意力机制的目标（`attention-goal`） | 第 6 章 00:00–01:46 |
| 34 | 注意力要解决的问题 | 同一个词的不同含义（`mole`） | 第 6 章 01:46–02:57 |
| 35–37 | 注意力要解决的问题 | 示例：逐步具体化 tower（`tower-refine`） | 第 6 章 02:57–03:29 |
| 39 | 查询、键与注意力模式 | 注意力块的结构（`block-structure`） | 第 6 章 20:31–22:03 的多头结构，提前作整体说明 |
| 40–41 | 查询、键与注意力模式 | 一个注意力头的例子（`fluffy-example`） | 第 6 章 04:29–06:14 |
| 42–44 | 查询、键与注意力模式 | 查询向量（`query`） | 第 6 章 06:14–07:41 |
| 45–48 | 查询、键与注意力模式 | 键向量（`key`） | 第 6 章 07:41–08:17 |
| 49–50 | 查询、键与注意力模式 | 键与查询的点积表（`attention-scores`） | 第 6 章 08:17–09:29 |
| 51–53 | 查询、键与注意力模式 | 注意力模式（`attention-pattern`） | 第 6 章 09:29–10:18 |
| 54 | 查询、键与注意力模式 | 值向量（`value`） | 第 6 章 13:10–14:39 |
| 55–58 | 查询、键与注意力模式 | 值的加权求和（`weighted-values`） | 第 6 章 14:39–15:49 |
| 59 | 查询、键与注意力模式 | 注意力的矩阵公式（`attention-formula`） | 第 6 章 10:18–11:05 |
| 60–63 | 查询、键与注意力模式 | 注意力的计算示例（`attention-example`） | 无对应片段 |
| 65 | 因果掩码 | 推理中的下文缺失（`generation-gap`） | 无对应片段 |
| 66 | 因果掩码 | 因果掩码（`causal-mask`） | 第 6 章 11:49–12:42 |

视频中没有放进讲义的片段：第 5 章 02:10–02:55 的模型规模对模型表现的影响、06:35–07:12 的系列安排、07:12–12:27 的深度学习的计算形式、13:27–14:21 的在低维子空间里投影高维向量（只在「sidebar：词向量的近邻」的讲稿中用一句说明）、14:58–15:50 中五对家庭关系词的例子（已删去）、17:06–17:50 的复数方向、17:50–18:25 的嵌入矩阵的参数量、18:25–19:24 的吸收上下文的嵌入、21:15–21:45 的每个位置的预测、21:45–22:22 的解嵌入矩阵的参数量、26:03–26:36 的结尾回顾、26:36–27:14 的发布说明，第 6 章 03:29–04:29 的推理小说一例、11:05–11:49 的每个位置的预测目标、12:42–13:10 的注意力模式的规模、15:49–16:24 的查询矩阵与键矩阵的参数量、16:24–18:11 的值映射的低秩分解、18:11–19:12 的交叉注意力、19:12–20:31 的多种上下文关系、22:03–22:16 的多头注意力块的参数量、22:16–23:11 的输出矩阵、23:11–24:16 的多层中的上下文、24:16–24:54 的注意力参数的总量、24:54–25:22 的注意力的可并行性。第 6 章 25:22–26:09 推荐的资料列在「参考资料」页。

## 例子与数值的来源

画面上的文字与数值取自 3Blue1Brown 的 Manim 源码（[3b1b/videos `_2024/transformers`](https://github.com/3b1b/videos/tree/master/_2024/transformers)）与图文版的插图：生成例子的起始文本与 12 个候选的百分数（取整数部分）、分词的句子、聊天的系统提示词、tower 的近邻、性别方向的 king、queen、woman、man 一例、Italy/Germany 与 sushi/bratwurst 的类比、Snape 一例的候选与概率、Softmax 与温度的输入、mole 与 tower 的短语。tower 的近邻、性别方向与类比这几个嵌入几何的例子，在 Manim 源码中用 50 维的 GloVe 词向量计算（gensim 的 `glove-wiki-gigaword-50`），没有使用 GPT 的嵌入矩阵 $W_E$。GPT-3 的结构数字（词表 50,257、嵌入维数 12,288、上下文 2,048、每个注意力块 96 个头、键查询空间 128）与视频一致。

多层感知机一页的 Michael Jordan 与篮球一例，以及多层感知机约占 GPT-3 参数三分之二的说法，取自第 7 章的图文版（[How might LLMs store facts](https://www.3blue1brown.com/lessons/mlp)）。

视频中没有保存的内容不编造：两种温度的故事文本画成文字条，只标出旁白说出的内容。

注意力一例（a fluffy blue creature roamed the verdant forest）的分数写在 `diagrams/attn_data.py` 中，为手设的小数值：名词的查询与修饰它的形容词的键匹配，其余配对很小，creature 一列归一化后与视频相同，为 fluffy 0.42、blue 0.58。视频中的原始分数约为 ±100，未缩放。

「注意力的计算示例」一页不取自视频：两个 token（fluffy、creature）的嵌入维数为 4，嵌入与 $W_Q$、$W_K$、$W_V$ 是手设的小整数，写在 `diagrams/attention-example.py` 中，布局与前面的表相同，每个 token 占一列。`check_math.py` 重新计算这个例子，并核对页面上的两位小数。

注意力的表采用视频的方向：键为行、查询为列，Softmax 逐列进行。公式页以论文的形式 $\mathrm{softmax}(QK^T/\sqrt{d_k})V$ 为主（每个 token 占一行），并写出表的方向下的转置形式 $V\,\mathrm{softmax}(K^TQ/\sqrt{d_k})$。视频画面写作 $\mathrm{softmax}(K^TQ/\sqrt{d_k})V$，混用了两种方向，讲义没有照抄。

## KV cache 部分的依据与约定

- [Attention Is All You Need](https://arxiv.org/abs/1706.03762)：注意力与多头的计算，缩放的理由（§3.2.1）。
- [Language Models are Few-Shot Learners](https://arxiv.org/abs/2005.14165)：GPT-3 175B 的结构参数。
- [Hugging Face：Caching](https://huggingface.co/docs/transformers/main/cache_explanation)：因果推理中的逐层 KV cache。
- [SGLang: Efficient Execution of Structured Language Model Programs](https://arxiv.org/abs/2312.07104)（Zheng 等，NeurIPS 2024）：RadixAttention 用一棵基数树（前缀树的一种）保存各个 token 序列与它们的 KV cache，按 LRU 先淘汰最久没有使用的叶结点（第 3 节）。
- [vLLM：Automatic Prefix Caching](https://docs.vllm.ai/en/latest/design/prefix_caching.html)：设计文档。KV cache 按块查找，每一块的哈希由这一块的 token 与它之前整个前缀的哈希一起算出，哈希相同就表示从第一个 token 起的前缀相同。
- [Claude Code：Prompt caching](https://code.claude.com/docs/en/prompt-caching)：Claude Code 的每个请求按变化从少到多排列，依次是系统提示词（含工具的定义）、项目的上下文（CLAUDE.md 等）、对话，请求中带有 `cache_control` 标记，使用 Anthropic 接口的前缀缓存。
- [Anthropic：Lessons from building Claude Code: Prompt caching is everything](https://claude.com/blog/lessons-from-building-claude-code-prompt-caching-is-everything)（Thariq Shihipar，2026-04-30，2026-11-19 核对，链接重定向到 claude.dev 上的同一篇）：「新闻：Anthropic 博客」一页的图与引文取自它。图是博客中的「System Prompt Layout」，从上到下是 Base System Instructions、Tools、CLAUDE.md & Memory、Session State、Messages 五层，静态内容在前，动态内容在后。引文是博客的一段英文原文，列出 Claude Code 破坏这个顺序的几种原因，其中之一是在静态的系统提示词中写入详细的时间戳。更新的信息写进下一条用户消息或工具的结果中，放在一个 system-reminder 标签里。图中为每层注明了缓存的范围（比如 globally cached），讲稿没有解释：Claude API 的缓存按组织隔离，不同组织即使提示词相同也不共用缓存（[Claude API：Prompt caching](https://platform.claude.com/docs/en/build-with-claude/prompt-caching)，「Cache storage and sharing」一节）。
- [Claude API：Pricing](https://platform.claude.com/docs/en/about-claude/pricing)（2026-11-19 核对）：「填满上下文的费用」一页的价格取自它。Claude Fable 5.2 每百万 token 的价格为：普通输入 $8，写入缓存 $10（保留 5 分钟）或 $16（保留 1 小时），读取缓存 $0.20（普通输入的 0.025 倍），输出 $40，没有长上下文的加价。这个链接只作为表的脚注，没有加入「参考资料」页，该页的正文已到 y≈690。
- [Thibault Jaigu：The Coding Agent Economy: A 12-Month Observation of Coding Agent Economics](https://www.requesty.ai/coding-agent-economy)（Requesty，2026-05，2026-11-19 按 PDF 的正文核对）：「填满上下文的费用」一页的每个请求新增的 token 数取自它。Requesty 是转发模型 API 请求的网关，报告统计 2025 年 5 月到 2026 年 4 月经过它的 9 个编码 Agent 的生产数据。用到的几句原文："Average input tokens per call rose from approximately 50,000 (May 2025) to 84,000 (April 2026)."，"Claude Code achieves a 92% cache hit rate, meaning only 8% of its input tokens require fresh processing. This is extraordinary given that its average prompt is 84,000 tokens."，"a typical Claude Code session involves 50 to 200 API calls"。缓存命中率按输入 token 加权。网页正文写 Claude Code 的平均输入 "exceeds 200,000 tokens"，与 PDF 的 84,000 不一致，页面用 PDF 的数字。
- [OpenRouter：October 2026 recap](https://openrouter.ai/data/october-2026)：2026 年 10 月 Agent 请求的缓存命中率中位数超过 90%，与 Requesty 的 92% 相符，只在讲稿中提到，页面上没有脚注。

「请求之间的重复计算」一页的图引用 Claude Code 系统提示词的第一句 "You are Claude Code, Anthropic's official CLI for Claude."。这一句出现在发布的 npm 包 [@anthropic-ai/claude-code](https://unpkg.com/@anthropic-ai/claude-code@2.1.87/cli.js)（2.1.87，`cli.js`）中，也逐字出现在 GitHub 仓库 openclaude（Twigpine/openclaude，原名 Gitlawb/openclaude）第一次提交（d2542c9）的 `src/constants/system.ts` 中。openclaude 自己的 LICENSE 写明它未经授权取自 Claude Code 的专有源码，因此页面的脚注引用 npm 包。

「前缀缓存」小节讲稿中的事实于 2026-11-19 核对。「固定前缀」一页的讲稿只写服务商对命中缓存的输入 token 按较低的价格计费，不写各家接口的具体价格、折扣与最短可缓存长度，这些条款变化频繁。「填满上下文的费用」一页只用 Claude Fable 5.2 一种模型的价格，脚注写明查询日期，价格变化时按定价页更新表与讲稿中的数字。

「填满上下文的费用」一页的推导：Claude Code 的请求平均输入 84,000 个 token，其中 8% 未命中缓存，8% × 84,000 = 6,720，即每个请求约新增 6,700 个 token，约 45 个请求填满 300,000 个 token。表按 45 个请求恰好填满计算，每个请求新增 300,000 / 45 个 token。没有 KV cache 时，45 个请求的输入共 (300,000 / 45) × (1 + 2 + … + 45) = 6,900,000 个 token，按普通输入计费，为 $55.20。每次都命中时，读取缓存共 6,600,000 个 token（$1.32），写入缓存共 300,000 个 token（$3.00），合计 $4.32，两者相差约 13 倍。输出在两种情况下相同，表中不计入。6,700 只来自一个来源中的一个 Agent（Claude Code，2026 年 4 月，一个网关的数据），见 `lectures/PENDING.md`。

## 图

图的源文件在 `diagrams/`，每个图（或每组动画）一个脚本，脚本名就是图名，运行 `sh diagrams/render.sh` 生成全部 `assets/*.svg`，生成的 SVG 不手改。公共部件：`svgkit.py`（绘图原语与配色）、`tk.py`（本讲的部件与样式约定：向量、权重矩阵、token、注意力表、概率条、数学文字）、`attn_data.py`（注意力一例的数值）、`math_data.py`（检查用的向量运算）、`kv_grid.py`（第三部分「逐步生成中的重复计算」「因果计算的前缀不变性」「逐层保存的 KV cache」「查询与键值的使用范围」四页共用的网格：token 的列、各层的行、token 标签与图例的位置，翻页时列与行的位置不变）、`prompt_bar.py`（「请求之间的重复计算」「前缀缓存」「固定前缀」三页共用的部件：一段提示词画成一行分段的条，同一种分段在各图中宽度相同，写入提示词的当前时间画成分段中的一个白色小框）。`check_math.py` 重新计算「注意力的计算示例」的数值，并用一个两层的小网络检查使用缓存与整段重算的结果相同，`check_bounds.py` 检查文字与方框是否越出画布。

「请求之间的重复计算」一页的图 `diagrams/request-recompute.py` 从左到右画出三个用户（人形图标与气泡中各自的问题）、三人共用的 Claude Code 客户端（框中的代码写出系统提示词的第一句）、客户端发出的三个请求与 Anthropic 服务器。每个请求是 `prompt_bar.py` 的一行分段的条，系统提示词在三行中等宽，请求 2、3 的系统提示词画斜线，按原文写出的问题是一种单独的分段，宽度随文字。

「固定前缀」一页的图 `diagrams/prompt-order.py` 画出两组连续的两个请求，同一组的两个请求左端对齐，当前时间画成分段中的白色小框，请求 1 为 10:01，请求 2 为 10:02。上方一组把时间写在最后一条消息中：请求 2 的系统提示词与工具说明下方画复用的带，最后一条消息是第一个不同的分段，画红框。下方一组把时间写在系统提示词中约三分之一处：请求 2 中时间之前的部分复用，时间画红框，时间之后的系统提示词与工具说明画斜线，标注「缓存未命中，重新计算」，问题 2 是新的输入，为灰色。两组的分段比画布窄，图用 `save` 的 `left` 参数在画布中居中。

「新闻：Anthropic 博客」一页的图 `assets/news-anthropic-blog.png` 是博客自己的图「System Prompt Layout」，不由 `diagrams/` 生成：从博客的图片地址（https://claude.dev/media/8234c2ef7c2f5624440f380ddbd14ca13d87d3c3827148968c1a83a7d86b73f6.png）下载，裁到内容区域（1752×1074），放在页面右侧，配色与字体是博客的。它是外部图片，版权属于 Anthropic，图的脚注注明博客的出处，没有找到开放的授权，是否可用见 `lectures/PENDING.md`。

「嵌入空间中的方向与点积」一页的图 `diagrams/embedding-direction.py` 由两个已删去的图合并而成：左半部分取自原「现象：性别方向」一页动画的第 1 帧，右半部分取自原「词向量的点积」一页的图的右半部分。

「sidebar：Softmax 的温度」一页的图 `assets/temperature.svg` 由 `diagrams/temperature.py` 用 `lecturekit.bind` 生成，绑定到讲义变量 T（`lecture.py` 中的 `lecture.variable("T", 1, min=0, max=10)`，写法见仓库 `docs/dsl.md` 的「Bound figures」）。查看器中拖动页面上的滑块，图中的概率按当前的 T 重新计算。PDF、PNG 等静态形式显示 T = 1 时的图。

全讲一套配色：数据（嵌入、激活、文本）为灰色，权重矩阵的元素按正负为蓝、红（视频的约定），查询为橙色，键为蓝色，值为紫色，更新量与结果为绿色。整段重算中重复执行的计算用斜线标出，不另用颜色。「前缀缓存」小节的图中斜线保持这个含义，表示前面的请求已经算过、这个请求又计算一次的内容。复用的 K、V 在 `prompt_bar.py` 的图中画成分段下方一条蓝（键）紫（值）两色的带，标注「复用」。红色边框标出与缓存的内容第一个不同的分段或写入的时间，虚线框表示被淘汰的结点。图中的文字为中文，视频例子中的英文词保持原样。

## 构建与检查

在仓库根目录执行：

```bash
sh lectures/6-transformer/diagrams/render.sh
python3 -m lecturekit.cli inspect lectures/6-transformer
python3 -m lecturekit.cli render lectures/6-transformer --to png
python3 -m lecturekit.cli view lectures/6-transformer --watch
python3 -m lecturekit.cli i18n check lectures/6-transformer --lang en
```

## 用语与排版约定

页面用简短的陈述句，一句一个结论，不用比喻。标题是名词短语。页面不提视频本身（「原片」「视频中」只出现在讲稿说明数值来源的地方），也不标注视频的时间段。每页一行引出、一张图、一行结论。动画只用于一个过程的步骤或一个量的变化，并列的例子（模型的各种任务、几组类比、一个词的两种语境）画在一张静态图上。中文 PNG 在 1280×720 下检查正文不越过 y≈700。中文修改后同步英文覆盖层，英文版面不逐页检查。

「嵌入」只指查表得到的向量 E（含位置信息），即第 1 层的输入。经过注意力块或多层感知机更新后的向量写作「向量」，比如「更新后的向量」「最后一个向量」。它的术语名为隐藏状态，记号为 h。ΔE 统一称为「更新量」。「训练」「预训练」「微调」按所指区分。GPT 的全部参数（含 $W_E$）在同一次预训练中学习，GloVe 是单独训练的词向量模型。英文覆盖层对应写作 embedding、vector 或 updated vector、hidden state、update、training、pre-training、fine-tuning。

## 审核状态

2026-11-14 重写前两部分，重画全部图（每个图一个脚本），中文 PNG 逐页看过，正文不越过 y≈700。`render.sh` 的数值检查与越界检查通过，英文覆盖层 0 missing、0 changed、0 orphaned。PPTX 能生成，本机缺少 SVG 栅格化工具（rsvg-convert 或 cairosvg）时图会被跳过，PPTX 没有在 PowerPoint 中打开检查。同日删去全部练习与 sidebar（14 个页面函数，16 张），又删去原第四部分「Transformer 的算子与 GPU」与原第三部分的「计算量与计算特征」小节及「参数量的分布」一页，第三部分改名为「KV cache」，以整段重算的重复计算开头，之后又删去「缓存节省的计算」与「KV cache 的容量」两页。2026-11-16 删去第一部分的「深度学习的计算形式」小节（6 个页面函数，11 张），权重矩阵的着色约定写在「嵌入矩阵」一页的讲稿中。同日删去「嵌入矩阵的参数量」一页（1 个页面函数，1 张）。同日删去「词向量的类比」一页中 cat 近邻的小图，删去「吸收上下文的嵌入」一页（1 个页面函数，3 张），在「上下文长度」之后加入「上下文超出上限的处理方法」一页（1 个页面函数，1 张）。同日删去「每个位置的预测」「解嵌入矩阵的参数量」「logits」三页（3 个页面函数，3 张）：GPT-3 的参数表改在「注意力参数的总量」一页第一次出现，八类矩阵、参数总数与嵌入矩阵、解嵌入矩阵的参数量在这一页的讲稿中介绍。logits 改在「解嵌入矩阵与下一个 token 的预测」一页定义。同日在「查询、键与注意力模式」小节开头加入「注意力块的结构」一页（1 个页面函数，1 张），先给出多个头并行、修改量相加的整体结构，再进入单个头的例子。同日删去「远距离的信息传递」一页（1 个页面函数，1 张）。同日在「每个位置的预测目标」与「因果掩码」之间加入三页（两页标题为「训练中的答案泄露」，一页为「推理中的下文缺失」，3 个页面函数，3 张），按发现问题、分析问题、解决问题引出因果掩码。同日删去「一个注意力头的参数」小节（「查询矩阵与键矩阵的参数量」「值映射的低秩分解」两页）与「多头注意力块的参数量」「输出矩阵」两页（4 个页面函数，9 张，含小节过渡页），值矩阵降到 128 维与输出矩阵改在「注意力参数的总量」一页的讲稿中说明，「值向量」一页的动画改为一张静态图。同日把「值与嵌入的更新」小节并入「查询、键与注意力模式」，「值向量」「值的加权求和」两页移到「注意力模式」与「注意力的矩阵公式」之间，「值的加权求和」的图改为不加掩码的注意力模式。同日「因果掩码」一页的动画改为一张静态图，删去「注意力模式的规模」一页（1 个页面函数，1 张），该页讲的两点（注意力模式的元素个数是上下文长度的平方，长上下文的注意力变体如 Sparse Attention、Longformer）不再出现在讲义中，小节「掩码与注意力模式的规模」改名为「因果掩码」。同日删去「多种上下文关系」「多头注意力」两页（2 个页面函数，4 张），多头注意力的名称改在「注意力块的结构」一页的讲稿中给出，小节「交叉注意力与多头注意力」改名为「交叉注意力」。同日在「交叉注意力」之后加入「交叉注意力与自注意力」一页（1 个页面函数，1 张），说明交叉注意力不需要掩码的原因并对比两种注意力适合的任务。同日删去「多层与规模」小节（「多层中的上下文」「注意力参数的总量」「注意力的可并行性」三页，3 个页面函数，5 张，含小节过渡页），上文记作在「注意力参数的总量」一页介绍的参数表、八类矩阵、参数总数、嵌入与解嵌入矩阵的参数量、值矩阵降维与输出矩阵随之不再出现在讲义中。同日删去第三部分的「逐位置的前馈网络」「完整的解码器层」两页（2 个页面函数，4 张），一层的组成改在「因果计算的前缀不变性」一页的讲稿中说明，「KV cache 部分的依据与约定」中关于前馈网络与 pre-norm 的符号约定随之删去。同日删去「prefill 与 decode」一页（1 个页面函数，2 张），第三部分以「查询与键值的使用范围」一页结束。同日「温度」一页的三帧动画改为滑块 `p.slider('T', marks=[0, 1, 10])` 与一张绑定讲义变量 T 的图（1 张）。同日「续写与对话」一页从「Transformer 的数据流」末尾移到「什么是 GPT」，排在「预测、采样与追加」之后、「模型规模对模型表现的影响」之前。同日讲义目录由 `lectures/7-transformer` 改为 `lectures/6-transformer`。同日删去「模型规模对模型表现的影响」「现象：复数方向」「每个位置的预测目标」三页、标题为「训练中的答案泄露」的两页与「交叉注意力」小节（「交叉注意力」「交叉注意力与自注意力」两页），共 7 个页面函数，9 张（含小节过渡页），第二部分以「因果掩码」小节结束，这一小节以「推理中的下文缺失」一页开头。同日在「注意力的矩阵公式」之后加入「注意力的计算示例」一页（1 个页面函数，4 帧），用两个 token、4 维嵌入的例子把一个注意力头完整算一遍。同日「Softmax 的取值特点」一页的动画由三帧改为两帧，「因果计算的前缀不变性」一页的图改为逐层画出每个位置的 q、k、v，隐藏状态画成向上的箭头。同日统一全讲的术语：「嵌入」只指查表得到的 E，更新后的向量写作「向量」或隐藏状态，ΔE 统一称为「更新量」，「训练」「预训练」「微调」按所指区分，用到 GloVe 的三页（tower 的近邻、性别方向、类比）写明所用的模型，「现象：tower 的近邻」的图加了 GloVe 的出处脚注，英文覆盖层按同一套用语重译。同日第三部分的四张图改用 `diagrams/kv_grid.py` 定义的同一套网格，各页的列与行位置一致，「逐步生成中的重复计算」中重复执行的计算由橙色改为灰色向量上的斜线，橙色此后只表示查询，「温度与模型的表现」的图删去了下方关于取前 20 个候选并取 1/5 次方的说明与两张概率条图。2026-11-19 按导师的意见重组第一部分：投影与相似度的几页过长，解嵌入与温度对本课程不重要，上下文长度与主线无关，与主线关系不强的内容改为标题清楚的 sidebar。第一部分改名为「GPT 的计算流程」，只留「什么是 GPT」「Transformer 的数据流」两个小节，「词嵌入」「解嵌入与 Softmax」两个小节及其过渡页删去（2 张）。「Transformer 的内部结构」一页改为「Transformer 的计算流程」，图改为自下而上的流程，左栏写出每一步的主要计算，输出一端画出 $W_U$、Softmax、概率分布与采样。「在低维子空间里投影高维向量」「现象：性别方向」「词向量的点积」三页合并为「嵌入空间中的方向与点积」一页（新图 `diagrams/embedding-direction.py`），高维空间中的点与点积的逐分量计算改在这一页的讲稿中说明，投影改在「sidebar：词向量的近邻」的讲稿中用一句说明，性别方向中五对家庭关系词的一帧删去。「什么是 token」的第二页（`token-vectors`）删去，它的一句移到「嵌入矩阵」的讲稿中，位置嵌入的说明也由「上下文长度」的讲稿移到「嵌入矩阵」的讲稿中。「Softmax」一页留在主线作为最后一步，解嵌入在它的第一句中说明。「现象：tower 的近邻」「词向量的类比」「上下文长度」「上下文超出上限的处理方法」「解嵌入矩阵与下一个 token 的预测」「Softmax 的取值特点」「温度」「温度与模型的表现」八页改为第一部分末尾的 sidebar（9 张），id 加上前缀 `sidebar-`，标题依次改为「sidebar：词向量的近邻」「sidebar：词向量的类比」「sidebar：上下文长度」「sidebar：上下文超出上限的处理方法」「sidebar：解嵌入矩阵与 logits」「sidebar：Softmax 的取值特点」「sidebar：Softmax 的温度」「sidebar：温度对生成文本的影响」，图的文件名不变，「sidebar：词向量的类比」删去了结尾的提问。共删去 4 个页面函数、加入 1 个（43 个变为 40 个），删去 7 张、加入 1 张（85 张变为 79 张），动画由 14 组变为 13 组。`render.sh` 的数值检查与越界检查通过，英文覆盖层 0 missing、0 changed、0 orphaned，第一部分（第 2–30 页）的中文 PNG 逐页看过，正文不越过 y≈700。同日按导师的意见在第三部分末尾加入「前缀缓存」小节（第 80–87 页）：一页小节过渡页，「请求之间的重复计算」「前缀缓存」（3 帧）「前缀树」（2 帧）「提示词的排列」四页，共 4 个页面函数、8 张（44 个页面函数，87 张），动画由 13 组变为 15 组。新图由 `diagrams/request-recompute.py`、`prefix-cache.py`、`prefix-tree.py`、`prompt-order.py` 生成，公共部件 `diagrams/prompt_bar.py` 加入 `render.sh` 的跳过列表，「前缀缓存」的图沿用 `kv_grid.py` 的列。「参考资料」页加入 SGLang 论文、vLLM 的 Automatic Prefix Caching 设计文档与 OpenAI 的 Prompt caching 指南三项。`render.sh` 的数值检查与越界检查通过，英文覆盖层 0 missing、0 changed、0 orphaned，第三部分与「参考资料」页（第 67–88 页）的中文 PNG 逐页看过，正文不越过 y≈700。同日第一部分末尾的八页 sidebar 改为这一部分的最后一个小节「sidebar」，以一页小节过渡页开头（1 张，全讲由 87 张变为 88 张），页面的 id 与标题不变，原第 21 页起的页码各加 1，过渡页的中文 PNG 看过。同日「请求之间的重复计算」一页改以 Claude Code 为例，图改为三个用户经同一个 Claude Code 客户端向 Anthropic 服务器发出以同一段系统提示词开头的请求，图下加 npm 包的出处脚注，「提示词的排列」讲稿中 OpenAI 的例子改为 Claude Code 的请求顺序，「参考资料」页的 OpenAI Prompt caching 指南换为 Claude Code 的 Prompt caching 文档，`render.sh` 的检查通过，英文覆盖层 0 missing、0 changed、0 orphaned，第 81、87、88 页的中文 PNG 看过。同日按要求删去原「前缀缓存」一页（1 个页面函数，3 张，全讲由 88 张变为 85 张，动画由 15 组变为 14 组）与它的图 `diagrams/prefix-cache.py`，原「前缀树」一页改名为「前缀缓存」，回答第 81 页的问题并在讲稿中给出前缀缓存的定义，「提示词的排列」一页改为「固定前缀」，以 Agent harness 在每次请求的系统提示词中写入当前时间为例说明缓存大量未命中，结论是设计 Agent harness 时应当尽量固定前缀，图 `diagrams/prompt-order.py` 改为时间写在最后一条消息中与写在系统提示词中两组请求的对比，`render.sh` 的检查通过，英文覆盖层 0 missing、0 changed、0 orphaned，第 80–85 页的中文 PNG 看过。同日在「固定前缀」一页末尾加一句：大模型厂商的 Agent harness 往往固定系统提示词，使它持续命中 KV cache，参考资料页加入 Anthropic 的博客。同日删去「固定前缀」末尾加的这一句与讲稿中关于缓存保留时间和 Gemini CLI 的几句，这一页的图恢复为 1040 px 宽，在它之后加入「新闻：Anthropic 博客」一页（1 个页面函数，1 张，全讲由 85 张变为 86 张，44 个页面函数），图由 `diagrams/news-anthropic-blog.py` 生成，`render.sh` 的检查通过，英文覆盖层 0 missing、0 changed、0 orphaned，第 84–86 页的中文 PNG 看过。同日「新闻：Anthropic 博客」一页改为一则趣闻：删去 `diagrams/news-anthropic-blog.py` 与它画的图，改用博客自己的图 `assets/news-anthropic-blog.png`（页面右侧，图下是博客的出处脚注），文字一栏是一行引出与博客的英文原文引文，页面上的中文结论删去，讲稿重写，`prompt_bar.py` 的说明改回三个图共用。在它之后加入「缓存未命中的费用」一页（1 个页面函数，1 张，全讲由 86 张变为 87 张，45 个页面函数，动画仍为 14 组），按 Claude Fable 5.2 的价格（2026-11-19 查询）比较一个请求每次都未命中与每次都命中时输入的费用，「参考资料」移到第 87 页。`render.sh` 的检查通过，英文覆盖层 0 missing、0 changed、0 orphaned，第 85、86 页的中文 PNG 看过。同日「新闻：Anthropic 博客」的讲稿中「左图」改为「右图」，与图在页面右侧一致。同日按要求重写第 86 页，标题改为「填满上下文的费用」：按 OpenRouter 的两项统计（State of AI，2025-12，Cache is Crucial，2026-10-08）估算 Agent 的每个请求约使上下文增加 4,000 个 token，填满 300,000 个 token 的上下文约需 75 个请求，表按 Claude Fable 5.2 的价格比较没有 KV cache（$91.20）与每次都命中（$5.22）时输入的费用，相差约 17 倍，页数与页码不变，英文覆盖层 0 missing、0 changed、0 orphaned。同日按要求再次重写第 86 页（OpenRouter 统计的平均输出约 400 个 token 偏低，改用其他平台更新的报告）：每个请求新增的 token 数改按 Requesty 的报告 The Coding Agent Economy（2026-05）估算，Claude Code 的请求平均输入 84,000 个 token、缓存命中率 92%，每个请求约新增 6,700 个 token，约 45 个请求填满 300,000 个 token，没有 KV cache 时输入 $55.20，每次都命中时 $4.32，相差约 13 倍，两个 OpenRouter 脚注删去，引出句的脚注改为 Requesty 的报告，讲稿用 OpenRouter 2026 年 10 月的统计佐证，英文覆盖层 0 missing、0 changed、0 orphaned。待决定的事项列在 `lectures/PENDING.md`。
