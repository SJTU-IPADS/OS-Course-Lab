"""Transformer：页面正文与讲稿。图的源文件在 diagrams/，每个图一个脚本。"""

# ======================================================================
# 第一部分：GPT 的计算流程（3Blue1Brown 深度学习第 5 章）
#
# 问题：GPT 读入一段文本，怎样一步步算出下一个词？
# 先说明任务（预测下一个词的分布）和由此得到的生成方法与续写、对话。
# 再用一张流程图给出数据流的各步与每一步的主要计算：嵌入是查表，注意力块与
# 多层感知机交替多层，主要是矩阵乘法，其中只有注意力块在位置之间传递信息，
# 最后是解嵌入与 Softmax。之后按数据流的顺序逐步说明：token、嵌入矩阵、
# 嵌入空间中的方向与点积、注意力块、多层感知机、多层的堆叠、Softmax。
# 与主线关系不强的内容放在本部分末尾的 sidebar 小节，顺序与它们展开的主线页相同：
# 词向量的近邻与类比、上下文长度、解嵌入矩阵、Softmax 的取值特点与温度。
# 小节：什么是 GPT · Transformer 的数据流 · sidebar。
# ======================================================================

# 从名称出发：GPT 的三个词分别说明任务、训练方式和网络结构。
# 视频：第 5 章 00:00–00:35。
def gpt(p):
    p.title('生成式预训练 Transformer')
    p.slide('GPT 是 Generative Pretrained Transformer 的缩写。', autobold=False)
    p.notes('Generative 指模型生成新的文本。Pretrained 指模型先在大量文本上训练全部参数，训练目标是预测下一个词。之后还可以用较少的数据继续训练，针对具体任务调整参数，这一步称为微调。Transformer 是一种神经网络，是当前这一轮人工智能发展的核心技术。本讲沿着数据在 Transformer 中的流动，逐步说明每一步的计算。本讲的例子与数字多取自 GPT-3，它是 OpenAI 在 2020 年发布的 GPT 模型。')
    p.image('assets/gpt.svg', width_px=1040)


# 名称中的 Transformer 是一种结构，可以组成多种模型，由此限定本讲研究的模型。
# 视频：第 5 章 00:35–01:45。
def tasks(p):
    p.title('Transformer 的任务')
    p.slide('用 Transformer 可以构造多种模型。', autobold=False)
    p.notes('有的模型读入语音、输出文字，有的反过来由文字合成语音。由文字描述生成图像的工具也使用 Transformer。2017 年 Google 提出的原始 Transformer 用于机器翻译。ChatGPT 所用的模型读入一段文本，可能还附带图像或声音，预测接下来的内容。预测的形式是对许多可能的后续文本片段给出概率分布。')
    p.image('assets/tasks.svg', width_px=1040)
    p.slide('本讲研究的对象为纯文本模型。', autobold=False)


# 模型只预测下一个词，反复预测并采样，就得到长文本。
# 视频：第 5 章 01:45–02:10。
def generation(p):
    p.title('预测、采样与追加')
    p.slide('把预测下一个词的模型反复使用，就能生成长文本。', autobold=False)
    p.notes('给模型一段起始文本，模型预测下一个词的概率分布。从分布中随机采样一个词，预测的概率越高，越容易被选中。图中的起始文本是 Behold, a wild pi creature, foraging in its native，模型给 land 22% 的概率，land 最容易被选中。采样是随机的，这次得到的是只有 1% 的 woodland，低概率的词也可能被选中。采样得到的词追加到文本末尾，再以全部文本为输入做下一次预测。图中的百分数取整数部分。')
    p.frames('assets/generation-1.svg', 'assets/generation-2.svg', 'assets/generation-3.svg', 'assets/generation-4.svg', 'assets/generation-5.svg', width_px=1040)
    p.slide('预测、采样、追加，再用加长后的文本重新预测。', autobold=False)


# 上一页的生成循环就是续写，对话也是同一个循环：系统提示词与用户的问题放在开头，模型续写回答。
# 视频：第 5 章 05:38–06:35。
def chat(p):
    p.title('续写与对话')
    p.slide('Transformer 能够做到的事情是：给定起始文本，反复预测、采样、追加，就得到续写。', autobold=False)
    p.notes('在 ChatGPT 出现之前，GPT-3 的早期演示就是根据开头续写故事和文章。要得到聊天机器人，最简单的做法是先写一段文字，设定用户与一个乐于助人的 AI 助手对话的场景，这段文字称为系统提示词。用户的问题作为对话的第一句，模型接着预测 AI 助手会说什么。要让它表现良好，还需要在预训练之后继续训练模型，这一步称为微调，比如 ChatGPT 由 GPT-3.5 系列的一个模型微调得到。微调使用人工编写的对话，以及人工对模型回答的排序。')
    p.image('assets/chat.svg', width_px=1040)
    p.slide('Agent 的对话模式本质上也是一种续写：输入的文本里，系统提示词和用户的问题放在开头，由模型续写 AI 助手的回答。', autobold=False)


# 小节的开头：用一张流程图给出数据流的各步与每一步的主要计算，之后各页按这个顺序逐步说明。
# 视频：无对应片段。
def dataflow_overview(p):
    p.title('Transformer 的计算流程')
    p.slide('输入的 token 依次经过嵌入、多层的注意力块与多层感知机、解嵌入与 Softmax。', autobold=False)
    p.notes('输入的文本先被切成 token，图中只画出最后四个 token，其中 aging 是 foraging 切开后的后一段。嵌入为每个 token 取出嵌入矩阵中对应的一列，得到一个向量。向量序列进入注意力块，各位置的向量经矩阵乘法、点积与 Softmax 互相传递信息。之后进入多层感知机，每个向量各自经过同一组矩阵乘法。一个注意力块和一个多层感知机组成一层，这样的层重复多次，前一层的输出是后一层的输入。最后一层输出的最后一个向量乘以解嵌入矩阵，为词表中每个 token 得到一个分数，Softmax 把分数变为概率分布。从分布中采样得到 woodland，追加到文本末尾，再开始下一次预测。')
    p.image('assets/dataflow-overview.svg', width_px=1040)
    p.slide('嵌入之后的各步主要是矩阵乘法。只有注意力块在位置之间传递信息。', autobold=False)


# 流程图之后按数据流的顺序逐步看，第一步：输入先被切成 token。
# 视频：第 5 章 03:03–03:37。
def tokens(p):
    p.title('什么是 token')
    p.slide('Transformer 先把输入切成小段，每一段称为一个 token。', autobold=False)
    p.notes('图中的句子按 GPT-3 的分词结果切分：cleverest 被切成 cle、ve、rest 三个 token，逗号单独是一个 token。为了便于举例，后面常把一个词当作一个 token。')
    p.image('assets/tokens.svg', width_px=1040)
    p.slide('token 可以是词、词的一部分或标点，也可以是图像的小块、声音的短片段。', autobold=False)


# token 本身不能参与计算，嵌入矩阵把每个 token 换成一个向量。
# 视频：第 5 章 03:37–03:55 与 12:27–13:27。
def embedding_matrix(p):
    p.title('嵌入矩阵')
    p.slide(r'模型有一个预先确定的词表。嵌入矩阵 $W_E$ 为词表中的每个 token 保存一列。', autobold=False)
    p.notes('每个 token 对应一个向量，即一列数，用来编码这个 token 的含义。为了举例方便，这里把 token 当作完整的词。GPT-3 的词表有 50,257 个 token。W_E 是模型参数的一部分，它的值从随机数开始，在预训练中与网络的其他参数一起根据数据学习。图中矩阵的元素按正负着色，蓝色为正，红色为负。输入第 1 层的嵌入还要加上位置嵌入矩阵中这个位置的一列，使它同时编码 token 和它在文本中的位置。位置嵌入矩阵也在预训练中学习，比如 GPT-2 为 1,024 个位置各保存一列。把词变成向量在 Transformer 之前已是机器学习的常用做法，比如 word2vec 与 GloVe 专门训练一张词向量表，再交给其他模型使用。')
    p.image('assets/embedding-matrix.svg', width_px=1040)
    p.slide(r'输入的每个 token 取出 $W_E$ 中对应的一列，作为它的向量。', autobold=False)


# 嵌入矩阵给出的向量是高维空间中的点：空间中的方向可以带有语义，两个向量的方向是否一致用点积衡量。
# 注意力按语义调整向量、用点积比较向量，都要用到这两点。
# 视频：第 5 章 14:58–15:50 的性别方向与 16:34–17:06 的点积。
def embedding_direction(p):
    p.title('嵌入空间中的方向与点积')
    p.slide('嵌入空间中的方向可以带有语义，比如 woman 与 man 之差接近 queen 与 king 之差。', autobold=False)
    p.notes('嵌入是高维空间中的点，比如 GPT-3 的嵌入是 12,288 维空间中的点，含义相近的词彼此靠近。图中点的位置是示意。左边的例子用 GloVe 词向量模型计算：king 加上 woman 减 man，离结果最近的词是 queen，但 queen 的实际位置与这个点有一段距离。点积把两个向量的对应分量相乘再相加，比如 (2, 1, −1, 3) 与 (1, 3, 0, 2) 的点积为 2 + 3 + 0 + 6 = 11。woman − man 与 queen − king 方向接近，它们的点积为正。')
    p.image('assets/embedding-direction.svg', width_px=1040)
    p.slide('两个向量的点积衡量它们的方向是否一致：方向相近时为正，垂直时为零，相反时为负。', autobold=False)


# 嵌入只编码单个 token，要利用上下文，向量之间需要传递信息，这是注意力块的作用。
# 视频：第 5 章 03:55–04:29。
def attention_preview(p):
    p.title('注意力块：向量之间的信息传递')
    p.slide('向量序列经过注意力块，各向量之间传递信息，使每个向量包含其他 token 的一部分信息。', autobold=False)
    p.notes('machine learning model 中的 model 指机器学习模型，fashion model 中的 model 指时装模特。两处 model 开始时是同一个向量，注意力根据上下文把它们更新成不同的向量。这里所说的含义，全部编码在向量的分量中。')
    p.image('assets/attention-preview.svg', width_px=1040)


# 注意力之后的第二种块：各向量各自变换，加入预训练中学到的知识。
# 视频：第 5 章 04:29–05:13，Michael Jordan 一例取自第 7 章。
def mlp_preview(p):
    p.title('多层感知机：逐个向量的变换')
    p.slide('注意力块的输出向量接下来经过多层感知机（也称前馈层），它对每个向量单独变换，加入模型在预训练中学到的知识。', autobold=False)
    p.notes('多层感知机对每个向量单独计算：先检验向量编码了哪些特征（图中写成问题），再根据结果给出一个更新量，加到这个向量上，比如一个向量经过注意力块后编码了 Michael Jordan 这个名字，上下文中没有出现篮球，多层感知机可以在这个向量上加入与篮球相关的方向。这条知识存在多层感知机的参数中，由预训练得到。GPT-3 约三分之二的参数在多层感知机中。注意力块和多层感知机的运算都主要是线性运算，即矩阵乘法，理解这些矩阵是本讲的主要任务。两种块之间还有归一化步骤，这里略去。')
    p.image('assets/mlp-preview.svg', width_px=960)
    p.slide('注意力块从上下文中获取信息，多层感知机从模型参数中获取信息。', autobold=False)


# 两种块组成一层，层重复多次：说明为什么需要多层。
# 视频：第 5 章 05:13–05:38。
def layer_stack(p):
    p.title('多层 Transformer')
    p.slide('注意力块与多层感知机往往交替重复多次。', autobold=False)
    p.notes('一个注意力块和一个多层感知机组成一层。在一层中，每个向量从其他向量获得一次信息，再各自经过一次变换。其他向量在这一层获得的信息，要到下一层才能传给它。层数越多，每个向量能结合的上下文越多，编码的含义也越具体。最后一层输出的最后一个向量用来预测下一个 token。')
    p.image('assets/layer-stack.svg', width_px=580)
    p.slide('重复多层，向量才能多次利用在前面各层已经汇集的上下文，逐步编码更复杂的含义。', autobold=False)


# 数据流的最后一步：最后一个向量经解嵌入矩阵得到词表中每个 token 的分数，Softmax 把分数变为概率分布。
# 视频：第 5 章 20:23–21:15 的解嵌入（只取一句）与 22:22–23:30 的 Softmax。
def softmax(p):
    p.title('Softmax')
    p.slide(r'最后一个向量乘以解嵌入矩阵 $W_U$，得到词表中各 token 的分数。分数还不是概率分布。', autobold=False)
    p.notes('概率分布要求每个值在 0 与 1 之间，且总和为 1。矩阵乘法得到的分数常常是负数或远大于 1，总和也几乎不会是 1。取指数后所有值都为正，再除以总和就归一化为和为 1 的一列数。最大的输入对应最接近 1 的输出，较小的输入对应接近 0 的输出。图中是一段提到 Harry Potter、以 least favourite teacher, Professor 结尾的文本之后，几个候选 token 的分数。')
    p.image('assets/softmax.svg', width_px=1040)
    p.slide(r'Softmax 先对每个数取 $e$ 的幂，再除以它们的总和：$p_i = e^{x_i} / \sum_j e^{x_j}$。', autobold=False)


# sidebar：展开「嵌入空间中的方向与点积」一页讲稿中「含义相近的词彼此靠近」一句，
# 看一个真实词向量模型中的近邻。
# 视频：第 5 章 14:21–14:58，13:27–14:21 的投影只在讲稿中用一句说明。
def sidebar_tower_neighbors(p):
    p.title('sidebar：词向量的近邻')
    p.slide('训练得到的嵌入中，含义相近的词彼此靠近，比如 GloVe 词向量模型中 tower 的近邻。', autobold=False)
    p.notes('GloVe 是 2014 年提出的词向量模型，它只学习一张词向量表，训练数据是语料中词与词共同出现的统计。图中是 50 维的 GloVe，用 Wikipedia 与 Gigaword 新闻语料训练。GPT 的嵌入矩阵在预训练中与网络的其他参数一起学习，训练目标是预测下一个 token。虚线框内是与 tower 距离最近的十个词，距离按 50 维向量之间的欧氏距离计算。图中点的位置是示意，把高维向量画在三维空间中，其余方向上的分量全部丢失。')
    p.image('assets/tower-neighbors.svg', width_px=640).footnote('[GloVe（glove-wiki-gigaword-50）](https://nlp.stanford.edu/projects/glove/)')
    p.slide('与 tower 的嵌入距离最近的词，都与塔和建筑相关。', autobold=False)


# sidebar：性别之外，嵌入空间中别的方向也带有语义，展开「嵌入空间中的方向与点积」一页的第一句。
# 视频：第 5 章 15:50–16:34。
def sidebar_analogies(p):
    p.title('sidebar：词向量的类比')
    p.slide('嵌入空间中还有许多方向带有语义，向量的加减能揭示训练语料中词与词的关联。', autobold=False)
    p.notes('图中的例子用 GloVe 计算，点的位置是示意。Italy 减 Germany 再加 Hitler，结果接近 Mussolini，好像 GloVe 学到了一个表示意大利的方向和一个表示二战轴心国领导人的方向。Germany 减 Japan 加到 sushi 上，结果接近 bratwurst（德国香肠）。')
    p.image('assets/analogies.svg', width_px=726)


# sidebar：网络一次能处理的向量个数有上限，称为上下文长度。
# 视频：第 5 章 19:24–20:23。
def sidebar_context_size(p):
    p.title('sidebar：上下文长度')
    p.slide('网络一次只能处理固定数量的向量，称为上下文长度，比如 GPT-3 的上下文长度为 2,048。', autobold=False)
    p.notes('每个 token 对应一个向量，上下文长度限制了一次预测能用到的 token 数。早期版本的 ChatGPT 在对话很长之后会显得忘记前面的内容，原因就在这个长度限制。')
    p.image('assets/context-size.svg', width_px=1040)
    p.highlight('如何处理上下文超出上限的情况？')


# sidebar：上一页以问题结束：输入超过上下文长度时怎么办。这一页给出三种常见的处理方法，
# 依次是丢掉窗口外的部分、拒绝整个请求、把窗口外的部分压缩成摘要。
# 视频：无对应片段。
def sidebar_context_overflow(p):
    p.title('sidebar：上下文超出上限的处理方法')
    p.slide('输入的 token 数超过上下文长度时，常见的处理方法有三种。', autobold=False)
    p.notes('第一种方法只保留最近的 token，窗口之外的早期内容不再参与预测。第二种方法拒绝整个请求并返回错误，由调用方自行缩短输入，模型的云服务接口默认采用这种方式。第三种方法在上下文接近上限时，让模型把此前的会话写成一段摘要，用摘要替换原来的会话再继续，Agent 多采用这种方式。摘要保留此前会话的要点，细节可能丢失。')
    p.image('assets/context-overflow.svg', width_px=1040)
    p.slide('舍弃与报错实现简单，总结能保留早先内容的要点，但生成摘要需要模型额外推理一次。', autobold=False)


# sidebar：展开「Softmax」一页第一句中的解嵌入矩阵：它的形状、输出的名称 logits 与 Snape 一例。
# 视频：第 5 章 20:23–21:15，logits 的名称取自 25:33–26:03。
def sidebar_unembedding(p):
    p.title('sidebar：解嵌入矩阵与 logits')
    p.slide('解嵌入矩阵的输入是最后一层注意力块和多层感知机的输出，输出称为 logits，反映各 token 概率的相对高低，经 Softmax 归一化后是下一个 token 的概率分布。', autobold=False)
    p.notes('文本中出现了 Harry Potter，紧挨着最后一个词的是 least favourite teacher，最后一个词是 Professor。训练良好、熟悉哈利·波特的网络会给 Snape 很高的概率。计算分两步。先用解嵌入矩阵把最后一个向量映射为一列数，每个 token 一个值，这列数就是 logits。再用 Softmax 把 logits 归一化为概率分布。W_U 的元素与嵌入矩阵一样从随机数开始，在预训练中学习。W_U 也可以直接取 W_E 的转置，两者共用同一组参数，比如 GPT-2。')
    p.image('assets/unembedding.svg', width_px=880)
    p.slide(r'比如 GPT-3 的解嵌入矩阵 $W_U$ 把最后一个向量映射为 50,257 个值，与词表中 token 的总数相同，Softmax 再把它们归一化为概率。', autobold=False)


# sidebar：Softmax 的名称来自它与“取最大值”的区别。
# 视频：第 5 章 23:30–23:55。
def sidebar_softmax_behavior(p):
    p.title('sidebar：Softmax 的取值特点')
    p.slide('Softmax 是一种“柔和”的取最大值。一个输入明显最大时，它几乎占据全部概率。几个输入同样大时，它们共同分得概率。', autobold=False)
    p.notes('若从这个分布采样，一个明显最大的输入几乎总会被选中。与直接取最大值不同，其他值同样大时，它们也分得可观的概率。')
    p.frames('assets/softmax-behavior-1.svg', 'assets/softmax-behavior-2.svg', width_px=1040)


# sidebar：上一页的集中程度在推理时可以调节，调节它的常数称为温度。
# 视频：第 5 章 23:55–24:33。
# 滑动条改变 T，图中的概率随之更新，刻度 0、1、10 标出三种典型情形。
def sidebar_temperature(p):
    p.title('sidebar：Softmax 的温度')
    p.slide(r'Softmax 的指数中可以除以一个常数 $T$，称为温度。', autobold=False)
    p.notes('这个名称来自它与某些热力学公式中温度的相似。ChatGPT 用这个分布生成下一个词时，可以用温度调节随机性。T = 0 按极限理解，即总是选择最大的值。滑动条下的三个刻度是三种典型情形：T = 10 时各个概率相近，T = 1 时结果与不带温度的 Softmax 相同，T = 0 时全部概率落在 Snape 上。')
    p.slider('T', marks=[0, 1, 10])
    p.image('assets/temperature.svg', width_px=1040)
    p.slide(r'$T$ 越大，分布越均匀。$T$ 越小，最大值越占优势。$T = 0$ 时全部概率落在最大值上。', autobold=False)


# sidebar：温度对生成结果的影响。
# 视频：第 5 章 24:33–25:33。
def sidebar_temperature_stories(p):
    p.title('sidebar：温度对生成文本的影响')
    p.slide('让 GPT-3 从 Once upon a time, there was a 开始写故事，取不同的温度。', autobold=False)
    p.notes('温度为 0 时，模型总是选择最可能的词，故事是 Goldilocks 的老套翻版。较高的温度让模型有机会选择不太可能的词，故事从一位韩国的年轻网络艺术家开始，但很快变得没有意义。当时的 API 不允许温度超过 2，这不是数学上的限制，只是为了避免工具生成过于荒谬的内容。视频中的高温演示取 API 返回的前 20 个候选，把概率取 1/5 次方后重新归一化，相当于在这 20 个候选上取 T = 5。')
    p.image('assets/temperature-stories.svg', width_px=1040)
    p.slide('温度为 0 时故事老套。温度较高时开头更新颖，但有失去连贯的风险。', autobold=False)


# ======================================================================
# 第二部分：注意力机制（3Blue1Brown 深度学习第 6 章）
#
# 问题：查表得到的嵌入与上下文无关，怎样让每个位置的向量吸收上下文？
# 先用例子说明需要的行为（mole、tower），再给出注意力块由许多头
# 并行组成的整体结构，然后用一个注意力头构造
# 计算：查询与键给出相关程度，Softmax 把它变成权重，值向量决定加什么，
# 加权求和与残差完成更新，论文把这些计算写成一个式子，再用一个小例子按式子算一遍。
# 训练时每个位置预测它的下一个 token，注意力却能读到这个 token，推理时它又不存在，
# 由此引出因果掩码。
# 小节：注意力要解决的问题 · 查询、键与注意力模式 · 因果掩码。
# ======================================================================

# 本部分的出发点：每个位置的向量要从词义变为上下文中的含义。
# 视频：第 6 章 00:00–01:46。
def attention_goal(p):
    p.title('注意力机制的目标')
    p.slide('注意力机制由 2017 年的论文 *Attention Is All You Need* 提出。', autobold=False)
    p.notes('嵌入空间中的方向可以对应语义，比如有一个方向对应性别。可以设想还有许多方向，分别对应词义的其他方面。初始嵌入只编码单个词，Transformer 的目标是逐层调整每个位置的向量，使它带有丰富得多的上下文含义。嵌入经过注意力块或多层感知机更新后得到的向量称为隐藏状态（hidden state）。注意力机制常让人困惑，下面先看几个希望注意力实现的行为，再看计算。')
    p.image('assets/attention-goal.svg', width_px=1040)
    p.slide('注意力从嵌入出发逐步调整每个位置的向量，使它从表示单个词变为表示带有上下文的含义。', autobold=False)


# 第一个例子：同一个词，初始嵌入相同，含义却不同。
# 视频：第 6 章 01:46–02:57。
def mole(p):
    p.title('同一个词的不同含义')
    p.slide('mole 在下面三个短语中含义各不相同。', autobold=False)
    p.notes('American shrew mole 中是鼹鼠，One mole of carbon dioxide 中是摩尔，Take a biopsy of the mole 中是痣。分词并查表之后，三处 mole 的向量完全相同，因为初始嵌入是一张查表，不涉及上下文。到下一步，周围的嵌入才有机会把信息传给它。可以设想嵌入空间中有几个不同的方向，分别编码 mole 的不同含义。')
    p.image('assets/mole.svg', width_px=1040)
    p.slide('注意力根据上下文为每个词算出一个更新量，叠加到这个词的向量上。', autobold=False)


# 第二个例子：上下文可以一步步把含义限定得更具体。
# 视频：第 6 章 02:57–03:29。
def tower_refine(p):
    p.title('示例：逐步具体化 tower')
    p.slide('tower 的嵌入是一个笼统的方向，与许多高大的名词相近。', autobold=False)
    p.notes('前面出现 Eiffel 时，希望注意力把 tower 的向量更新到更具体地表示埃菲尔铁塔的方向，与巴黎、法国和钢铁制品相关。再出现 miniature 时，向量应进一步更新，不再与高大的事物相关。')
    p.frames('assets/tower-refine-1.svg', 'assets/tower-refine-2.svg', 'assets/tower-refine-3.svg', width_px=1040)
    p.slide('上下文中的每个词都可能进一步限定一个词的含义。', autobold=False)


# 小节的开头：先给出注意力块的整体结构，用一句话上的几个设想的头作宏观示例，
# 说明各个头的更新量怎样合成，再放大其中一个头讲解它的计算。
# 视频：第 6 章 20:31–22:03 的多头结构，提前到这里作整体说明。
def block_structure(p):
    p.title('注意力块的结构')
    p.slide('注意力块由许多注意力头并行组成，比如 GPT-3 的每个注意力块有 96 个注意力头。', autobold=False)
    p.notes('图中是三个设想的头在同一句话上的作用。图中的注意力块位于第 1 层，输入是嵌入 E。第一个头让形容词调整其后名词的含义，第二个头让限定词 a、the 调整其后名词的含义，第三个头把主语 creature 的信息传给动词 roamed。每个头用自己的一组可学习的矩阵，根据上下文为每个位置的嵌入算出一个更新量。各个头的更新量相加，再叠加到原嵌入 E 上，得到更新后的向量 E′。这种结构称为多头注意力。真实的头学到的关系由训练决定，往往难以解释。')
    p.image('assets/block-structure.svg', width_px=1040)
    p.slide('每个头根据上下文为每个位置算出一个更新量，所有头的更新量相加后叠加到原向量上。', autobold=False)


# 上一页给出注意力块由许多头组成，这一页放大其中一个头：用一个简单的设想行为构造它。
# 视频：第 6 章 04:29–06:14。
def fluffy_example(p):
    p.title('一个注意力头的例子')
    p.slide('设想一个注意力头只做一种更新：形容词调整其后名词的含义。', autobold=False)
    p.notes('例句是 a fluffy blue creature roamed the verdant forest。初始嵌入记为 E，它编码词本身和词在上下文中的位置。计算仍主要是矩阵乘向量，矩阵中是可学习的参数，也称权重。“形容词更新名词”是为说明而设想的行为。真实的头的行为由训练中大量参数的调整决定，难以解释。')
    p.frames('assets/fluffy-example-1.svg', 'assets/fluffy-example-2.svg', width_px=1040)
    p.slide(r"目标是由嵌入 $E$ 算出一组更新后的向量 $E'$，使名词的向量吸收修饰它的形容词的含义。", autobold=False)


# 计算的第一步：名词提出“要找什么”。
# 视频：第 6 章 06:14–07:41。
def query(p):
    p.title('查询向量')
    p.slide('设想 creature 在问：我前面有形容词吗？这个问题编码为一个向量，称为查询。', autobold=False)
    p.notes('查询的维数远小于嵌入的维数，GPT-3 中为 128。W_Q 的元素是模型参数，它在一个头中实际做什么很难解释。这里设想它把名词的嵌入映射到查询空间中一个表示“寻找前面的形容词”的方向，它对其他嵌入的作用暂不考虑。')
    p.frames('assets/query-1.svg', 'assets/query-2.svg', 'assets/query-3.svg', width_px=1040)
    p.slide(r'查询 $q_i = W_Q E_i$。$W_Q$ 乘以上下文中的每个嵌入，每个 token 得到一个 128 维的查询。', autobold=False)


# 第二步：与查询页同样的形式，先回到问答的语境，说明形容词的回答也编码为一个向量（键），
# 再给出一个键的计算，然后是所有位置的查询与键，最后在查询/键空间中比较。
# 视频：第 6 章 07:41–08:17。
def key(p):
    p.title('键向量')
    p.slide('设想 fluffy 和 blue 回答：我是形容词，在这个位置。这个回答也编码为一个向量，称为键。', autobold=False)
    p.notes('W_K 同样由可学习的参数组成，把嵌入映射到与查询相同的 128 维空间。在这个例子中，W_K 把 fluffy、blue 等形容词映射为与 creature 的查询方向接近的键。')
    p.frames('assets/key-1.svg', 'assets/key-2.svg', 'assets/key-3.svg', 'assets/key-4.svg', width_px=1040)
    p.slide(r'键 $k_i = W_K E_i$，与查询位于同一个 128 维空间。键与查询方向接近时，认为二者匹配。', autobold=False)


# 第三步：用点积同时比较每一对键与查询。
# 视频：第 6 章 08:17–09:29。
def attention_scores(p):
    p.title('键与查询的点积表')
    p.slide('每一对键和查询做点积，衡量它们的匹配程度。', autobold=False)
    p.notes('圆点越大，点积越大。fluffy、blue 的键与 creature 的查询点积为较大的正数。the 的键与 creature 的查询点积较小或为负，说明两者无关。这张表中的值可以是任意实数，给出每个词与更新其他每个词的含义的相关程度。')
    p.frames('assets/attention-scores-1.svg', 'assets/attention-scores-2.svg', width_px=1040)
    p.slide('点积大时，称 creature 关注（attend to）fluffy 和 blue。', autobold=False)


# 分数要作为加权求和的权重，需要归一化。
# 视频：第 6 章 09:29–10:18。
def attention_pattern(p):
    p.title('注意力模式')
    p.slide('这些分数将用作每一列加权求和的权重，因此每个值应在 0 与 1 之间，每列之和为 1。', autobold=False)
    p.notes('对每一列做 Softmax 即可。归一化之后，每一列给出左侧各词与顶部这个词的相关程度。')
    p.frames('assets/attention-pattern-1.svg', 'assets/attention-pattern-2.svg', 'assets/attention-pattern-3.svg', width_px=1040)
    p.slide('对每一列做 Softmax，得到的表称为注意力模式。', autobold=False)


# 注意力模式只说明谁与谁相关，下一步决定传递什么。
# 视频：第 6 章 13:10–14:39。
def value(p):
    p.title('值向量')
    p.slide('有了注意力模式，下一步是更新嵌入，比如让 fluffy 改变 creature 的嵌入。', autobold=False)
    p.notes('值矩阵 W_V 乘以 fluffy 的嵌入 E_2，得到值向量 v_2。W_V 乘以一个词的嵌入，可以理解为：如果这个词与调整另一个词的含义相关，应该往那个词的嵌入上加什么。')
    p.image('assets/value.svg', width_px=1040)
    p.slide(r'值向量 $v_2 = W_V E_2$ 加到 creature 的嵌入 $E_4$ 上，它与嵌入位于同一个空间。', autobold=False)


# 每个位置可能有多个相关的词，按注意力权重汇总它们的值向量。
# 视频：第 6 章 14:39–15:49。
def weighted_values(p):
    p.title('值的加权求和')
    p.slide(r'$W_V$ 乘以每个嵌入，得到一列值向量，每个值向量与同一行的键对应。', autobold=False)
    p.notes('得到注意力模式后，查询和键就不再需要。以 creature 这一列为例，fluffy 和 blue 的值向量以较大比例加入，其余值向量几乎为零。相加得到的更新量 ΔE 加到 creature 的原嵌入上，结果有望编码“毛茸茸的蓝色生物”这样更丰富的含义。对所有列做同样的加权求和，就得到全部位置更新后的向量 E′。以上全部过程称为一个注意力头。')
    p.frames('assets/weighted-values-1.svg', 'assets/weighted-values-2.svg', 'assets/weighted-values-3.svg', 'assets/weighted-values-4.svg', width_px=1040)
    p.slide(r"每列按权重把值向量相加得到 $\Delta E$，加到原嵌入上即为更新后的向量：$E' = E + \Delta E$。", autobold=False)


# 以上步骤合起来，就是论文中的注意力公式。
# 视频：第 6 章 10:18–11:05。
def attention_formula(p):
    p.title('注意力的矩阵公式')
    p.slide('Attention Is All You Need 论文把这些计算写成一个紧凑的式子。', autobold=False)
    p.notes('Q、K 分别是全部查询向量与全部键向量组成的数组。分子紧凑地表示所有键与查询的点积。除以 √d_k 是为了数值稳定。V 是值向量组成的数组，用于更新每个位置的向量。第 1 层之后，各层的 Q、K、V 由上一层输出的隐藏状态算出，计算方法相同。论文中每个 token 占一行，QKᵀ 的每一行对应一个查询，Softmax 沿行进行。前面的表以查询为列，是它的转置，对应的写法是 V·softmax(KᵀQ/√d_k)，V 的每一列是一个值向量。')
    p.image('assets/attention-formula.svg', width_px=1040)
    p.slide(r'点积除以键查询空间维数的平方根 $\sqrt{d_k}$，再对每个查询做 Softmax。', autobold=False)


# 按上一页的式子，用两个 token、4 维嵌入的小例子把一个注意力头完整算一遍。
# 布局与前面的表相同，每个 token 占一列。视频之外的补充。
def attention_example(p):
    p.title('注意力的计算示例')
    p.slide(r'两个 token 的嵌入维数为 4，给定 $W_Q$、$W_K$、$W_V$，按注意力公式计算一个注意力头的输出。', autobold=False)
    p.notes('这里查询与键的维数 d_k = 4，√d_k = 2。各矩阵的每一列对应一个 token：Q = W_Q E，K = W_K E，V = W_V E。KᵀQ 第 i 行第 j 列是第 i 个键与第 j 个查询的点积，除以 2 后对每一列做 Softmax，得到注意力模式。creature 一列的分数为 3 和 1，Softmax 后为 0.88 和 0.12。输出的每一列按这一列的权重把两个值向量相加。')
    p.frames('assets/attention-example-1.svg', 'assets/attention-example-2.svg', 'assets/attention-example-3.svg', 'assets/attention-example-4.svg', width_px=1040)
    p.slide('creature 一列的权重集中在 fluffy 上，它的输出主要来自 fluffy 的值向量。', autobold=False)


# 因果掩码的出发点：训练时整段文本一次输入，每个位置预测它的下一个 token，
# 而目前的注意力能读到这个 token。推理时这个 token 还不存在，训练学到的做法无从使用。
def generation_gap(p):
    p.title('推理中的下文缺失')
    p.slide('训练时，每个位置预测它的下一个 token，比如 creature 一列预测 roamed，而注意力能读到 roamed。推理时，要预测的 token 还不存在。', autobold=False)
    p.notes('训练时整段文本一次输入，最后一层的每个向量都用于预测紧跟在它后面的 token。没有限制时，每一列的查询对全部键做 Softmax，creature 一列能经注意力读到它要预测的 roamed。训练按模型给真实的下一个 token 的概率高低调整参数，模型只需读出这个 token，就能给它接近 1 的概率。推理时，最后一个位置之后没有 token，这种做法无从使用，模型给出的预测没有意义。要让训练学到的做法在推理时同样可用，训练时每个位置看到的信息就必须与推理时相同，也就是只看它之前的 token。')
    p.image('assets/generation-gap.svg', width_px=1040)
    p.highlight('如何让每个位置只受它之前的 token 影响？')


# 回答上一页的问题：后面的词不能影响前面的词，在 Softmax 之前把键在查询之后的格子屏蔽。
# 视频：第 6 章 11:49–12:42。
def causal_mask(p):
    p.title('因果掩码')
    p.notes('表中后面的 token 影响前面 token 的位置都必须为 0。最简单的想法是直接把它们设为 0，但这样各列之和不再为 1。GPT 总是使用掩码。掩码在训练中最关键，模型作为聊天机器人运行时也同样使用。')
    p.image('assets/causal-mask.svg', height_px=400)
    p.slide(r'在做 Softmax 之前先把红色格子的分数设定为 $-\infty$，运算后它们的权重变为 0，此时各列概率之和仍为 1。这称为因果掩码。', autobold=False)


# ======================================================================
# 第三部分：KV cache
#
# 问题：推理时每采样一个 token，就把它追加到输入并再次运行模型。把整段
# 重新运行时，旧位置的计算是否必须重做？
# 先看整段重算重复了哪些计算。为回答旧位置的结果是否改变，逐个看一层中
# 的运算读取哪些位置：多层感知机逐位置进行，只有注意力读取其他
# 位置，而它受因果掩码限制。由此证明追加新 token 不改变旧位置的任何
# 一层，旧位置各层的 K、V 可以保存复用，得到逐层的 KV cache。之后说明
# 只缓存 K、V 的原因。
# 最后把复用推广到请求之间：多轮对话的每一轮与使用同一系统提示词的
# 不同请求，输入以相同的 token 开头，请求结束后释放的 KV cache 被重新计算。
# 由前缀不变性，两个输入从第一个 token 起相同的部分 K、V 也相同，保留
# 已结束请求的 KV cache 供后来的请求复用，即前缀缓存。缓存的各个序列组成
# 一棵前缀树，按 LRU 淘汰叶结点。复用只到第一个不同的 token 为止，所以
# Agent harness 在系统提示词中写入当前时间会使缓存大量未命中，设计时应当尽量固定前缀。
# 小节以 Anthropic 的博客结束：Claude Code 曾经在静态的系统提示词中写入时间戳，
# 现在把更新的信息写进后面的消息。
# 小节：重复计算与前缀不变性 · KV cache 的建立与使用 · 前缀缓存。
# ======================================================================

# 本部分的出发点：生成的每一步都再次运行模型，最直接的做法是整段重算。
def recompute(p):
    p.title('逐步生成中的重复计算')
    p.slide('推理时，每采样一个 token 就追加到输入，前面的 token 被一次又一次地输入模型。', autobold=False)
    p.notes('图中模型已经处理了 a fluffy blue creature roamed 五个 token，采样得到 the 之后追加到输入。整段重新运行时，旧的五个位置在第 1 层再次计算查询、键、值、注意力与多层感知机，其后每一层也是如此。图中斜线标出旧位置被重复执行的计算。需要先判断这些结果是否仍然有效。')
    p.frames('assets/recompute-1.svg', 'assets/recompute-2.svg', 'assets/recompute-3.svg', width_px=1040)
    p.slide('旧的 token 在每一层的计算都会再次执行，这些结果会因新 token 而改变吗？', autobold=False)


# 回答上一页的问题：一层中只有注意力读取其他位置，而它受因果掩码限制，旧位置的结果因此不变。
def prefix_invariance(p):
    p.title('因果计算的前缀不变性')
    p.slide('因果掩码下，一个位置的每一层只依赖它自己和它之前的位置。', autobold=False)
    p.notes('一层由注意力块和多层感知机组成。多层感知机对每个向量单独计算，只有注意力读取其他位置，而它受因果掩码限制。图中 creature 在每一层的查询只读取位置 1 到 4 的键和值，读取 roamed 与 the 的连线被掩码切断。图中的 h_4^(l) 是第 4 个位置在第 l 层输出的隐藏状态。按层归纳：第 1 层的输入是嵌入，旧位置不变。若第 l 层旧位置的输出不变，第 l+1 层的因果注意力只读取旧前缀，多层感知机也不涉及新位置，旧位置的输出仍不变。前提是推理时参数固定、位置的处理方式不变。编辑前缀、更换参数或改变位置规则时，受影响的状态必须重算。')
    p.frames('assets/prefix-invariance-1.svg', 'assets/prefix-invariance-2.svg', width_px=1040)
    p.slide('追加新 token 不改变旧位置在任何一层的 Q、K、V 与输出，这些结果不必重算。', autobold=False)


# 新 token 的查询只读取旧位置的 K、V，把它们逐层保存即可。
def kv_cache(p):
    p.title('逐层保存的 KV cache')
    p.slide('新 token 的查询要与所有历史位置的键比较，并汇总它们的值。', autobold=False)
    p.notes('新 token 在每一层计算自己的 Q、K、V，读取该层缓存的历史 K、V 完成注意力，再把自己的 K、V 追加到该层缓存。它完成第 l 层后，用这一层输出的隐藏状态 h_6^(l) 计算第 l+1 层的 Q、K、V。')
    p.frames('assets/kv-cache-1.svg', 'assets/kv-cache-2.svg', 'assets/kv-cache-3.svg', 'assets/kv-cache-4.svg', width_px=1040).footnote('[Hugging Face · Transformers 的缓存机制](https://huggingface.co/docs/transformers/main/cache_explanation)')
    p.slide('每层缓存历史位置的 K、V。新 token 只计算自己的 Q、K、V，并追加 K、V。', autobold=False)


# 为什么缓存 K、V 而不缓存 Q。
def query_lifetime(p):
    p.title('查询与键值的使用范围')
    p.slide('每一步的查询只在这一步使用，但键和值会被之后每一步的查询读取。因此只有 K 和 V 需要缓存，Q 不需要被缓存。', autobold=False)
    p.notes('旧位置的查询在它们自己那一步已经完成了输出，之后的步骤会产生新的查询。所以只需要保存 K、V。')
    p.image('assets/query-lifetime.svg', width_px=1040)


# 新小节的出发点：上一小节的 KV cache 只在一个请求之内使用，请求结束后释放。
# 以 Claude Code 为例：所有用户运行同一个客户端，每个请求都以同一段系统提示词开头，
# 服务器为每个请求重新计算这段提示词的 K、V。
def request_recompute(p):
    p.title('请求之间的重复计算')
    p.slide('Claude Code 的用户运行同一个客户端，每个请求都以同一段系统提示词开头。', autobold=False)
    p.notes('Claude Code 是 Anthropic 的命令行编程助手。它的系统提示词说明它的工作方式和各个工具的用法，图中只写出第一句，代码是示意。许多用户运行同一个 Claude Code 客户端，各自提出不同的问题。客户端把系统提示词和用户的问题接成一段输入，作为一个请求发往 Anthropic 的服务器。系统提示词中还嵌有每个用户自己的内容，比如记忆文件的路径，各个用户相同的是它前面固定的部分。一个请求是送给模型的一段输入，模型从它开始逐个生成 token，直到结束。服务器在一个请求结束后释放它的 KV cache，下一个请求到来时，再计算一遍同一段系统提示词在各层的 K、V，图中用斜线标出。同一个用户的下一轮对话以之前的全部对话开头，之前的对话也被再次计算。')
    p.image('assets/request-recompute.svg', width_px=1040).footnote('[Claude Code 的 npm 包 @anthropic-ai/claude-code（2.1.87）](https://unpkg.com/@anthropic-ai/claude-code@2.1.87/cli.js)')
    p.highlight('所有请求共用的系统提示词，K、V 能否只计算一次？')


# 回答上一页的问题：一个位置在每一层的 K、V 只依赖它和它之前的 token，两个请求从第一个 token
# 起相同的部分 K、V 也相同，可以保留下来复用，即前缀缓存。缓存的各个序列按公共前缀合并成
# 一棵树，新请求沿树匹配最长前缀。显存有限，淘汰时只淘汰叶结点，按最久没有使用的顺序。
def prefix_tree(p):
    p.title('前缀缓存')
    p.slide('两个请求从第一个 token 起相同的部分，各层的 K、V 也相同，可以保留下来复用。', autobold=False)
    p.notes('因果掩码下，一个位置在每一层的 K、V 只依赖它自己和它之前的 token。服务器在请求结束后保留它的 KV cache，新请求找出与已缓存的序列相同的最长前缀，直接使用这些位置的 K、V，只为其余的 token 计算，这称为前缀缓存（prefix caching）。复用只到第一个不同的 token 为止，之后的位置在注意力中读取了这个不同的 token，K、V 也不同。同一个 token 出现在不同的位置，位置嵌入不同，K、V 也不同，所以匹配从第一个 token 开始。缓存的各个序列按公共前缀合并成一棵树，称为前缀树。树的每个结点保存一段 token 和这些 token 在各层的 K、V，从根到一个结点的路径是一个已缓存的前缀。图中所有请求共用系统提示词这个结点，用户 A 的两轮共用前几段。第 1 帧中新请求沿树向下匹配到回答 1 为止，复用这条路径上的 K、V，为问题 4 新建一个结点。显存有限，缓存放满时要淘汰一部分结点。内部结点是它下面各个序列的前缀，所以只淘汰叶结点，在叶结点中先淘汰最久没有使用的，即 LRU。第 2 帧淘汰了用户 B 的回答 3。被淘汰的前缀再次出现时重新计算。推理系统已经采用这种组织方式，比如 SGLang 的 RadixAttention 用这样的树保存 KV cache，按 LRU 淘汰叶结点。vLLM 按块查找，每一块的标识是这一块的 token 与它之前整个前缀的哈希值，标识相同就表示从第一个 token 起的前缀相同。')
    p.frames('assets/prefix-tree-1.svg', 'assets/prefix-tree-2.svg', width_px=1040)
    p.slide('显存装不下时，先淘汰最久没有使用的叶结点。', autobold=False)


# 前缀缓存只复用到第一个不同的 token 为止。由此看 Agent harness 的一种实现：在系统提示词中写入
# 当前时间，每次请求从时间处开始不同，缓存大量未命中。所以设计时应当尽量固定前缀。
def prompt_order(p):
    p.title('固定前缀')
    p.slide('为了让 Agent 能够感知时间，一种实现是在每次请求的系统提示词中写入当前时间，但这会导致时间之后的提示词的 KV cache 全部无法复用。', autobold=False)
    p.notes('Agent harness 是调用模型的程序，它组织每次请求的提示词，比如 Claude Code 的客户端。有的实现希望模型知道当前时间，于是在每次请求的系统提示词中写入当前时间。时间每次都不同，每个请求从这里开始与缓存的前缀不同，时间之后的系统提示词、工具的说明和之前的对话都要重新计算，缓存大量未命中。图中下方是这种实现，上方把时间写在最后一条消息中，问题之前的部分全部复用。系统提示词、工具的说明和作为参考的文档每次都相同，放在前面。用户的问题和当前时间每次不同，放在末尾。模型的服务商也提供前缀缓存，命中缓存的输入 token 按较低的价格计费。服务商按组织隔离缓存，不同组织的请求即使前缀相同，也不共用缓存。Claude Code 的请求就按这个顺序排列：系统提示词与工具的说明在前，然后是项目的上下文，最后是对话，并使用 Anthropic 接口的前缀缓存。')
    p.image('assets/prompt-order.svg', width_px=1040)
    p.slide('设计 Agent harness 时应当尽量固定前缀，变化的内容放在末尾。', autobold=False)


# 一则趣闻：上一页的情形在 Claude Code 中发生过。Anthropic 的博客给出了 Claude Code 的提示词布局，
# 并写到曾经在静态的系统提示词中写入时间戳而破坏这个顺序。图与引文取自博客原文。
def news_anthropic_blog(p):
    p.title('新闻：Anthropic 博客')
    p.side_image('assets/news-anthropic-blog.png', alt='contain', width='52%', side='right', valign='middle').footnote('[Thariq Shihipar · Lessons from building Claude Code: Prompt caching is everything（2026-04-30）](https://claude.com/blog/lessons-from-building-claude-code-prompt-caching-is-everything)')
    p.slide('Anthropic 2026 年 4 月的博客给出了 Claude Code 的提示词布局，并写道：', autobold=False)
    p.aside('But this approach can be surprisingly fragile. We’ve broken this ordering before for a variety of reasons, including: putting an in-depth timestamp in the static system prompt, shuffling tool order definitions non-deterministically, and updating parameters of tools (e.g., what agents the Agent tool can call).')
    p.notes('博客的作者是 Thariq Shihipar，2026 年 4 月 30 日发表。右图是博客中 Claude Code 的提示词布局，从上到下依次是基础的系统指令、工具、CLAUDE.md 与记忆、会话状态、每轮增长的消息，越往下变化越多。引文的意思是：这种排列很脆弱，Claude Code 曾经因为几种原因破坏它，包括在静态的系统提示词中写入详细的时间戳、工具定义的顺序不固定、修改工具的参数，比如 Agent 工具可以调用哪些 agent。写入时间戳就是上一页的情形，时间每次不同，之后的内容都不能命中缓存。博客的做法是把会过时的信息放进对话中的消息，Claude Code 在下一条用户消息或工具的结果中加一个 system-reminder 标签，写入更新后的信息。博客还提到，几个百分点的缓存未命中率就会显著影响成本和延迟。')


# 上一页说几个百分点的未命中率就会显著影响成本，这一页算出一次会话的差距：按 Requesty 对编码 Agent
# 的统计估算每个请求新增的 token 数，得到填满 300,000 个 token 的上下文要多少个请求，再按
# Claude Fable 5.2 的价格比较没有 KV cache 与每次都命中时输入的费用。
def cache_cost(p):
    p.title('填满上下文的费用')
    p.slide('Requesty 的统计中，Claude Code 的请求平均输入 84,000 个 token，92% 命中缓存，即每个请求约新增 6,700 个 token，填满 300,000 个 token 的上下文约需 45 个请求。', autobold=False).footnote('[Thibault Jaigu · The Coding Agent Economy（Requesty，2026-05）](https://www.requesty.ai/coding-agent-economy)')
    p.table(headers=['计费项', '价格（每百万 token）', '没有 KV cache', '每次都命中 KV cache'],
            rows=[['普通输入', '$8', '6,900,000 个，$55.20', '0'],
                  ['读取缓存', '$0.20', '0', '6,600,000 个，$1.32'],
                  ['写入缓存', '$10', '0', '300,000 个，$3.00'],
                  ['输入合计', '', '$55.20', '$4.32']],
            align=['left', 'right', 'right', 'right']).footnote('[Anthropic · Claude API 定价（2026-11-19 查询）](https://platform.claude.com/docs/en/about-claude/pricing)')
    p.slide('按 Claude Fable 5.2 的价格，两种情况输入的费用约为 $55 和 $4，相差约 13 倍。', autobold=False)
    p.notes('Requesty 是转发模型 API 请求的网关，它统计了 2025 年 5 月到 2026 年 4 月经过它的 9 个编码 Agent 的请求。2026 年 4 月 Claude Code 的请求平均输入 84,000 个 token，缓存命中率为 92%，即只有 8% 的输入需要重新计算，平均每个请求约 6,700 个 token。这部分是每个请求新增的内容，包括上一个请求的输出、工具的结果和用户的消息。报告还提到一次 Claude Code 会话通常有 50 到 200 次调用。OpenRouter 的统计与此相符，2026 年 10 月 Agent 请求的缓存命中率中位数超过 90%。从空的上下文开始，每个请求新增约 6,700 个 token，约 45 个请求填满 300,000 个 token。下面按 45 个请求恰好填满计算，每个请求新增 300,000 / 45 个 token。价格取自 Anthropic 的定价页，单位是每百万 token 的美元数，Claude Fable 5.2 的普通输入为 $8，写入缓存为 $10（保留 5 分钟），读取缓存为 $0.20，输出为 $40。没有 KV cache 时，每个请求的全部输入都按普通输入计费，45 个请求共 (300,000 / 45) × (1 + 2 + … + 45) = 6,900,000 个 token，费用为 $55.20。每次都命中时，第 i 个请求读取前 i − 1 个请求累积的 token，共 6,600,000 个，费用为 $1.32。每个请求把新增的 token 写入缓存，共 300,000 个，费用为 $3.00。两项合计 $4.32。输出在两种情况下相同，表中不计入。缓存默认保留 5 分钟，每次命中重新计时，Agent 连续发出的请求间隔远小于 5 分钟，所以能够持续命中。没有 KV cache 时，服务器还要为每个请求重新计算全部位置的 K、V，上下文越长，返回第一个 token 之前的等待越长。')


# ======================================================================
# 参考资料
# ======================================================================

def sources(p):
    p.title('参考资料')
    p.slide('- [3Blue1Brown · 深度学习第 5 章](https://www.youtube.com/watch?v=wjZofJX0v4M) · [图文版](https://www.3blue1brown.com/lessons/gpt)\n- [3Blue1Brown · 深度学习第 6 章](https://www.youtube.com/watch?v=eMlx5fFNoYc) · [图文版](https://www.3blue1brown.com/lessons/attention)\n- [Andrej Karpathy · Let\'s build GPT: from scratch, in code, spelled out](https://www.youtube.com/watch?v=kCc8FmEb1nY)\n- [Vivek · What does it mean for computers to understand language?](https://www.youtube.com/watch?v=1il-s4mgNdI)\n- [Art of the Problem · 30 Year History of ChatGPT](https://www.youtube.com/watch?v=OFS90-FX6pg)\n- [Vaswani 等 · Attention Is All You Need（2017）](https://arxiv.org/abs/1706.03762)\n- [Brown 等 · Language Models are Few-Shot Learners（2020）](https://arxiv.org/abs/2005.14165)\n- [Hugging Face · Transformers 的缓存机制](https://huggingface.co/docs/transformers/main/cache_explanation)\n- [Zheng 等 · SGLang: Efficient Execution of Structured Language Model Programs（2024）](https://arxiv.org/abs/2312.07104)\n- [vLLM · Automatic Prefix Caching](https://docs.vllm.ai/en/latest/design/prefix_caching.html)\n- [Claude Code · Prompt caching](https://code.claude.com/docs/en/prompt-caching)\n- [Anthropic · Lessons from building Claude Code: Prompt caching is everything](https://claude.com/blog/lessons-from-building-claude-code-prompt-caching-is-everything)', autobold=False)
