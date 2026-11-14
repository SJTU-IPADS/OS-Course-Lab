"""Transformer：页面正文与讲稿。图的源文件在 diagrams/，每个图一个脚本。"""

# ======================================================================
# 第一部分：GPT 的数据流与词嵌入（3Blue1Brown 深度学习第 5 章）
#
# 问题：GPT 读入一段文本，怎样一步步算出下一个词？
# 回答按第 5 章的顺序建立：先说明任务（预测下一个词的分布）和由此得到的
# 生成方法；再预览数据在网络中的流动（token、向量、注意力、MLP、末向量）；
# 然后说明深度学习对计算形式的要求（实数数组、加权和、矩阵），把 GPT-3
# 的参数分成八类矩阵；接着展开网络的入口（嵌入矩阵、嵌入空间的方向、
# 点积、上下文中的嵌入）与出口（解嵌入矩阵、Softmax、温度、logits）。
# 参数量随讲随记，入口与出口各计一次。
# 小节：预测下一个词 · Transformer 的数据流 · 深度学习的计算形式 · 词嵌入 ·
# 解嵌入与 Softmax。
# ======================================================================

# 从名称出发：GPT 的三个词分别说明任务、训练方式和网络结构。
# 视频：第 5 章 00:00–00:35。
def gpt(p):
    p.title('生成式预训练 Transformer')
    p.slide('GPT 是 Generative Pretrained Transformer 的缩写。', autobold=False)
    p.notes('Generative 指模型生成新的文本。Pretrained 指模型先在大量数据上学习参数，之后还可以通过额外的训练针对具体任务微调。Transformer 是一种神经网络，是当前这一轮人工智能发展的核心技术。本讲沿着数据在 Transformer 中的流动，逐步说明每一步的计算。')
    p.image('assets/gpt.svg', width_px=1040)


# 名称中的 Transformer 是一种结构，可以组成多种模型；由此限定本讲研究的模型。
# 视频：第 5 章 00:35–01:45。
def tasks(p):
    p.title('Transformer 的任务')
    p.slide('用 Transformer 可以构造多种模型。', autobold=False)
    p.notes('有的模型读入语音、输出文字，有的反过来由文字合成语音。由文字描述生成图像的工具也使用 Transformer。2017 年 Google 提出的原始 Transformer 用于机器翻译。ChatGPT 所用的模型读入一段文本，可能还附带图像或声音，预测接下来的内容。预测的形式是对许多可能的后续文本片段给出概率分布。')
    p.image('assets/tasks.svg', width_px=1040)
    p.slide('本讲研究的模型读入一段文本，输出下一段文本的概率分布。', autobold=False)


# 模型只预测下一个词；反复预测并采样，就得到长文本。
# 视频：第 5 章 01:45–02:10。
def generation(p):
    p.title('预测、采样与追加')
    p.slide('把预测下一个词的模型反复使用，就能生成长文本。', autobold=False)
    p.notes('给模型一段起始文本，模型输出下一个词的概率分布。从分布中随机采样一个词，追加到文本末尾，再以全部文本为输入做下一次预测。图中的起始文本是 Behold, a wild pi creature, foraging in its native。模型给 land 22% 的概率，这次采样得到的是只有 1% 的 woodland。采样是随机的，低概率的词也可能被选中。图中的百分数取整数部分。')
    p.frames('assets/generation-1.svg', 'assets/generation-2.svg', 'assets/generation-3.svg', 'assets/generation-4.svg', width_px=1040)
    p.slide('预测、采样、追加，再用加长后的文本重新预测。', autobold=False)


# 同一个循环，换用更大的模型，生成的质量明显不同。
# 视频：第 5 章 02:10–02:55。
def gpt2_gpt3(p):
    p.title('GPT-2 与 GPT-3 的续写')
    p.slide('GPT-2 与 GPT-3 是同一种模型，GPT-3 的规模大得多。', autobold=False)
    p.notes('在笔记本电脑上运行 GPT-2，反复预测和采样，得到的故事不太合理。换成调用 GPT-3 的接口，得到的故事合理，甚至推断出 pi creature 生活在一个关于数学与计算的地方。和 ChatGPT 等大语言模型对话时看到的逐词输出，就是这样反复预测与采样的结果。')
    p.image('assets/gpt2-gpt3.svg', width_px=1040)
    p.slide('从同一段起始文本续写，GPT-2 的故事前后不连贯，GPT-3 的故事连贯。', autobold=False)


# 预览数据流的第一步：输入先被切成 token。
# 视频：第 5 章 03:03–03:37。
def tokens(p):
    p.title('token')
    p.slide('Transformer 先把输入切成小段，每一段称为一个 token。', autobold=False)
    p.notes('图中的句子按 GPT-3 的分词结果切分：cleverest 被切成 cle、ve、rest 三个 token，逗号单独是一个 token。为了便于举例，后面常把一个词当作一个 token。')
    p.image('assets/tokens.svg', width_px=1040)
    p.slide('token 可以是词、词的一部分或标点，也可以是图像的小块、声音的短片段。', autobold=False)


# token 本身不能参与计算；每个 token 换成一个向量。
# 视频：第 5 章 03:37–03:55。
def token_vectors(p):
    p.title('token 的向量')
    p.slide('每个 token 对应一个向量，即一列数，用来编码这个 token 的含义。', autobold=False)
    p.notes('把向量看作高维空间中的坐标。在训练好的模型里，含义相近的词落在空间中彼此接近的位置。')
    p.frames('assets/token-vectors-1.svg', 'assets/token-vectors-2.svg', width_px=1040)
    p.slide('含义相近的词，向量也相近。', autobold=False)


# 每个向量只代表一个 token；要利用上下文，向量之间需要交换信息。
# 视频：第 5 章 03:55–04:29。
def attention_preview(p):
    p.title('注意力块：向量之间的信息传递')
    p.slide('向量序列经过注意力块，各向量相互传递信息，更新自己的值。', autobold=False)
    p.notes('machine learning model 中的 model 指机器学习模型，fashion model 中的 model 指时装模特。两处 model 开始时是同一个向量，注意力根据上下文把它们更新成不同的向量。这里所说的含义，全部编码在向量的分量中。')
    p.frames('assets/attention-preview-1.svg', 'assets/attention-preview-2.svg', width_px=1040)
    p.slide('注意力块决定上下文中哪些词与更新哪些词的含义相关，以及怎样更新。', autobold=False)


# 注意力之后的第二种块：各向量各自变换。
# 视频：第 5 章 04:29–05:13。
def mlp_preview(p):
    p.title('多层感知机：逐个向量的变换')
    p.slide('之后，向量经过多层感知机（MLP），也称前馈层。', autobold=False)
    p.notes('MLP 的运算可以理解为检验每个向量的一系列特征（图中写成问题），再根据结果更新向量。注意力块和 MLP 的计算都主要是矩阵乘法，理解这些矩阵是本讲的主要任务。两种块之间还有归一化步骤，这里略去。')
    p.frames('assets/mlp-preview-1.svg', 'assets/mlp-preview-2.svg', width_px=1040)
    p.slide('在 MLP 中，向量之间不传递信息，每个向量并行地经过同一个运算。', autobold=False)


# 两种块交替重复，最后由末尾的向量给出预测。
# 视频：第 5 章 05:13–05:38。
def layer_stack(p):
    p.title('层的重复与最后一个向量')
    p.slide('注意力块与 MLP 交替重复多次。', autobold=False)
    p.notes('目标是让整段文本中与预测相关的含义汇集到序列的最后一个向量中。对这个向量做一次运算，得到词表中每个 token 作为下一个 token 的概率。')
    p.frames('assets/layer-stack-1.svg', 'assets/layer-stack-2.svg', width_px=1040)
    p.slide('最后一个向量经过一次运算，得到所有可能的下一个 token 的概率分布。', autobold=False)


# 有了下一个 token 的分布，续写与对话都是同一个生成循环。
# 视频：第 5 章 05:38–06:35。
def chat(p):
    p.title('续写与对话')
    p.slide('给定起始文本，反复预测、采样、追加，就得到续写。', autobold=False)
    p.notes('在 ChatGPT 出现之前，GPT-3 的早期演示就是根据开头续写故事和文章。要得到聊天机器人，最简单的做法是先写一段文字，设定用户与一个乐于助人的 AI 助手对话的场景，这段文字称为系统提示词。用户的问题作为对话的第一句，模型接着预测 AI 助手会说什么。要让它表现良好，还需要额外的训练步骤。')
    p.image('assets/chat.svg', width_px=1040)
    p.slide('聊天机器人把系统提示词和用户的问题放在开头，让模型续写 AI 助手的回答。', autobold=False)


# 预览之后转向背景：模型的行为由数据确定，这是理解各个矩阵的前提。
# 视频：第 5 章 07:12–08:44。
def machine_learning(p):
    p.title('由数据确定模型的参数')
    p.slide('机器学习用数据确定模型的行为。', autobold=False)
    p.notes('图像分类和预测下一个词都需要直觉和模式识别，很难直接用代码写出步骤。机器学习不显式定义做法，而是建立一个带有可调参数的灵活结构，用大量输入与期望输出的样例调整参数，使模型模仿这些样例。最简单的例子是线性回归：输入是房屋面积，输出是价格，找一条最符合数据的直线来预测价格。')
    p.frames('assets/machine-learning-1.svg', 'assets/machine-learning-2.svg', 'assets/machine-learning-3.svg', width_px=1040)
    p.slide('线性回归的直线由斜率与截距两个参数描述，学习就是根据数据确定这两个参数。', autobold=False)


# 从两个参数到 1750 亿个参数：规模带来的问题，以及深度学习的回答。
# 视频：第 5 章 08:44–09:20。
def scale(p):
    p.title('参数规模与反向传播')
    p.slide('GPT-3 有约 1750 亿个参数。', autobold=False)
    p.notes('参数很多的模型不一定可用：它可能严重过拟合训练数据，也可能无法训练。深度学习指过去二十年里证明能够很好地扩展的一类模型。它们共用同一个训练算法：反向传播。为了让这个算法在大规模下有效，模型必须遵循特定的格式。了解这个格式，Transformer 处理语言的许多设计就有了理由。')
    p.image('assets/scale.svg', width_px=1040)
    p.slide('深度学习模型用反向传播训练，为此计算必须具有特定的形式。', autobold=False)


# 格式的第一条：输入与每一层都是实数数组。
# 视频：第 5 章 09:20–10:17。
def tensors(p):
    p.title('实数数组与逐层变换')
    p.slide('第一条：输入必须表示为实数数组。', autobold=False)
    p.notes('数组可以是一维的列表、二维的表，也常是更高维的数组，统称张量。输入数据逐层变换，每一层都是实数数组。文本模型的最后一层是一列数，表示所有可能的下一个 token 的概率分布。')
    p.frames('assets/tensors-1.svg', 'assets/tensors-2.svg', width_px=1040)
    p.slide('数据逐层变换，每一层都是实数数组，最后一层是输出。', autobold=False)


# 格式的第二条：参数只以加权和的形式作用于数据，写成矩阵乘向量。
# 视频：第 5 章 10:17–11:06。
def weights_data(p):
    p.title('权重与加权和')
    p.slide('第二条：参数只通过加权和与数据相互作用，因此称为权重。', autobold=False)
    p.notes('层与层之间还穿插一些非线性函数，它们不含参数。逐个写出的加权和通常打包成矩阵与向量的乘积，两者是同一回事。把参数写成矩阵、把数据写成向量，概念上更清楚。')
    p.frames('assets/weights-data-1.svg', 'assets/weights-data-2.svg', width_px=1040)
    p.slide('矩阵乘向量时，输出的每个分量是输入分量的加权和，权重是矩阵的一行。', autobold=False)


# 有了矩阵的视角，把 GPT-3 的全部参数按矩阵分类。
# 视频：第 5 章 11:06–11:41。
def gpt3_matrices(p):
    p.title('GPT-3 的八类权重矩阵')
    p.slide('GPT-3 的约 1750 亿个权重组织为 27,938 个矩阵，分为八类。', autobold=False)
    p.notes('选 GPT-3 是因为它的结构数字公开，也是第一个在机器学习领域之外引起广泛关注的大语言模型。27,938 = 96 × (96 × 3 + 3) + 2：96 层中每层 96 个头各有查询、键、值三个矩阵，每层还有一个输出矩阵（由各头的值升维部分合成）和两个 MLP 矩阵，另加嵌入与解嵌入矩阵。每讲到一类矩阵，就把它的参数量记入这张表。')
    p.image('assets/gpt3-matrices.svg', width_px=1040)
    p.slide('Transformer 的计算几乎全部是矩阵与向量的乘法。', autobold=False)


# 矩阵很多，需要分清哪些数是模型、哪些数是输入。
# 视频：第 5 章 11:41–12:27。
def weights_vs_data(p):
    p.title('权重与数据')
    p.slide('权重与被处理的数据要严格区分。', autobold=False)
    p.notes('图中权重用蓝色和红色表示正负，数据用灰色。同一组权重处理不同的句子，得到不同的结果。')
    p.image('assets/weights-vs-data.svg', width_px=1040)
    p.slide('权重在训练中学到，决定模型的行为。数据是某一次运行的具体输入。', autobold=False)


# 回到网络的入口：第一类矩阵把 token 变成向量。
# 视频：第 5 章 12:27–13:27。
def embedding_matrix(p):
    p.title('嵌入矩阵')
    p.slide(r'模型有一个预先确定的词表。嵌入矩阵 $W_E$ 为词表中的每个 token 保存一列。', autobold=False)
    p.notes('为了举例方便，这里把 token 当作完整的词。GPT-3 的词表有 50,257 个 token。W_E 的值从随机数开始，在训练中根据数据学习。把词变成向量在 Transformer 之前已是机器学习的常用做法。')
    p.frames('assets/embedding-matrix-1.svg', 'assets/embedding-matrix-2.svg', width_px=1040)
    p.slide(r'输入的每个 token 取出 $W_E$ 中对应的一列，作为它的向量。', autobold=False)


# 嵌入是高维空间中的点；要观察它，先说明怎样把高维投影到三维。
# 视频：第 5 章 13:27–14:21。
def embedding_space(p):
    p.title('嵌入向量的几何')
    p.slide('嵌入向量可以看作高维空间中的点。GPT-3 的嵌入有 12,288 维。', autobold=False)
    p.notes('三个数可以画成三维空间中的点。把三维空间中的点投影到一个二维平面上，与把高维向量投影到一个三维子空间是同样的思路。高维空间中有大量不同的方向，这一点在后面的计算中很重要。')
    p.frames('assets/embedding-space-1.svg', 'assets/embedding-space-2.svg', width_px=1040)
    p.slide('选取一个三维子空间，把向量投影上去，就能画出词的位置。', autobold=False)


# 投影之后可以观察：训练得到的嵌入中，相近的点含义相近。
# 视频：第 5 章 14:21–14:58。
def tower_neighbors(p):
    p.title('tower 的近邻')
    p.slide('训练调整嵌入的过程中，空间中的方向逐渐带有语义。', autobold=False)
    p.notes('这里的几何例子来自 GloVe 词向量模型（50 维）。它不是 Transformer，但足以说明空间中的方向可以带有语义。代码需要先安装 gensim，第一次运行时下载模型数据。图中的近邻按欧氏距离检索。')
    p.code('python', 'import gensim.downloader  # You need to pip install gensim\nmodel = gensim.downloader.load("glove-wiki-gigaword-50")\nmodel["tower"]')
    p.image('assets/tower-neighbors.svg', width_px=520)
    p.slide('与 tower 距离最近的嵌入，都是与塔和建筑相关的词。', autobold=False)


# 近邻说明位置有含义；差向量说明方向有含义。
# 视频：第 5 章 14:58–15:50。
def gender_direction(p):
    p.title('性别方向')
    p.slide('woman 与 man 的嵌入之差，接近 queen 与 king 的嵌入之差。', autobold=False)
    p.notes('若不知道女性君主这个词，可以计算 king 加上 woman 减 man，再找离结果最近的嵌入。在这个模型中，queen 的实际位置与这个点有一段距离，因为语料中 queen 的用法不只是 king 的女性形式。家庭关系的词更好地体现了这个方向：uncle 与 aunt、brother 与 sister、nephew 与 niece、father 与 mother、son 与 daughter。')
    p.frames('assets/gender-direction-1.svg', 'assets/gender-direction-2.svg', 'assets/gender-direction-3.svg', width_px=1040)
    p.slide('训练使嵌入空间中的一个方向编码性别信息。', autobold=False)


# 性别之外，别的方向也带有语义。
# 视频：第 5 章 15:50–16:34。
def analogies(p):
    p.title('词向量的类比与近邻')
    p.slide('嵌入空间中还有许多方向带有语义。', autobold=False)
    p.notes('Italy 减 Germany 再加 Hitler，结果接近 Mussolini，好像模型学到了一个表示意大利的方向和一个表示二战轴心国领导人的方向。在一些模型中，Germany 减 Japan 加到 sushi 上，结果接近 bratwurst（德国香肠）。在 cat 的近邻里还能找到 beast 和 monster。')
    p.image('assets/analogies.svg', width_px=1040)
    p.slide('近邻检索和向量的加减揭示了模型从语料中学到的关联。', autobold=False)


# 要定量地问“两个方向是否一致”，需要点积。
# 视频：第 5 章 16:34–17:06。
def dot_product(p):
    p.title('点积')
    p.slide('两个向量的点积衡量它们的方向是否一致。', autobold=False)
    p.notes('计算点积就是把对应分量相乘再相加。这也是一种加权和，符合深度学习对计算形式的要求。点积在注意力的计算中起核心作用。')
    p.frames('assets/dot-product-1.svg', 'assets/dot-product-2.svg', width_px=1040)
    p.slide('方向相近时点积为正，垂直时为零，相反时为负。', autobold=False)


# 用点积检验一个设想的语义方向。
# 视频：第 5 章 17:06–17:50。
def plural_direction(p):
    p.title('复数方向')
    p.slide(r'设 $\mathrm{plur} = E(\mathrm{cats}) - E(\mathrm{cat})$，检验它是否表示复数。', autobold=False)
    p.notes('把 plur 与若干单数名词和对应的复数名词分别做点积，再比较。复数名词的值一致地更高，说明它们与这个方向更一致。数词 one、two、three、four 的点积依次增大，好像可以定量地衡量模型认为一个词有多“复数”。图中的数值是视频中显示的值。')
    p.image('assets/plural-direction.svg', width_px=1040)
    p.slide('复数名词与 plur 的点积高于单数名词，数词的点积随数量增大。', autobold=False)


# 嵌入矩阵讲完，把它记入参数账目。
# 视频：第 5 章 17:50–18:25。
def embedding_params(p):
    p.title('嵌入矩阵的参数量')
    p.slide('嵌入矩阵是模型的第一组权重。', autobold=False)
    p.notes('GPT-3 的词表大小为 50,257，嵌入维数为 12,288，两者相乘约为 6.18 亿。整个模型的参数共 175,181,291,520 个。')
    p.image('assets/embedding-params.svg', width_px=1040)
    p.slide(r'GPT-3 的嵌入矩阵有 $12{,}288 \times 50{,}257 = 617{,}558{,}016$ 个权重。', autobold=False)


# 嵌入矩阵给出的是孤立词的向量；Transformer 中的嵌入还要吸收上下文。
# 视频：第 5 章 18:25–19:24。
def contextual_king(p):
    p.title('吸收上下文的嵌入')
    p.slide('Transformer 中的嵌入还编码词的位置，更重要的是能吸收上下文。', autobold=False)
    p.notes('从 king 的嵌入出发的向量，经过网络中各个块的更新，最后指向一个更具体的方向：一位生活在苏格兰、谋杀前任而即位、用莎士比亚式的语言描写的国王。理解一个词要依赖周围的文字，有时依赖很远处的文字。所以预测下一个词的模型需要有效地利用上下文。')
    p.frames('assets/contextual-king-1.svg', 'assets/contextual-king-2.svg', 'assets/contextual-king-3.svg', width_px=1040)
    p.slide('初始嵌入只表示一个词。网络逐层更新嵌入，使它表示更丰富、更具体的含义。', autobold=False)


# 吸收上下文有范围：网络一次能处理的向量数是固定的。
# 视频：第 5 章 19:24–20:23。
def context_size(p):
    p.title('上下文长度')
    p.slide('网络一次只能处理固定数量的向量，称为上下文长度。GPT-3 的上下文长度为 2,048。', autobold=False)
    p.notes('第一步得到的每个向量都直接取自嵌入矩阵，只编码单个词。网络的主要目标是让每个向量吸收比单个词丰富得多的含义。早期版本的 ChatGPT 在对话很长之后会显得忘记前面的内容，原因就在这个长度限制。')
    p.image('assets/context-size.svg', width_px=1040)
    p.slide('流经网络的数据是 2,048 列、每列 12,288 维的数组。窗口之外的文本不参与预测。', autobold=False)


# 跳到网络的出口：最后一个向量怎样变成下一个 token 的分布。
# 视频：第 5 章 20:23–21:15。
def unembedding(p):
    p.title('解嵌入矩阵与下一个词')
    p.slide('输出是下一个 token 的概率分布。', autobold=False)
    p.notes('文本中出现了 Harry Potter，紧挨着最后一个词的是 least favourite teacher，最后一个词是 Professor。训练良好、熟悉哈利·波特的网络会给 Snape 很高的概率。计算分两步：先用一个矩阵把最后一个向量映射为一列数，每个 token 一个值；再用 Softmax 把这列数归一化为概率分布。')
    p.frames('assets/unembedding-1.svg', 'assets/unembedding-2.svg', 'assets/unembedding-3.svg', width_px=1040)
    p.slide(r'解嵌入矩阵 $W_U$ 把最后一个向量映射为 50,257 个值，Softmax 再把它们归一化为概率。', autobold=False)


# 只用最后一个向量做预测，其余向量的用途需要说明。
# 视频：第 5 章 21:15–21:45。
def all_predictions(p):
    p.title('每个位置的预测')
    p.slide('最后一层还有许多其他向量，生成时只用最后一个。', autobold=False)
    p.notes('训练时让最后一层的每个向量同时预测紧跟在它后面的 token，训练效率高得多。一段训练文本因此同时提供多个预测目标。')
    p.image('assets/all-predictions.svg', width_px=1040)
    p.slide('训练时，最后一层的每个向量同时预测紧跟在它后面的 token。', autobold=False)


# 出口的矩阵讲完，记入参数账目。
# 视频：第 5 章 21:45–22:22。
def unembedding_params(p):
    p.title('解嵌入矩阵的参数量')
    p.slide(r'$W_U$ 的每一行对应词表中的一个 token，每行有 12,288 个元素。', autobold=False)
    p.notes('W_U 与嵌入矩阵的形状行列对调，元素同样从随机数开始，在训练中学习。参数账目累计到 1,235,116,032，是 1750 亿中很小但不可忽略的一部分。')
    p.image('assets/unembedding-params.svg', width_px=1040)
    p.slide(r'$W_U$ 有 $50{,}257 \times 12{,}288 = 617{,}558{,}016$ 个参数，与嵌入矩阵合计约 12.4 亿。', autobold=False)


# 出口的第二步：把任意一列数变成概率分布。
# 视频：第 5 章 22:22–23:30。
def softmax(p):
    p.title('Softmax')
    p.slide('概率分布要求每个值在 0 与 1 之间，且总和为 1。矩阵乘法的输出不满足这两条。', autobold=False)
    p.notes('矩阵与向量相乘得到的值常常是负数或远大于 1，总和也几乎不会是 1。取指数后所有值都为正，再除以总和就归一化为和为 1 的一列数。最大的输入对应最接近 1 的输出，较小的输入对应接近 0 的输出。')
    p.frames('assets/softmax-1.svg', 'assets/softmax-2.svg', 'assets/softmax-3.svg', width_px=1040)
    p.slide(r'Softmax 先对每个数取 $e$ 的幂，再除以它们的总和：$p_i = e^{x_i} / \sum_j e^{x_j}$。', autobold=False)


# Softmax 的名称来自它与“取最大值”的区别。
# 视频：第 5 章 23:30–23:55。
def softmax_behavior(p):
    p.title('Softmax 的取值特点')
    p.slide('Softmax 是一种“柔和”的取最大值。', autobold=False)
    p.notes('若从这个分布采样，一个明显最大的输入几乎总会被选中。与直接取最大值不同，其他值同样大时，它们也分得可观的概率。输入连续变化时，输出也连续变化。')
    p.frames('assets/softmax-behavior-1.svg', 'assets/softmax-behavior-2.svg', 'assets/softmax-behavior-3.svg', width_px=1040)
    p.slide('一个输入明显最大时，它几乎占据全部概率。几个输入同样大时，它们共同分得概率。', autobold=False)


# 生成时可以调节 Softmax 的集中程度。
# 视频：第 5 章 23:55–24:33。
def temperature(p):
    p.title('温度')
    p.slide(r'Softmax 的指数中可以除以一个常数 $T$，称为温度。', autobold=False)
    p.notes('这个名称来自它与某些热力学公式中温度的相似。ChatGPT 用这个分布生成下一个词时，可以用温度调节随机性。T = 0 按极限理解，即总是选择最大的值。')
    p.frames('assets/temperature-1.svg', 'assets/temperature-2.svg', 'assets/temperature-3.svg', width_px=1040)
    p.slide(r'$T$ 越大，分布越均匀。$T$ 越小，最大值越占优势。$T = 0$ 时全部权重落在最大值上。', autobold=False)


# 温度对生成结果的影响。
# 视频：第 5 章 24:33–25:33。
def temperature_stories(p):
    p.title('温度与生成的故事')
    p.slide('让 GPT-3 从 Once upon a time, there was a 开始写故事，取不同的温度。', autobold=False)
    p.notes('温度为 0 时，模型总是选择最可能的词，故事是 Goldilocks 的老套翻版。较高的温度让模型有机会选择不太可能的词，故事从一位韩国的年轻网络艺术家开始，但很快变得没有意义。当时的 API 不允许温度超过 2，这不是数学上的限制，只是为了避免工具生成过于荒谬的内容。视频中的高温演示取 API 返回的前 20 个候选，把概率取 1/5 次方后重新归一化，相当于在这 20 个候选上取 T = 5。')
    p.image('assets/temperature-stories.svg', width_px=1040)
    p.slide('温度为 0 时故事老套。温度较高时开头更新颖，但有失去连贯的风险。', autobold=False)


# Softmax 的输入、输出各有名称。
# 视频：第 5 章 25:33–26:03。
def logits(p):
    p.title('logits')
    p.slide('Softmax 的输出称为概率，输入称为 logits。', autobold=False)
    p.notes('输入文本后，词嵌入流经整个网络，最后与解嵌入矩阵相乘。这个原始、未归一化的输出的各个分量，就是预测下一个 token 的 logits。')
    p.image('assets/logits.svg', width_px=1040)
    p.slide('最后一个向量乘以解嵌入矩阵，得到的未归一化输出是下一个 token 的 logits。', autobold=False)


# ======================================================================
# 第二部分：注意力机制（3Blue1Brown 深度学习第 6 章）
#
# 问题：查表得到的嵌入与上下文无关，怎样让每个嵌入吸收上下文？
# 先用例子说明需要的行为（mole、tower、推理小说），再用一个注意力头构造
# 计算：查询与键给出相关程度，Softmax 与掩码把它变成权重，值向量决定加
# 什么，加权求和与残差完成更新。随后计参数量，引出值映射的低秩分解，
# 说明交叉注意力这一变体；再从多种上下文关系引出多头、输出矩阵与多层，
# 以全部注意力参数的总量和可并行性结束。
# 小节：注意力要解决的问题 · 查询、键与注意力模式 · 掩码与注意力模式的规模 ·
# 值与嵌入的更新 · 一个注意力头的参数 · 交叉注意力与多头注意力 · 多层与规模。
# ======================================================================

# 本部分的出发点：嵌入要从词义变为上下文中的含义。
# 视频：第 6 章 00:00–01:46。
def attention_goal(p):
    p.title('注意力机制的目标')
    p.slide('注意力机制由 2017 年的论文 *Attention Is All You Need* 提出。', autobold=False)
    p.notes('嵌入空间中的方向可以对应语义，例如有一个方向对应性别。可以设想还有许多方向，分别对应词义的其他方面。初始嵌入只编码单个词，Transformer 的目标是逐步调整这些嵌入，使它们带有丰富得多的上下文含义。注意力机制常让人困惑，下面先看几个希望注意力实现的行为，再看计算。')
    p.image('assets/attention-goal.svg', width_px=1040)
    p.slide('注意力逐步调整嵌入，使每个嵌入从表示单个词变为表示带有上下文的含义。', autobold=False)


# 第一个例子：同一个词，初始嵌入相同，含义却不同。
# 视频：第 6 章 01:46–02:57。
def mole(p):
    p.title('同一个词的不同含义')
    p.slide('mole 在下面三个短语中含义各不相同。', autobold=False)
    p.notes('American shrew mole 中是鼹鼠，One mole of carbon dioxide 中是摩尔，Take a biopsy of the mole 中是痣。分词并查表之后，三处 mole 的向量完全相同，因为初始嵌入是一张查表，不涉及上下文。到下一步，周围的嵌入才有机会把信息传给它。可以设想嵌入空间中有几个不同的方向，分别编码 mole 的不同含义。')
    p.frames('assets/mole-1.svg', 'assets/mole-2.svg', 'assets/mole-3.svg', width_px=1040)
    p.slide('注意力根据上下文计算要加到通用嵌入上的量，把它移向具体含义的方向。', autobold=False)


# 第二个例子：上下文可以一步步把含义限定得更具体。
# 视频：第 6 章 02:57–03:32。
def tower_refine(p):
    p.title('逐步具体化的含义')
    p.slide('tower 的嵌入是一个笼统的方向，与许多高大的名词相近。', autobold=False)
    p.notes('前面出现 Eiffel 时，希望注意力把 tower 的向量更新到更具体地表示埃菲尔铁塔的方向，与巴黎、法国和钢铁制品相关。再出现 miniature 时，向量应进一步更新，不再与高大的事物相关。')
    p.frames('assets/tower-refine-1.svg', 'assets/tower-refine-2.svg', 'assets/tower-refine-3.svg', width_px=1040)
    p.slide('上下文中的每个词都可能进一步限定一个词的含义。', autobold=False)


# 第三个例子：信息需要跨越很远的距离，汇集到最后一个向量。
# 视频：第 6 章 03:32–04:42。
def long_range(p):
    p.title('远距离的信息传递')
    p.slide('注意力可以在相距很远的嵌入之间传递信息，内容可以比单个词丰富得多。', autobold=False)
    p.notes('输入几乎是一整部推理小说，直到接近结尾的一句 Therefore, the murderer was。要预测下一个词，即凶手的名字，最后一个向量最初只是 was 的嵌入，它必须经过所有注意力块的更新，编码上下文中与预测相关的全部信息。')
    p.frames('assets/long-range-1.svg', 'assets/long-range-2.svg', width_px=1040)
    p.slide('预测下一个 token 只用最后一个向量，它必须汇集整个上下文中与预测相关的信息。', autobold=False)


# 从例子转向计算：用一个简单的设想行为构造一个注意力头。
# 视频：第 6 章 04:42–06:14。
def fluffy_example(p):
    p.title('一个注意力头的例子')
    p.slide('设想一个注意力头只做一种更新：形容词调整其后名词的含义。', autobold=False)
    p.notes('例句是 a fluffy blue creature roamed the verdant forest。这里描述的是一个注意力头，完整的注意力块由许多头并行组成。初始嵌入记为 E，它编码词本身和词在上下文中的位置。计算仍主要是矩阵乘向量，矩阵中是可学习的权重。“形容词更新名词”是为说明而设想的行为。真实的头的行为由训练中大量参数的调整决定，难以解释。')
    p.frames('assets/fluffy-example-1.svg', 'assets/fluffy-example-2.svg', width_px=1040)
    p.slide('目标是算出一组新的嵌入，使名词的嵌入吸收修饰它的形容词的含义。', autobold=False)


# 计算的第一步：名词提出“要找什么”。
# 视频：第 6 章 06:14–07:41。
def query(p):
    p.title('查询向量')
    p.slide('设想 creature 在问：我前面有形容词吗？这个问题编码为一个向量，称为查询。', autobold=False)
    p.notes('查询的维数远小于嵌入，GPT-3 中为 128。W_Q 的元素是模型参数，它在一个头中实际做什么很难解释。这里设想它把名词的嵌入映射到查询空间中一个表示“寻找前面的形容词”的方向，它对其他嵌入的作用暂不考虑。')
    p.frames('assets/query-1.svg', 'assets/query-2.svg', 'assets/query-3.svg', width_px=1040)
    p.slide(r'查询 $q_i = W_Q E_i$。$W_Q$ 乘以上下文中的每个嵌入，每个 token 得到一个 128 维的查询。', autobold=False)


# 第二步：每个词给出“能提供什么”，与查询比较。
# 视频：第 6 章 07:41–08:17。
def key(p):
    p.title('键向量')
    p.slide(r'第二个矩阵 $W_K$ 同样乘以每个嵌入，得到键向量。键可以看作对查询的回答。', autobold=False)
    p.notes('W_K 同样由可调参数组成，把嵌入映射到与查询相同的 128 维空间。在这个例子中，W_K 把 fluffy、blue 等形容词映射为与 creature 的查询方向接近的键。')
    p.frames('assets/key-1.svg', 'assets/key-2.svg', width_px=1040)
    p.slide('键与查询位于同一个空间。键与查询方向接近时，认为二者匹配。', autobold=False)


# 第三步：用点积同时比较每一对键与查询。
# 视频：第 6 章 08:17–09:29。
def attention_scores(p):
    p.title('键与查询的点积表')
    p.slide('每一对键和查询做点积，衡量它们的匹配程度。', autobold=False)
    p.notes('圆点越大，点积越大。fluffy、blue 的键与 creature 的查询点积为较大的正数。the 的键与 creature 的查询点积较小或为负，说明两者无关。这张表中的值可以是任意实数，给出每个词与更新其他每个词的含义的相关程度。')
    p.frames('assets/attention-scores-1.svg', 'assets/attention-scores-2.svg', 'assets/attention-scores-3.svg', width_px=1040)
    p.slide('点积大时，称 fluffy、blue 的嵌入关注（attend to）creature 的嵌入。', autobold=False)


# 分数要作为加权求和的权重，需要归一化。
# 视频：第 6 章 09:29–10:18。
def attention_pattern(p):
    p.title('注意力模式')
    p.slide('这些分数将用作每一列加权求和的权重，因此每个值应在 0 与 1 之间，每列之和为 1。', autobold=False)
    p.notes('对每一列做 Softmax 即可。归一化之后，每一列给出左侧各词与顶部这个词的相关程度。')
    p.frames('assets/attention-pattern-1.svg', 'assets/attention-pattern-2.svg', 'assets/attention-pattern-3.svg', width_px=1040)
    p.slide('对每一列做 Softmax，得到的表称为注意力模式。', autobold=False)


# 以上步骤合起来，就是论文中的注意力公式。
# 视频：第 6 章 10:18–11:05。
def attention_formula(p):
    p.title('注意力的矩阵公式')
    p.slide('原始论文把这些计算写成一个紧凑的式子。', autobold=False)
    p.notes('Q、K 分别是全部查询向量与全部键向量组成的数组。分子紧凑地表示所有键与查询的点积。除以 √d_k 是为了数值稳定。V 是值向量组成的数组，用于更新嵌入。论文中每个 token 占一行，QKᵀ 的每一行对应一个查询，Softmax 沿行进行。前面的表以查询为列，是它的转置，对应的写法是 V·softmax(KᵀQ/√d_k)，V 的每一列是一个值向量。')
    p.image('assets/attention-formula.svg', width_px=1040)
    p.slide(r'点积除以键查询空间维数的平方根 $\sqrt{d_k}$，再对每个查询做 Softmax。', autobold=False)


# 训练方式对注意力模式提出一个限制。
# 视频：第 6 章 11:05–11:49。
def training_targets(p):
    p.title('每个位置的预测目标')
    p.slide('训练时，模型同时预测每个前缀之后的下一个 token。', autobold=False)
    p.notes('训练根据模型给真实下一个词的概率高低，微调全部权重。对于例句，模型同时预测 creature 之后的词、the 之后的词，等等。')
    p.frames('assets/training-targets-1.svg', 'assets/training-targets-2.svg', width_px=1040)
    p.slide('一个训练样本因此相当于多个样本。', autobold=False)


# 同时预测各个位置，后面的词就不能影响前面的词。
# 视频：第 6 章 11:49–12:42。
def causal_mask(p):
    p.title('因果掩码')
    p.slide('后面的词不能影响前面的词，否则它们会泄露要预测的答案。', autobold=False)
    p.notes('表中后面的 token 影响前面 token 的位置都必须为 0。最简单的想法是直接把它们设为 0，但这样各列之和不再为 1。GPT 总是使用掩码。掩码在训练中最关键，模型作为聊天机器人运行时也同样使用。有些注意力的变体不使用掩码。')
    p.frames('assets/causal-mask-1.svg', 'assets/causal-mask-2.svg', 'assets/causal-mask-3.svg', height_px=400)
    p.slide(r'现在红色格子的位置被设定为 0，各列仍归一化。这称为掩码。', autobold=False)


# 注意力模式的大小随上下文长度平方增长。
# 视频：第 6 章 12:42–13:10。
def context_cost(p):
    p.title('注意力模式的规模')
    p.slide('注意力模式的元素个数是上下文长度的平方。', autobold=False)
    p.notes('为了支持更长的上下文，出现了多种注意力的变体，例如 Sparse Attention、Blockwise Attention、Linformer、Reformer、Ring Attention、Longformer、Adaptive Attention Span。本部分只讨论基本的注意力。')
    p.image('assets/context-cost.svg', width_px=1040)
    p.slide('上下文长度因此是大语言模型的重要瓶颈，扩大它并不容易。', autobold=False)


# 注意力模式只说明谁与谁相关；下一步决定传递什么。
# 视频：第 6 章 13:10–14:39。
def value(p):
    p.title('值向量')
    p.slide('有了注意力模式，下一步是真正更新嵌入：让 fluffy 改变 creature 的嵌入。', autobold=False)
    p.notes('值矩阵 W_V 乘以 fluffy 的嵌入 E_2，得到值向量 v_2。先介绍最直接的做法，多头注意力中会略作修改。W_V 乘以一个词的嵌入，可以理解为：如果这个词与调整另一个词的含义相关，应该往那个词的嵌入上加什么。')
    p.frames('assets/value-1.svg', 'assets/value-2.svg', 'assets/value-3.svg', width_px=1040)
    p.slide(r'值向量 $v_2 = W_V E_2$ 加到 creature 的嵌入上，它与嵌入位于同一个空间。', autobold=False)


# 每个位置可能有多个相关的词，按注意力权重汇总它们的值向量。
# 视频：第 6 章 14:39–15:49。
def weighted_values(p):
    p.title('值的加权求和')
    p.slide(r'$W_V$ 乘以每个嵌入，得到一列值向量，每个值向量与同一行的键对应。', autobold=False)
    p.notes('得到注意力模式后，查询和键就不再需要。以 creature 这一列为例，fluffy 和 blue 的值向量以较大比例加入，其余值向量几乎为零。相加得到的变化量 ΔE 加到 creature 的原嵌入上，结果有望编码“毛茸茸的蓝色生物”这样更丰富的含义。对所有列做同样的加权求和，就得到一整列更新后的嵌入。以上全部过程称为一个注意力头。')
    p.frames('assets/weighted-values-1.svg', 'assets/weighted-values-2.svg', 'assets/weighted-values-3.svg', 'assets/weighted-values-4.svg', width_px=1040)
    p.slide(r"每列按权重把值向量相加得到 $\Delta E$，再加到原嵌入上：$E' = E + \Delta E$。", autobold=False)


# 一个头讲完，开始计算它的参数量。
# 视频：第 6 章 15:49–16:24。
def head_params(p):
    p.title('查询矩阵与键矩阵的参数量')
    p.slide('一个头由三个矩阵确定：查询、键和值。', autobold=False)
    p.notes('W_Q 与 W_K 的列数等于嵌入维数 12,288，行数等于键查询空间的维数 128。')
    p.image('assets/head-params.svg', width_px=1040)
    p.slide(r'$W_Q$、$W_K$ 各有 $128 \times 12{,}288 = 1{,}572{,}864$ 个参数。', autobold=False)


# 按上面的写法，值矩阵是方阵，参数远多于查询与键；实践中把它分解。
# 视频：第 6 章 16:24–18:11。
def value_lowrank(p):
    p.title('值映射的低秩分解')
    p.slide('值矩阵的输入与输出都在 12,288 维的嵌入空间，若写成方阵，约有 1.5 亿个参数。', autobold=False)
    p.notes('完整的方阵是可行的设计，但实践中让值映射的参数与查询、键相当效率更高，在多头并行时尤其如此。概念上仍可把它看作嵌入空间到嵌入空间的线性映射，例如把 blue 的嵌入映射到可以加到名词上的“蓝色”方向。第一个矩阵这里称为值降维矩阵 Value↓，第二个称为值升维矩阵 Value↑，这不是通用的名称。分解后先降到 128 维，再升回 12,288 维，秩至多为 128。一个头共有 4 个同样大小的矩阵，6,291,456 个参数。')
    p.frames('assets/value-lowrank-1.svg', 'assets/value-lowrank-2.svg', 'assets/value-lowrank-3.svg', width_px=1040)
    p.slide('实践中把值映射分解为降维与升维两个矩阵之积，整体是低秩变换。', autobold=False)


# 同样的计算，换一种数据来源，就是交叉注意力。
# 视频：第 6 章 18:11–19:12。
def cross_attention(p):
    p.title('交叉注意力')
    p.slide('以上是自注意力头。交叉注意力头处理两种不同的数据。', autobold=False)
    p.notes('翻译模型中，键来自一种语言的原文，查询来自正在生成的另一种语言的译文，注意力模式描述两种语言中哪些词相互对应。语音转写中，键来自语音输入，查询来自正在写出的文字。这里不存在后面的 token 影响前面的 token 的问题，所以通常不加掩码。GPT 只使用自注意力。')
    p.image('assets/cross-attention.svg', width_px=1040)
    p.slide('交叉注意力的计算与自注意力相同，只是键和查询来自不同的数据，通常不加掩码。', autobold=False)


# 回到自注意力：一个头只能表达一种关系，而上下文的关系有许多种。
# 视频：第 6 章 19:12–20:31。
def context_kinds(p):
    p.title('多种上下文关系')
    p.slide('上下文影响词义的方式远不止形容词修饰名词。', autobold=False)
    p.notes('若 car 之前出现 they crashed the，说明这辆车的形状和结构受到影响。许多关联与语法无关：同一段文字中出现 wizard，Harry 很可能指 Harry Potter；出现 Queen、Sussex、William，Harry 的嵌入应更新为指向哈里王子。每种上下文更新需要不同的查询、键参数来捕捉注意力模式，也需要不同的值映射决定加到嵌入上的内容。实际中这些映射的行为更难解释，权重被训练成最有利于预测下一个 token 的样子。')
    p.image('assets/context-kinds.svg', width_px=1040)
    p.slide('每种关系需要各自的查询、键参数与值映射。', autobold=False)


# 多种关系需要多组参数：多个头并行。
# 视频：第 6 章 20:31–22:03。
def multi_head(p):
    p.title('多头注意力')
    p.slide('注意力块由多个头并行组成，每个头有自己的查询、键和值映射。GPT-3 每个块有 96 个头。', autobold=False)
    p.notes('96 组查询与键矩阵产生 96 个注意力模式，每个头用自己的值映射产生一列值向量，再按自己的注意力模式加权求和。并行运行许多不同的头，使模型能够学到许多种由上下文改变词义的方式。')
    p.frames('assets/multi-head-1.svg', 'assets/multi-head-2.svg', 'assets/multi-head-3.svg', width_px=1040)
    p.slide('对每个位置，每个头提出一个变化量。所有头的变化量相加，再加到该位置的原嵌入上。', autobold=False)


# 多头之后更新参数账目。
# 视频：第 6 章 22:03–22:16。
def block_params(p):
    p.title('多头注意力块的参数量')
    p.slide('每个头有四个矩阵，共 6,291,456 个参数。', autobold=False)
    p.notes('每个头的四个矩阵是查询、键、值降维和值升维矩阵，大小相同。')
    p.image('assets/block-params.svg', width_px=1040)
    p.slide(r'96 个头的注意力块有 $96 \times 6{,}291{,}456 = 603{,}979{,}776$ 个参数，约 6 亿。', autobold=False)


# 论文和实现中的写法与上面的分头写法不同，需要对应起来。
# 视频：第 6 章 22:16–23:11。
def output_matrix(p):
    p.title('输出矩阵')
    p.slide(r'实现中把各头的值升维矩阵拼成一个大矩阵，称为输出矩阵 $W_O$。', autobold=False)
    p.notes('输出矩阵属于整个多头注意力块。这只是写法与实现上的差别，参数相同。在每个头中保留一对值降维、值升维矩阵也是正确的实现。阅读其他资料时要注意，论文中某个头的“值矩阵”只指值降维矩阵。')
    p.frames('assets/output-matrix-1.svg', 'assets/output-matrix-2.svg', 'assets/output-matrix-3.svg', width_px=1040)
    p.slide(r'各头先在 128 维中加权求和，拼接后乘 $W_O$，等于各头分别升维后求和。', autobold=False)


# 一个注意力块之外，数据还要经过许多层。
# 视频：第 6 章 23:11–24:16。
def deep_layers(p):
    p.title('多层中的上下文')
    p.slide('数据还经过 MLP，并反复经过许多层注意力块与 MLP。', autobold=False)
    p.notes('一个词吸收部分上下文之后，更精细的嵌入还有许多机会受到同样更精细的周围嵌入的影响。深层的嵌入有能力编码超出修饰关系和语法结构的信息，例如情感、语气、这是否是一首诗、与内容相关的科学事实。图中以 Frost 的诗 The Road Not Taken 为例：I took the one less traveled by 中的 one，逐层获得“两条路中的一条”“象征选择”“与熟悉之路相对”等含义。')
    p.frames('assets/deep-layers-1.svg', 'assets/deep-layers-2.svg', width_px=1040)
    p.slide('越往深层，每个嵌入从越来越精细的其他嵌入中吸收含义，能够编码更抽象的信息。', autobold=False)


# 层数确定之后，可以算出全部注意力参数。
# 视频：第 6 章 24:16–24:54。
def attention_total(p):
    p.title('注意力参数的总量')
    p.slide('GPT-3 有 96 层，每层注意力块的参数再乘以 96。', autobold=False)
    p.notes('查询、键、值、输出四类矩阵各有 14,495,514,624 个参数。注意力获得了最多的关注，但大部分参数位于注意力块之间的 MLP 中。')
    p.image('assets/attention-total.svg', width_px=1040)
    p.slide('全部注意力头共有 57,982,058,496 个参数，约 580 亿，只占约 1750 亿的三分之一。', autobold=False)


# 注意力成功的另一个原因在于计算形式。
# 视频：第 6 章 24:54–25:22。
def parallel_gpu(p):
    p.title('注意力的可并行性')
    p.slide('注意力的大部分计算相互独立，可以高度并行。', autobold=False)
    p.notes('注意力的成功，很大程度上不在于它实现的某种具体行为，而在于它高度可并行，能用 GPU 在短时间内完成大量计算。过去十几年深度学习的一个重要经验是：规模本身就能显著提升模型的表现。')
    p.image('assets/parallel-gpu.svg', width_px=1040)
    p.slide('可并行的结构能用 GPU 在短时间内完成大量计算，从而扩大模型规模。', autobold=False)


# ======================================================================
# 第三部分：KV cache
#
# 问题：生成时每采样一个 token，就把它追加到输入并再次运行模型。把整段
# 重新运行时，旧位置的计算是否必须重做？
# 先看整段重算重复了哪些计算。为回答旧位置的结果是否改变，逐个看一层中
# 的运算读取哪些位置：FFN、归一化与残差逐位置进行，只有注意力读取其他
# 位置，而它受因果掩码限制。由此证明追加新 token 不改变旧位置的任何
# 一层，旧位置各层的 K、V 可以保存复用，得到逐层的 KV cache。最后说明
# 只缓存 K、V 的原因与生成的两个阶段。
# 小节：重复计算与前缀不变性 · KV cache 的建立与使用。
# ======================================================================

# 本部分的出发点：生成的每一步都再次运行模型，最直接的做法是整段重算。
def recompute(p):
    p.title('逐步生成中的重复计算')
    p.slide('生成时，每采样一个 token 就追加到输入。最直接的做法是把整段重新运行一遍。', autobold=False)
    p.notes('图中模型已经处理了 a fluffy blue creature roamed 五个 token，采样得到 the 之后追加到输入。整段重新运行时，旧的五个位置在第 1 层再次计算查询、键、值、注意力与 MLP，其后每一层也是如此。需要先判断这些结果是否仍然有效。')
    p.frames('assets/recompute-1.svg', 'assets/recompute-2.svg', 'assets/recompute-3.svg', width_px=1040)
    p.slide('旧位置在每一层的计算都再次执行。这些结果会因新 token 而改变吗？', autobold=False)


# 要回答上一页的问题，需要知道一层中每个运算读取哪些位置。先看注意力之后的 FFN。
def feedforward(p):
    p.title('逐位置的前馈网络')
    p.slide('注意力之后是前馈网络（FFN），即 MLP。它对每个位置的 $d$ 维向量分别变换，$d$ 为嵌入维数。', autobold=False)
    p.notes('判断旧位置的结果是否改变，要看一层中每个运算读取哪些位置。以两矩阵的 FFN 为例：W_1 为 4d×d，先升维到 4d，经过非线性函数（如 GELU），W_2 为 d×4d，再降回 d。各位置共用同一组参数，位置之间不交换信息。门控 FFN 等变体的矩阵个数与参数量不同。')
    p.image('assets/feedforward.svg', width_px=1040)
    p.slide(r'$\mathrm{FFN}(x) = W_2\,\phi(W_1 x + b_1) + b_2$ 只读取一个位置的向量，与其他位置之间不交换信息。', autobold=False)


# 归一化与残差同样逐位置进行，一层中只有注意力读取其他位置。
def full_block(p):
    p.title('完整的解码器层')
    p.slide('归一化沿每个 token 的特征维调整数值。它与注意力、FFN 和残差连接组成一层。', autobold=False)
    p.notes('图中是 GPT 系列常用的 pre-norm 结构：每个子层先归一化，再计算更新，最后与输入相加。2017 年的原始 Transformer 把归一化放在残差相加之后（post-norm）。归一化只在一个 token 的特征内进行，不引入位置之间的依赖。这样的层重复 L 次，GPT-3 中 L = 96。')
    p.frames('assets/full-block-1.svg', 'assets/full-block-2.svg', 'assets/full-block-3.svg', width_px=1040)
    p.slide('归一化、FFN 与残差都逐位置进行，一层中只有注意力读取其他位置。', autobold=False)


# 注意力受因果掩码限制，由此回答旧位置的结果是否改变。
def prefix_invariance(p):
    p.title('因果计算的前缀不变性')
    p.slide('因果掩码下，一个位置的每一层只依赖它自己和它之前的位置。', autobold=False)
    p.notes('按层归纳：第 1 层的输入是嵌入，旧位置不变；若第 l 层旧位置的输出不变，第 l+1 层的因果注意力只读取旧前缀，逐位置的归一化与 FFN 不涉及新位置，旧位置的输出仍不变。前提是推理时权重固定、位置的处理方式不变。编辑前缀、更换权重或改变位置规则时，受影响的状态必须重算。')
    p.frames('assets/prefix-invariance-1.svg', 'assets/prefix-invariance-2.svg', width_px=1040)
    p.slide('追加新 token 不改变旧位置的任何一层，旧位置各层的 K、V 可以复用。', autobold=False)


# 可复用的是 K、V；把它们逐层保存。
def kv_cache(p):
    p.title('逐层保存的 KV cache')
    p.slide('新 token 的查询要与所有历史位置的键比较，并汇总它们的值。', autobold=False)
    p.notes('新 token 在每一层计算自己的 Q、K、V，读取该层缓存的历史 K、V 完成注意力，再把自己的 K、V 追加到该层缓存。它完成第 l 层后，用更新后的向量计算第 l+1 层的 Q、K、V。')
    p.frames('assets/kv-cache-1.svg', 'assets/kv-cache-2.svg', 'assets/kv-cache-3.svg', 'assets/kv-cache-4.svg', width_px=1040).footnote('[Hugging Face · Transformers 的缓存机制](https://huggingface.co/docs/transformers/main/cache_explanation)')
    p.slide('每层缓存历史位置的 K、V。新 token 只计算自己的 Q、K、V，并追加 K、V。', autobold=False)


# 为什么缓存 K、V 而不缓存 Q。
def query_lifetime(p):
    p.title('查询与键值的使用范围')
    p.slide('每一步的查询只在这一步使用。键和值被之后每一步的查询读取。', autobold=False)
    p.notes('旧位置的查询在它们自己那一步已经完成了输出，之后的步骤会产生新的查询。所以只需要保存 K、V。')
    p.image('assets/query-lifetime.svg', width_px=1040)
    p.slide('当前查询决定本步的权重，历史键用于匹配，历史值用于汇总。', autobold=False)


# 有了缓存，生成分成两个阶段。
def prefill_decode(p):
    p.title('prefill 与 decode')
    p.slide('prefill 一次处理整个提示词并建立缓存。decode 每步只处理一个新 token。', autobold=False)
    p.notes('prefill 处理 P 个提示词 token，最后一个位置预测第一个生成的 token。采样得到它之后，decode 把它送入模型、追加它的 K、V，并预测下一个。生成 G 个 token 通常需要一次 prefill 和 G−1 次 decode。刚采样得到、尚未送入模型的 token 还没有 K、V。')
    p.frames('assets/prefill-decode-1.svg', 'assets/prefill-decode-2.svg', width_px=1040)
    p.slide('prefill 并行处理已知的前缀。decode 每步处理一个新 token，读取不断增长的历史 K、V。', autobold=False)


# ======================================================================
# 参考资料
# ======================================================================

def sources(p):
    p.title('参考资料')
    p.slide('- [3Blue1Brown · 深度学习第 5 章](https://www.youtube.com/watch?v=wjZofJX0v4M) · [图文版](https://www.3blue1brown.com/lessons/gpt)\n- [3Blue1Brown · 深度学习第 6 章](https://www.youtube.com/watch?v=eMlx5fFNoYc) · [图文版](https://www.3blue1brown.com/lessons/attention)\n- [Andrej Karpathy · Let\'s build GPT: from scratch, in code, spelled out](https://www.youtube.com/watch?v=kCc8FmEb1nY)\n- [Chris Olah 的博客](https://colah.github.io/)\n- [Vivek · What does it mean for computers to understand language?](https://www.youtube.com/watch?v=1il-s4mgNdI)\n- [Art of the Problem · 30 Year History of ChatGPT](https://www.youtube.com/watch?v=OFS90-FX6pg)\n- [Vaswani 等 · Attention Is All You Need（2017）](https://arxiv.org/abs/1706.03762)\n- [Brown 等 · Language Models are Few-Shot Learners（2020）](https://arxiv.org/abs/2005.14165)\n- [Hugging Face · Transformers 的缓存机制](https://huggingface.co/docs/transformers/main/cache_explanation)', autobold=False)
