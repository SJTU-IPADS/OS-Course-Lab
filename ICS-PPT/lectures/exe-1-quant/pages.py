"""习题课《把 Q4_0 写出来》的页面内容。

组织方式：一次课的时间写完 `nano-quant` 的 A 部分与 Q4_0，写完之后
`./nq-selftest a q4_0` 的六项全部与参考一致。第一部分先把实验对象打开：
课上讲义只说过这个文件有多大，本节把它的 625 个张量、层的形状与字节账目
逐项读出来；第二部分说明框架的分工与三个程序；第三、四部分逐个写六个函数；
第五部分对自查的六行输出。

全部命令与输出都在本机跑过：Intel i9-11900H，gcc 15.2.0，Python 3.14.4，
模型文件是 Qwen/Qwen3-VL-2B-Instruct 的 `model.safetensors`，4 255 140 312 字节。
`nano-quant` 的路径按仓库布局写成 `../2-data/nano-quant`。

用语要求：面向本科课程教学，采用陈述性的技术表述；不使用比喻、口语化措辞。

排版约定：
- 粗体后面不要紧跟全角冒号。`**词：**内容` 不符合 CommonMark 的闭合规则，
  写成 `**词**：内容`。
- `==标记==` 只在 `p.slide(...)` 里展开，写进 highlight / 图注 / 表格会变成字面量。
- 一页最多一个 `p.highlight`，并且只给真正的结论用。
- 代码注释统一写英文，中英两版共用。
"""

# ==============================================================================
# 共享代码清单
# ==============================================================================

ST_JSON = """{"__metadata__":{"format":"pt"},
 "model.language_model.embed_tokens.weight":
   {"dtype":"BF16","shape":[151936,2048],"data_offsets":[0,622329856]},
 "model.language_model.layers.0.input_layernorm.weight":
   {"dtype":"BF16","shape":[2048],"data_offsets":[622329856,622333952]},
 ...}"""

RD_U64LE = """uint64_t rd_u64le(const uint8_t *p) {
    uint64_t x = 0;
    for (int i = 7; i >= 0; i--)          /* high byte first */
        x = (x << 8) | (uint64_t) p[i];
    return x;
}"""

RD_U64LE_BAD = """uint64_t rd_u64le_bad(const uint8_t *p) {
    uint64_t x = 0;
    for (int i = 0; i < 8; i++)
        x |= p[i] << (8 * i);             /* p[i] promotes to int */
    return x;
}"""

BF16_CODE = """float bf16_to_f32(uint16_t h) {
    uint32_t bits = (uint32_t) h << 16;   /* the low 16 bits were dropped */
    float f;
    memcpy(&f, &bits, 4);
    return f;
}"""

FP16_CODE = """float fp16_to_f32(uint16_t h) {
    uint32_t sign = (uint32_t) (h & 0x8000) << 16;
    uint32_t exp  = (uint32_t) (h >> 10) & 0x1f;
    uint32_t mant = (uint32_t) h & 0x3ff;
    uint32_t out;

    if (exp == 0x1f)    out = sign | 0x7f800000u | (mant << 13);   /* inf/NaN  */
    else if (exp != 0)  out = sign | ((exp + 112) << 23) | (mant << 13);
    else if (mant == 0) out = sign;                                /* zero     */
    else {                                                         /* subnormal */
        int e = -1;
        do { mant <<= 1; e++; } while (!(mant & 0x400));
        out = sign | ((uint32_t) (112 - e) << 23) | ((mant & 0x3ff) << 13);
    }
    float f;
    memcpy(&f, &out, 4);
    return f;
}"""

F32_TO_FP16_NORMAL = """    int32_t e = exp - 127 + 15;
    if (e >= 0x1f) return (uint16_t) (sign | 0x7c00u);  /* overflow -> inf */

    if (e > 0) {                                        /* normal */
        uint32_t m = mant >> 13;                        /* kept bits   */
        uint32_t r = mant & 0x1fff;                     /* dropped bits */
        uint32_t h = sign | ((uint32_t) e << 10) | m;
        if (r > 0x1000 || (r == 0x1000 && (m & 1))) h++;
        return (uint16_t) h;
    }"""

F32_TO_FP16_SUB = """    if (e < -10) return (uint16_t) sign;                /* too small -> zero */

    mant |= 0x00800000;                                 /* restore the hidden 1 */
    int32_t  shift = 14 - e;
    uint32_t m     = mant >> shift;
    uint32_t r     = mant & ((1u << shift) - 1);
    uint32_t half  = 1u << (shift - 1);
    if (r > half || (r == half && (m & 1))) m++;
    return (uint16_t) (sign | m);"""

Q4_0_LAYOUT = """byte  0  1 |  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17
      d(fp16)|            qs[16], two 4-bit codes per byte"""

Q4_0_PAIRING = """qs[0]   low nibble <- x[0]     high nibble <- x[16]
qs[1]   low nibble <- x[1]     high nibble <- x[17]
...
qs[15]  low nibble <- x[15]    high nibble <- x[31]"""

Q4_0_AMAX = """float amax = 0.0f, max = 0.0f;
for (int j = 0; j < 32; j++) {
    float v = x[j];
    if (amax < fabsf(v)) { amax = fabsf(v); max = v; }
}"""

Q4_0_SCALE = """const float d  = max / -8.0f;
const float id = d ? 1.0f / d : 0.0f;   /* multiply later, never divide */

uint16_t dh = f32_to_fp16(d);
blk[0] = (uint8_t) (dh & 0xff);         /* little endian */
blk[1] = (uint8_t) (dh >> 8);"""

Q4_0_ENCODE = """for (int j = 0; j < 16; j++) {
    const float x0 = x[j]      * id;
    const float x1 = x[j + 16] * id;
    const uint8_t q0 = (uint8_t) ((int8_t)(x0 + 8.5f) < 15 ? (int8_t)(x0 + 8.5f) : 15);
    const uint8_t q1 = (uint8_t) ((int8_t)(x1 + 8.5f) < 15 ? (int8_t)(x1 + 8.5f) : 15);
    blk[2 + j] = (uint8_t) (q0 | (q1 << 4));
}"""

Q4_0_DEQUANT = """void q4_0_dequantize(const uint8_t *blk, float *x) {
    const float d = fp16_to_f32((uint16_t) (blk[0] | (blk[1] << 8)));
    for (int j = 0; j < 16; j++) {
        x[j]      = d * (float) ((blk[2 + j] & 0xf) - 8);
        x[j + 16] = d * (float) ((blk[2 + j] >>  4) - 8);
    }
}"""

STUDENT_SKELETON = """uint64_t rd_u64le(const uint8_t *p) {
    (void) p;
    todo("rd_u64le");        /* prints the name and exits with code 3 */
    return 0;
}"""

# ==============================================================================
# 回顾与本节的安排
# ==============================================================================

def session_goal(p):
    p.title("本节要做完的事：写完六个函数，自查六项一致")
    p.slide("""
上一讲把模型权重文件里的每个字节读到了浮点数一级，最后一部分讲了量化：
把 16 位的权重压成 4 位，一个块共用一个缩放系数。
- 本节用一次课的时间，把 Q4_0 这条路从字节写到字节
- 写完之后 `./nq-selftest a q4_0` 的六行全部报「与参考一致」
""")
    p.slide("""
要写的是 `student/student.cpp` 里的==六个函数==：四个转换函数，
加上 Q4_0 的量化与解量化。
""", autobold=False)


def session_scope(p):
    p.title("本节与实验说明的分工")
    p.table(
        headers=["内容", "在哪里做", "材料"],
        rows=[
            ["A 部分的四个转换函数", "本节课上", "`EXERCISE-q4_0.md` 第一节"],
            ["Q4_0 的量化与解量化", "本节课上", "`EXERCISE-q4_0.md` 第三、四节"],
            ["Q4_1、Q4_K", "课后", "`LAB-quantize.md` B 部分"],
            ["整模型的量化与账目", "课后", "`LAB-quantize.md` C 部分"],
            ["拼成 GGUF 并加载", "课后", "`LAB-quantize.md` D 部分"],
        ],
        align=["left", "left", "left"],
    )
    p.slide("""
课上写完的这六个函数是后面每一部分的底座：三种块格式的缩放系数都由
`f32_to_fp16` 写出，读文件头的每一步都调 `rd_u64le`。
""", autobold=False)


def session_outline(p):
    p.title("本节的顺序")
    p.slide("""
- 实验对象：这个 4.25 GB 的文件里装着什么
- 代码框架：三个程序、四份文件，以及它们的分工
- A 部分：四个转换函数
- Q4_0：十八个字节怎么写、怎么读
- 自查：六行输出各自说明了什么
""", reveal="items")
    p.aside("讲义页与实验章节的对应写在 `EXERCISE-q4_0.md` 末尾的附表里。")


# ==============================================================================
# 第一部分 · 实验对象
# ==============================================================================

def one_file(p):
    p.title("实验对象是一个文件")
    p.slide("""
`Qwen/Qwen3-VL-2B-Instruct` 的权重放在一个文件里，4 255 140 312 字节。
- 文件名 `model.safetensors`，格式由 Hugging Face 定义
- 全部权重都是 BF16，一个权重两个字节
- 21 亿个权重，其中 17 亿属于语言模型
""")
    p.slide("""
本节要做的事，==全部都在这个文件的字节上==。
""", autobold=False)


def st_layout(p):
    p.title("safetensors 的三段排布")
    p.code("text", """[0, 8)        uint64  header length N, little endian
[8, 8+N)      UTF-8 JSON header
[8+N, EOF)    tensor data, back to back""")
    p.slide("""
JSON 头的每个键是一个张量名，值给出三样东西：`dtype`、`shape`、`data_offsets`。
`data_offsets` 相对数据区起点，左闭右开。
""", autobold=False)
    p.slide("""
框架读第一段的 8 个字节时调的就是==你写的 `rd_u64le`==。
""", autobold=False)


def st_head_bytes(p):
    p.title("文件开头的三十二个字节")
    p.demo("看文件的前 32 个字节", "xxd -l 32 ~/Downloads/model.safetensors",
           output="""00000000: d029 0100 0000 0000 7b22 5f5f 6d65 7461  .)......{"__meta
00000010: 6461 7461 5f5f 223a 7b22 666f 726d 6174  data__":{"format""")
    p.slide("""
前 8 个字节是 `d0 29 01 00 00 00 00 00`，按小端读出 ==76240==，即 JSON 头的长度。
第 9 个字节是 `7b`，也就是 `{`，JSON 从这里开始。
""", autobold=False)
    p.aside("按大端读同样这 8 个字节，得到的是 1.5×10^19，`nano-quant` 会报出来并提示检查移位方向。")


def st_record(p):
    p.title("JSON 头里的两条记录")
    p.code("json", ST_JSON)
    p.slide("""
- `dtype` 是 `BF16`，全文件 625 个张量都是这个类型
- `shape` 行优先，`shape[0]` 变化最慢
- `data_offsets` 的两个数相减即字节数，`[0, 622329856)` 是 6.22 亿字节
""")


def st_groups(p):
    p.title("六百二十五个张量分成两堆")
    p.demo("按名字前缀汇总", "python3 examples/sthead.py ~/Downloads/model.safetensors",
           output="""header 76240 bytes, 625 tensors
model.language_model.embed_tokens           1    622329856
model.language_model.layers               308   2818816000
model.language_model.norm                   1         4096
model.visual.blocks                       288    604618752
model.visual.deepstack_merger_list         18    151080960
model.visual.merger                         6     50348032
model.visual.patch_embed                    2      3147776
model.visual.pos_embed                      1      4718592""",
           files=["examples/sthead.py"])
    p.slide("""
`model.language_model.` 开头的 ==310 个==张量是语言模型，
`model.visual.` 开头的 315 个是视觉塔。本实验只处理前者。
""", autobold=False)


def two_towers(p):
    p.title("语言模型与视觉塔的字节账")
    p.table(
        headers=["部分", "张量数", "字节数", "占比"],
        rows=[
            ["语言模型 `model.language_model.`", "310", "3 441 149 952", "80.9%"],
            ["视觉塔 `model.visual.`", "315", "813 914 112", "19.1%"],
            ["合计", "625", "4 255 064 064", "100%"],
        ],
        align=["left", "right", "right", "right"],
    )
    p.slide("""
数据区合计 4 255 064 064 字节，加上 8 字节的头长与 76 240 字节的 JSON 头，
正好是文件的 4 255 140 312 字节。
""", autobold=False)
    p.aside("视觉塔把图像切成小块编码成向量，与本实验要处理的矩阵形状不同，`tensor_selected` 把它整个跳过。")


def one_layer(p):
    p.title("一层里的十一个张量")
    p.demo("列出第 0 层", "python3 examples/sthead.py ~/Downloads/model.safetensors \\\n  model.language_model.layers.0.",
           output="""input_layernorm.weight         BF16  [2048]               4096
mlp.down_proj.weight           BF16  [2048, 6144]     25165824
mlp.gate_proj.weight           BF16  [6144, 2048]     25165824
mlp.up_proj.weight             BF16  [6144, 2048]     25165824
post_attention_layernorm.weight BF16  [2048]               4096
self_attn.k_norm.weight        BF16  [128]                 256
self_attn.k_proj.weight        BF16  [1024, 2048]      4194304
self_attn.o_proj.weight        BF16  [2048, 2048]      8388608
self_attn.q_norm.weight        BF16  [128]                 256
self_attn.q_proj.weight        BF16  [2048, 2048]      8388608
self_attn.v_proj.weight        BF16  [1024, 2048]      4194304""")
    p.slide("""
28 层，每层这 11 个张量，形状逐层相同。==308 = 28 × 11==。
""", autobold=False)


def shapes_to_hyperparams(p):
    p.title("从形状反推出模型的超参数")
    p.table(
        headers=["观察到的形状", "推出的超参数", "值"],
        rows=[
            ["`embed_tokens [151936, 2048]`", "词表大小 × 隐藏维", "151936 × 2048"],
            ["`layers.N`，N 最大到 27", "层数", "28"],
            ["`q_proj [2048, 2048]`", "查询头数 × 头维", "16 × 128"],
            ["`k_proj [1024, 2048]`", "键值头数 × 头维", "8 × 128"],
            ["`q_norm [128]`", "头维", "128"],
            ["`gate_proj [6144, 2048]`", "前馈中间维", "6144"],
        ],
        align=["left", "left", "right"],
    )
    p.slide("""
`config.json` 不在本实验的输入里。这些数==全部由张量的形状定出来==。
""", autobold=False)


def attention_shapes(p):
    p.title("注意力的四个矩阵")
    p.slide("""
一层的注意力用四个二维张量，每一个都是一次矩阵乘的权重。
- `q_proj [2048, 2048]`：把 2048 维的输入映成 16 个 128 维的查询向量
- `k_proj [1024, 2048]`、`v_proj [1024, 2048]`：只映出 ==8 组==键与值
- `o_proj [2048, 2048]`：把拼起来的 16 × 128 映回 2048 维
""")
    p.slide("""
16 个查询头共用 8 组键值，每两个查询头共用一组。键值缓存的体积因此减半。
""", autobold=False)
    p.aside("`q_norm`、`k_norm` 各 128 个系数，在每个头的 128 维上各做一次归一化，逐头进行。")


def mlp_shapes(p):
    p.title("前馈的三个矩阵")
    p.slide("""
`gate_proj` 与 `up_proj` 各把 2048 维升到 6144 维，`down_proj` 再降回 2048 维。
- 两个升维矩阵的输出逐元素相乘，其中一路先过一次激活函数
- 三个矩阵各 25 165 824 字节，合起来是一层里最大的一块
""")
    p.table(
        headers=["部分", "字节数", "占一层"],
        rows=[
            ["注意力四个矩阵", "25 165 824", "25.0%"],
            ["前馈三个矩阵", "75 497 472", "75.0%"],
            ["四个一维张量", "8 704", "0.009%"],
            ["一层合计", "100 672 000", "100%"],
        ],
        align=["left", "right", "right"],
    )


def norm_tensors(p):
    p.title("一维张量与二维张量的分工")
    p.table(
        headers=["维数", "个数", "字节数", "占语言模型"],
        rows=[
            ["一维（各种归一化系数）", "113", "247 808", "0.007%"],
            ["二维（各种矩阵）", "197", "3 440 902 144", "99.993%"],
        ],
        align=["left", "right", "right", "right"],
    )
    p.slide("""
一维张量逐元素乘在激活值上，全部加起来不到语言模型的万分之一，压缩它省不下字节，
误差却直接加在每个通道上。所以实验的配方规定：==一维张量一律留 F32==。
""", autobold=False)
    p.highlight("能压的字节全部在那 197 个二维张量里。", tone="blue")


def account_before(p):
    p.title("量化之前就能把账算出来")
    p.demo("按 Q4_0 排一遍清单", """cd ../2-data/nano-quant
./nano-quant-ref plan ~/Downloads/model.safetensors --recipe q4_0 2>/dev/null | head -4""",
           output="""token_embd.weight Q4_0 2048 151936 175030272
blk.0.attn_norm.weight F32 2048 8192
blk.0.ffn_down.weight Q4_0 6144 2048 7077888
blk.0.ffn_gate.weight Q4_0 2048 6144 7077888""")
    p.slide("""
`plan` ==不读一个权重==，只读 JSON 头。每个张量的产物字节数由元素个数、
块的大小与块的字节数三个整数算出：一个 Q4_0 块 32 个元素、18 字节。
""", autobold=False)


def account_summary(p):
    p.title("排出来的账目")
    p.demo("汇总走标准错误", """cd ../2-data/nano-quant
./nano-quant-ref plan ~/Downloads/model.safetensors --recipe q4_0 >/dev/null""",
           output="""配方 q4_0，28 层，310 个张量，1720574976 个权重
源 3441149952 字节，产物张量数据 968249344 字节，平均 4.5020 位/权重，压缩 3.554 倍
跳过 315 个张量，813914112 字节""")
    p.slide("""
每个权重平均 4.5020 位：32 个权重 128 位，加上一个 fp16 的缩放系数 16 位，
除以 32 得 4.5 位；一维张量留 F32 把平均值抬高了 0.002 位。
""", autobold=False)
    p.aside("清单走标准输出、汇总走标准错误，所以 `plan` 的输出可以直接接给 `awk` 求和，与这三行不混在一起。")


# ==============================================================================
# 第二部分 · 代码框架
# ==============================================================================

def framework_three_programs(p):
    p.title("框架给出三个程序")
    p.table(
        headers=["程序", "做什么", "要不要模型文件"],
        rows=[
            ["`nq-selftest`", "把六个函数与参考实现逐项对照", "不要"],
            ["`nano-quant`", "`plan` 排清单，`quant` 逐段量化写 `.nq`", "要"],
            ["`nq2gguf`", "把 `.nq` 与一段元数据拼成 GGUF", "要"],
        ],
        align=["left", "left", "center"],
    )
    p.slide("""
本节只用 `nq-selftest`。它不读模型文件，跑一次不到两秒，
写一个函数就能跑一次。
""", autobold=False)


def framework_dirs(p):
    p.title("四份文件的归属")
    p.table(
        headers=["路径", "归谁", "说明"],
        rows=[
            ["`include/nano_quant.h`", "只读", "全部函数的原型与注释，判定只按它调用"],
            ["`student/student.cpp`", "你写", "本节的全部工作都在这里"],
            ["`src/`（11 个文件）", "只读", "safetensors 解析、类型表、容器、md5"],
            ["`Makefile`", "只读", "编译选项是判定的一部分"],
        ],
        align=["left", "center", "left"],
    )
    p.slide("""
`Makefile` 里有两个选项要单独说：`-ffp-contract=off` 禁止把 `a*b+c` 合成一条
FMA 指令，`-fno-fast-math` 不允许编译器假定浮点可结合。==两条都改不得==。
""", autobold=False)


def framework_interface(p):
    p.title("学生与框架之间的唯一接口")
    p.code("c", """/* A: read and assemble bits */
uint64_t rd_u64le(const uint8_t *p);
float    bf16_to_f32(uint16_t h);
float    fp16_to_f32(uint16_t h);
uint16_t f32_to_fp16(float f);

/* B: one block format, this session's target */
void q4_0_quantize(const float *x, uint8_t *blk);     /* 32 floats -> 18 bytes */
void q4_0_dequantize(const uint8_t *blk, float *x);   /* 18 bytes -> 32 floats */""")
    p.slide("""
全部函数都是纯函数：只读入参、只写出参，==不分配内存，不打印，不退出==。
""", autobold=False)


def framework_todo(p):
    p.title("骨架里每个函数体是一句 todo")
    p.code("c", STUDENT_SKELETON)
    p.slide("""
调到谁就在谁那里停下。写完一个，报错里的名字换成下一个没写的，
所以这个名字直接指出接着写谁。
""", autobold=False)
    p.demo("课前的三条命令", """cd ../2-data/nano-quant
make >/dev/null && ./nq-selftest a q4_0""",
           output="student.cpp: rd_u64le 还没有实现")


def framework_groups(p):
    p.title("自查可以只跑点到名的那几组")
    p.code("text", """./nq-selftest                 run every group
./nq-selftest a q4_0          run only these two
./nq-selftest --full          walk all 2^32 bit patterns for f32_to_fp16""")
    p.slide("""
组名有 `a`、`scale_min`、`q4_0`、`q4_1`、`q4_k`、`q6_k` 六个。
本节只用 ==`a` 与 `q4_0`== 两组。
""", autobold=False)


def framework_stream(p):
    p.title("框架按段读权重")
    p.slide("""
`quant` 逐段读、逐段量化、逐段写出，一段是 4 Mi 个元素、16 MB。
- 峰值内存只与段长有关，与最大的那个张量无关
- 输入 4.25 GB，实测峰值常驻内存 48 MB
""")
    p.slide("""
所以本节写的两个块函数每次只看 ==32 个 `float`==，
既不知道自己在哪个张量里，也不需要知道。
""", autobold=False)


def framework_rules(p):
    p.title("两条纪律")
    p.slide("""
**全程用 `float`，不要用 `double` 中转。** 中间结果一旦升到 64 位，
再舍回 32 位的结果与直接算不同，产物的字节跟着不同。
""", autobold=False)
    p.slide("""
**不要改 `Makefile` 的编译选项。** 判定要求产物与参考实现逐字节相同，
编译选项是这个要求的一部分。
""", autobold=False)
    p.highlight("这两件事上任何偏离，六行里都会有几行对不上。", tone="orange")


# ==============================================================================
# 第三部分 · A 部分的四个转换函数
# ==============================================================================

def rd_u64le_page(p):
    p.title("`rd_u64le`：八个字节合成一个数")
    p.code("c", RD_U64LE)
    p.slide("""
从高位字节往低位走，每一步先左移 8 位，再或进下一个字节。
要求==显式移位合成==，禁止 `memcpy` 到 `uint64_t`：那样写出来的程序
在大端机器上读出的是另一个数，而这个格式规定了小端。
""", autobold=False)


def rd_u64le_trap(p):
    p.title("另一种写法在大文件上给出错的数")
    p.code("c", RD_U64LE_BAD)
    p.slide("""
`p[i]` 的类型是 `uint8_t`，参加运算前提升成 `int`，`int` 只有 32 位。
`i >= 4` 时移位量不小于 32，==行为未定义==。
""", autobold=False)
    p.slide("""
上一页那种写法里被移的是 `uint64_t`，没有这个问题。
自查的第三条向量是 4294967296，只有第 4 字节为 1，专门检查这一处。
""", autobold=False)


def bf16_page(p):
    p.title("`bf16_to_f32`：一次左移")
    p.code("c", BF16_CODE)
    p.slide("""
BF16 与 FP32 的符号位和 8 位指数域完全一致，尾数是 FP32 尾数的高 7 位。
所以 BF16 就是 FP32 砍掉低 16 位，这一步把它接回去。
""", autobold=False)
    p.slide("""
零、无穷、NaN、次规格化数四类==都不用单独处理==：字段位置相同，接回去自然就对。
""", autobold=False)


def bf16_memcpy(p):
    p.title("从整数取出浮点为什么用 memcpy")
    p.slide("""
写成 `*(float *)&bits` 违反 C 的严格别名规则：编译器可以按
「这两个指针指向不同的对象」来重排代码，开优化之后结果不确定。
""", autobold=False)
    p.slide("""
`memcpy` 在这里==不产生任何指令==。编译器认得这个惯用法，
四个字节的搬运被直接消掉，留下的是寄存器之间的一次传送。
""", autobold=False)
    p.aside("`nano_quant.h` 里的 `nq_round` 也用同一个惯用法，从 `float` 取出它的位型。")


def fp16_cases(p):
    p.title("`fp16_to_f32`：按指数域分四种情况")
    p.table(
        headers=["指数域", "尾数", "表示的数", "转换"],
        rows=[
            ["0", "0", "零", "只保留符号位"],
            ["0", "非 0", "次规格化数", "左移到最高位为 1，指数按移了几位算"],
            ["1..30", "任意", "规格化数", "指数加 112，尾数左移 13 位"],
            ["31", "任意", "无穷或 NaN", "指数域填满，尾数左移 13 位"],
        ],
        align=["center", "center", "left", "left"],
    )
    p.slide("""
半精度是 1 位符号、5 位指数、10 位尾数，偏置 15；单精度是 1 位符号、
8 位指数、23 位尾数，偏置 127。==112 = 127 − 15==，==13 = 23 − 10==。
""", autobold=False)


def fp16_code(p):
    p.title("`fp16_to_f32` 的四个分支")
    p.code("c", FP16_CODE)


def fp16_denormal(p):
    p.title("次规格化数那一支")
    p.slide("""
半精度的次规格化数表示 $m / 2^{10} \\times 2^{-14}$，它没有隐含的前导 1；
单精度的规格化数有。
""", autobold=False)
    p.slide("""
所以把尾数一直左移到第 10 位（`0x400`）出现 1 为止，这个 1 就是单精度的隐含位，移掉。
半精度指数域为 0 时代表的指数与为 1 时相同，对应单精度的 113；每左移一位指数减一，
移了 `e + 1` 位之后是 `113 − (e + 1)`，==即 `112 − e`==。
""", autobold=False)
    p.aside("半精度只有 65536 个位型，自查把它们全部走一遍，四个分支都会被覆盖到。")


def f32_to_fp16_intro(p):
    p.title("`f32_to_fp16`：反方向，就近舍入平局取偶")
    p.slide("""
三种块格式的缩放系数都用这个函数写出，它错了整个产物都对不上，
所以它是本节==最需要小心==的一个。
""", autobold=False)
    p.slide("""
舍入规则写成两半：被丢掉的低位记作 `r`，中点记作 `half`。
- `r > half` 时进位
- `r == half` 时看留下来的最低位，为 1 才进位，为 0 不进
""")
    p.slide("""
后半句就是平局取偶。少写这一句，全班的产物 md5 会各不相同。
""", autobold=False)


def f32_to_fp16_normal(p):
    p.title("规格化数那一支")
    p.code("c", F32_TO_FP16_NORMAL)
    p.slide("""
进位直接加在整个 16 位上。尾数进满时自然进到指数域，指数进满时自然变成无穷，
==两种情形都不用单独判断==。
""", autobold=False)


def f32_to_fp16_sub(p):
    p.title("次规格化数那一支")
    p.code("c", F32_TO_FP16_SUB)
    p.slide("""
源是单精度的规格化数，它的隐含 1 没有存在尾数里，所以先补回来再移位。
`e` 小于 −10 时半精度连最小的次规格化数都表示不了，直接归零。
""", autobold=False)


def f32_to_fp16_checks(p):
    p.title("这个函数被自查怎么查")
    p.slide("""
- 默认按质数步长抽 1700 万个位型与参考对照，`--full` 遍历全部 $2^{32}$ 个，约一分钟
- 半精度能精确表示的每一个值都要原样往返
- NaN 只检查是否仍是 NaN：指数为 255 且尾数非 0 时返回 `0x7e00`
""")
    p.highlight("四个转换函数写完，`./nq-selftest a` 的四项应当全部一致。", tone="green")


# ==============================================================================
# 第四部分 · Q4_0 的十八个字节
# ==============================================================================

def q4_0_layout(p):
    p.title("一个块是三十二个权重，压成十八字节")
    p.code("text", Q4_0_LAYOUT)
    p.slide("""
32 个权重 × 4 位 = 128 位 = 16 字节，加上 2 字节的 `d`，合计 18 字节，
平均每个权重 ==4.5 位==。原来每个权重 16 位，压缩 3.56 倍。
""", autobold=False)
    p.highlight(r"还原式只有一条：$\hat{x} = d \times (q - 8)$", tone="blue")


def q4_0_codes(p):
    p.title("十六个码字摊在哪里")
    p.slide("""
`q` 取 0 到 15，`q − 8` 取 −8 到 7，一个块共用一个 `d`。
- 16 个码字在 $[-8d,\\ 7d]$ 上等距排列，间隔就是 `d`
- 关于 0 对称的只有 15 个格子，`q = 8` 落在 0 上
""")
    p.slide("""
一个块里的 32 个权重共用一个 `d`，所以块内==量级相差很大==的一组数，
小的那些会全部落到同一个码字上。
""", autobold=False)


def q4_0_pairing(p):
    p.title("半字节的配对方式")
    p.code("text", Q4_0_PAIRING)
    p.slide("""
第 `j` 字节的低半字节放第 `j` 个权重，高半字节放第 ==`j + 16`== 个权重，
`j` 从 0 到 15。
""", autobold=False)


def q4_0_pairing_why(p):
    p.title("配成 `j` 与 `j + 1` 为什么查不出来")
    p.slide("""
把 `j` 与 `j + 1` 配成一个字节，算出来的每个权重误差都正常，
整块的顺序却是错的。
- 前面几项误差检查全都发现不了
- 只有逐字节比对能判定，也就是 `q4_0_bytes` 那一行
""")
    p.slide("""
ggml 这样配的原因在解码：SIMD 一次取 16 字节，低半字节一批、高半字节一批，
==两批各自对齐==，不需要在寄存器里再打乱一次顺序。
""", autobold=False)


def q4_0_steps(p):
    p.title("`q4_0_quantize` 分五步")
    p.slide("""
1. 找出绝对值最大的那个权重，连同它的符号
2. 由它定出步长 `d`，并预先算好倒数 `id`
3. 把 `d` 舍成半精度写进前两个字节
4. 每个权重乘 `id` 再就近舍入，得到 0 到 15 的码字
5. 两个码字打进一个字节
""", reveal="items")


def q4_0_amax(p):
    p.title("第一步：绝对值最大的权重与它的符号")
    p.code("c", Q4_0_AMAX)
    p.slide("""
两个变量各有分工：`amax` 用来比大小，`max` 保留符号，下一步定步长要用符号。
""", autobold=False)
    p.slide("""
比较写成 `amax < fabsf(v)`，取==先出现==的那个。写成 `<=` 会在有并列时
取后出现的那个，两者符号不同时定出的 `d` 差一个负号，产物就对不上。
""", autobold=False)
    p.aside("32 个权重全为 0 时两个变量都停在 0，下一步的 `d` 因此为 0。")


def q4_0_scale(p):
    p.title("第二、三步：定步长并写进前两个字节")
    p.code("c", Q4_0_SCALE)
    p.slide("""
还原式是 $\\hat{x} = d \\times (q-8)$，`q − 8` 落在 $[-8, 7]$。
让绝对值最大的那个权重正好落在范围的端点上，误差最小；端点有两个，
==−8 那一头比 7 那一头远==，所以取它。
""", autobold=False)


def q4_0_why_minus8(p):
    p.title("除以 −8 之后各种情形落在哪里")
    p.table(
        headers=["块里的情形", "`d` 的符号", "绝对值最大的那个权重"],
        rows=[
            ["`max` 为正", "负", "编码成 `q = 0`"],
            ["`max` 为负", "正", "编码成 `q = 0`"],
            ["32 个权重全为 0", "`d = 0`", "全部编码成 `q = 8`，还原回 0"],
        ],
        align=["left", "center", "left"],
    )
    p.slide("""
取 `d = amax / 7` 也能跑，但是 16 个码字只用上了 15 个，编码 0 永远不出现，
误差也更大。这一条是实验诊断部分的 q3。
""", autobold=False)


def q4_0_reciprocal(p):
    p.title("`id` 是提前算好的倒数")
    p.slide("""
后面的换算用乘法，==不用除法==。浮点的除法与「乘以倒数」的舍入结果不同，
判定要求逐字节相同，这一步照写。
""", autobold=False)
    p.slide("""
`d` 为 0 时倒数取 0，乘出来是 0，走到下一步加 8.5 再截断，正好落在 `q = 8` 上。
""", autobold=False)


def q4_0_encode(p):
    p.title("第四、五步：换算成码字并打包")
    p.code("c", Q4_0_ENCODE)
    p.slide("""
`x × (1/d) + 8.5` 再截断成整数，等于把 `x/d + 8` 就近舍入。
加 8 把 $[-8, 7]$ 挪到 $[0, 15]$，加 0.5 把截断变成就近舍入。
""", autobold=False)


def q4_0_clamp(p):
    p.title("上界为什么必须夹住")
    p.slide("""
`d` 由绝对值最大的那个权重定出，按实数算 `x/d + 8` 落在 $[0, 15]$ 之内。
- 第三步把 `d` 舍入成了半精度，`id` 是==舍入后==的 `d` 的倒数
- 个别权重因此算出 15.0000x，截断后是 15，再大一点就是 16
- 半字节装不下 16，所以要与 15 取小
""")
    p.slide("""
下界不会越出：绝对值最大的那个权重对应的是 0，舍入只会让它变成 0.0000x。
""", autobold=False)


def q4_0_dequant(p):
    p.title("`q4_0_dequantize`：反过来读，八行")
    p.code("c", Q4_0_DEQUANT)
    p.slide("""
`& 0xf` 取低半字节，`>> 4` 取高半字节。`blk` 的元素是 `uint8_t`，
右移补 0，不会带进符号位。先减 8 再乘 `d`，顺序与还原式一致。
""", autobold=False)


def q4_0_dequant_uses(p):
    p.title("解量化函数在实验里被用到三处")
    p.slide("""
- 本节的自查：压出去再读回来，与参考的浮点摘要对照
- 误差度量：`quant` 报的相对均方根误差由它算出
- 课后 D 部分：把别人的 GGUF 解开来读
""")
    p.slide("""
写错了==三处都不过==，而这个函数只有八行。
""", autobold=False)


# ==============================================================================
# 第五部分 · 自查
# ==============================================================================

def selftest_run(p):
    p.title("六行输出")
    p.demo("跑本节的两组", """cd ../2-data/nano-quant
./nq-selftest-ref a q4_0""",
           output="""q4_0  相对均方根误差 0.08408，最大绝对误差 8
bf16_to_f32  ok  与参考一致
f32_to_fp16  ok  与参考一致
fp16_to_f32  ok  与参考一致
q4_0_back    f444b647e38b04afee71056cd0b1acac  与参考一致
q4_0_bytes   942530b9c1c6ec1c41790c63de4317f8  与参考一致
rd_u64le     ok  与参考一致""",
           description="这里跑的是参考实现 `nq-selftest-ref`；写完六个函数之后 `./nq-selftest a q4_0` 应当逐字相同")
    p.slide("""
六行都写着「与参考一致」，本节就完成了。
""", autobold=False)


def selftest_meaning(p):
    p.title("每一行说明什么")
    p.table(
        headers=["行", "内容", "判据"],
        rows=[
            ["第 1 行", "相对均方根误差与最大绝对误差", "只报数，不参与比对"],
            ["`bf16_to_f32`", "全部 65536 个位型", "逐个与参考一致"],
            ["`f32_to_fp16`", "抽 1700 万个位型加往返检查", "逐个与参考一致"],
            ["`fp16_to_f32`", "全部 65536 个位型", "逐个与参考一致"],
            ["`q4_0_back`", "再还原回去的浮点的 md5", "全班同一个值"],
            ["`q4_0_bytes`", "压出来的全部字节的 md5", "全班同一个值"],
            ["`rd_u64le`", "三条向量，含 4294967296", "逐条与参考一致"],
        ],
        align=["left", "left", "left"],
    )
    p.slide("""
两个 md5 只用整数与四则运算算出，钉死 `-ffp-contract=off` 之后
在任何机器、任何编译器上都相同。
""", autobold=False)


def selftest_input(p):
    p.title("这组输入是怎么构造的")
    p.slide("""
误差那一行来自一组固定的 512 个超块，前 8 个是刻意构造的：
- 全零块、全相等的块、只有一个离群值的块
- 全为负数的块、全为正数的块、量级 1e-7 的块
- 关于 0 对称的块、只有一个非零点的块
""")
    p.slide("""
这几种情形在真实权重里都出现过，也是最容易写错的地方。
""", autobold=False)


def three_mismatches(p):
    p.title("三种常见的对不上")
    p.table(
        headers=["现象", "位置"],
        rows=[
            ["`f32_to_fp16` 不一致，其余都一致", "舍入的平局那一句漏了，或者次规格化数没补隐含位"],
            ["`q4_0_back` 一致而 `q4_0_bytes` 不一致", "半字节配对写成了 `j` 与 `j + 1`"],
            ["两个都不一致，误差明显偏大", "步长取了 `amax / 7`"],
        ],
        align=["left", "left"],
    )
    p.slide("""
第二行值得记住：还原回去的浮点全对，==写出去的字节却是错的==。
误差检查在这种错误面前完全没有反应。
""", autobold=False)


def whole_model(p):
    p.title("整个模型量化一遍的两组数字")
    p.table(
        headers=["方案", "产物张量数据", "位/权重", "量化耗时", "峰值内存"],
        rows=[
            ["全 Q4_0", "968 249 344 B", "4.502", "8 秒", "48 MB"],
            ["`q4_k_m` 配方", "1 101 457 408 B", "5.121", "88 秒", "48 MB"],
        ],
        align=["left", "right", "right", "right", "right"],
    )
    p.slide("""
再补上课后 C 部分的四个函数，`nano-quant` 就能把整个模型量化一遍。
源文件里参与量化的部分是 3 441 149 952 字节，全 Q4_0 之后压缩 3.55 倍。
""", autobold=False)
    p.aside("助教在 `Qwen/Qwen3-VL-2B-Instruct` 上跑出来的数，记录在 `nano-quant/tests/reference-qwen3vl.txt`。")


def half_bit(p):
    p.title("半位的差别在一条提示词上的样子")
    p.code("text", """prompt:  把这串数字逐位用中文写出来：523030910000

all Q4_0    5 2 3 0 3 0 9 1 0 0 0 0
q4_k_m      五二三零三零九一零零零零""")
    p.slide("""
两者都认出了这串数字并逐位输出。4.502 位/权重的那个没有照做「用中文写出来」，
5.121 位/权重的那个照做了。
""", autobold=False)
    p.aside("贪心解码，128 个词，纯 CPU，ollama 0.33.2。单条提示词上的对照不足以说明整体质量。")


def after_class(p):
    p.title("课后往下走的顺序")
    p.table(
        headers=["下一步", "函数", "说明在"],
        rows=[
            ["Q4_1：加一个偏移", "`q4_1_quantize`、`q4_1_dequantize`", "实验说明 B 部分"],
            ["Q4_K：超块与两级缩放", "`put_scale_min`、`q4_k_*`", "B 部分与框架 README"],
            ["整模型的量化与账目", "`tensor_selected`、`recipe_pick` 等四个", "实验说明 C 部分"],
            ["拼成 GGUF 并跑起来", "无新函数", "实验说明 D 部分"],
        ],
        align=["left", "left", "left"],
    )
    p.slide("""
Q4_1 与 Q4_0 只差一个偏移，本节的五步结构可以==整段套用==，
写完接着跑 `./nq-selftest a q4_0 q4_1`。
""", autobold=False)


def summary(p):
    p.title("本节要点")
    p.slide("""
- 实验对象是一个 4.25 GB 的文件：625 个张量，310 个属于语言模型，能压的字节全在 197 个矩阵里
- 框架按段读写，本节写的两个块函数每次只看 32 个 `float`
- 四个转换函数里 `f32_to_fp16` 最要紧，平局取偶那一句决定全班的 md5
- Q4_0 的五步：定 `amax`、定 `d`、写 `d`、换算码字、按 `j` 与 `j + 16` 打包
- 六行「与参考一致」是本节的验收，误差那一行不参与比对
""", reveal="items")
    p.highlight("写完这六个函数，读字节到写字节这条路就通了。", tone="green")
