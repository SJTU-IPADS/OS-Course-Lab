"""第二讲《数据的表示》的页面内容。

组织方式：以上一讲留下的模型权重文件为观察对象，把「1.9 GB 里的每个字节
表示什么」拆成四个问题——一个字节是什么、多字节对象如何排列、同一段位如何
得到不同的值、实数与低精度格式如何编码——逐节回答。每一节的结论都落回同一个
文件，最后一页把文件头逐字段读完。

用语要求：面向本科课程教学，采用陈述性的技术表述；不使用比喻、口语化措辞。

排版约定：
- 粗体后面不要紧跟全角冒号。`**词：**内容` 不符合 CommonMark 的闭合规则，
  会原样把 `**` 打到投影上；写成 `**词**：内容`。
- `==标记==` 只在 `p.slide(...)` 里展开，写进 highlight / 图注 / 表格会变成字面量。
- 一页最多一个 `p.highlight`，并且只给真正的结论用。
- 代码注释统一写英文，中英两版共用；第五部分照录 part-5.md，是例外。
"""

# ==============================================================================
# 共享代码清单
# ==============================================================================

HEX_GROUPS = """0x173A4C
十六进制      1    7    3    A    4    C
二进制      0001 0111 0011 1010 0100 1100"""

READ_FIELD = """static uint32_t le32(const unsigned char *b) {          /* little endian */
    return (uint32_t) b[0]       | (uint32_t) b[1] << 8 |
           (uint32_t) b[2] << 16 | (uint32_t) b[3] << 24;
}
static uint32_t be32(const unsigned char *b) {          /* big endian */
    return (uint32_t) b[3]       | (uint32_t) b[2] << 8 |
           (uint32_t) b[1] << 16 | (uint32_t) b[0] << 24;
}"""

SHOW_BYTES = """typedef unsigned char *byte_pointer;      /* a pointer to plain bytes */

void show_bytes(byte_pointer start, size_t len) {
    for (size_t i = 0; i < len; i++)
        printf(" %.2x", start[i]);            /* one byte at a time */
}"""

CAST_SAMPLE = """short          x  =  12345;                /* 0x3039 */
unsigned short ux = (unsigned short) x;    /* 0x3039, reads as 12345 */

short          y  = -12345;                /* 0xcfc7 */
unsigned short uy = (unsigned short) y;    /* 0xcfc7, reads as 53191 */"""

KERNEL_BUG = """void *memcpy(void *dest, const void *src, size_t n);   /* <string.h> */

#define KSIZE 1024
char kbuf[KSIZE];                       /* kernel memory the user may read */

int copy_from_kernel(void *user_dest, int maxlen) {
    int len = KSIZE < maxlen ? KSIZE : maxlen;   /* never more than the buffer */
    memcpy(user_dest, kbuf, len);                /* hand that many bytes over */
    return len;
}"""

MASK_SAMPLE = """x & 0x0f            /* keep the low four bits, clear the rest */
x | 0x80            /* set bit 7, leave the others alone  */
x & ~0x80           /* clear bit 7, leave the others alone */
x ^ y               /* the bits where x and y differ       */"""

NIBBLE_ACCESS = """static uint8_t lo4(uint8_t b) { return b & 0x0f; }        /* weight j      */
static uint8_t hi4(uint8_t b) { return b >> 4;   }        /* weight j + 16 */

static uint8_t pack(uint8_t lo, uint8_t hi) {
    return (uint8_t) (hi << 4 | (lo & 0x0f));             /* two into one byte */
}"""

NIBBLE_ADD = """static uint8_t add_naive(uint8_t a, uint8_t b) {
    return (uint8_t) (a + b);                          /* a carry crosses bit 3 */
}
static uint8_t add_packed(uint8_t a, uint8_t b) {
    uint8_t s = (uint8_t) ((a & 0x77) + (b & 0x77));   /* no carry leaves a lane */
    return (uint8_t) (s ^ ((a ^ b) & 0x88));           /* bit 3 added, carry dropped */
}"""

QUANT_CORE = """void quantize(const float *x, block_q4_0 *out) {
    float d  = extreme(x, 32) / -8.0f;         /* codes 0..15 mean -8..7 */
    float id = d ? 1.0f / d : 0.0f;
    out->d = f32_to_f16(d);                    /* the scale is stored as fp16 */
    for (int j = 0; j < 16; j++) {             /* weight j and weight j + 16 */
        int lo = clamp((long) (x[j]      * id + 8.5f));
        int hi = clamp((long) (x[j + 16] * id + 8.5f));
        out->qs[j] = (uint8_t) (hi << 4 | lo); /* two weights, one byte */
    }
}"""

K_SCALES = """static void get_scale_min(int j, const uint8_t *q, uint8_t *sc, uint8_t *m) {
    if (j < 4) {                    /* 0..3: one whole low six bits */
        *sc = q[j] & 63;
        *m  = q[j + 4] & 63;
    } else {                        /* 4..7: split across two bytes */
        *sc = (q[j + 4] & 0x0f) | (q[j - 4] >> 6) << 4;
        *m  = (q[j + 4] >> 4)   | (q[j]     >> 6) << 4;
    }
}"""


# ==============================================================================
# 回顾与本节的问题
# ==============================================================================

def course_info(p):
    p.gap(28)
    p.title("课程信息")
    p.slide("""
1. **课件下载**：https://github.com/SJTU-IPADS/OS-Course-Lab/tree/OS-Pre-Course-CSAPP
   便于实时更新、团队共建、师生共建
2. **课件使用**：问 AI 即可；欢迎丰富使用方式
3. **微信群**
""")
    p.slide("""
4. **课程安排**
   - 周六上课：从课程融合到实际课时压缩
   - 周中习题课改为讨论课
   - 从 A1 / A2 到 A0，再到 B / C：ICS 重回兵器谱前 XX，打造人气 TA
   - **臧斌宇老师**　ICS 课程在中国的创始人
     **伟大导师　·　伟大舵手　·　伟大奠基人　·　伟大引路人**
""").image_right("assets/ext/zang-binyu.jpg", width_px=160
    ).footnote("照片来自上海交通大学并行与分布式系统研究所（IPADS）成员页。")
    p.slide("""
5. **考核**：quiz + 期末 + lab，约 20 + 50 + 30；君子协定：对于 AI 的使用
""")


def ollama_intro(p):
    p.gap(30)
    p.title("回顾：用 Ollama 执行本地推理")
    p.slide("""
Ollama 是运行在本机的推理服务，提供 OpenAI 兼容的 HTTP 接口。
- 启动后监听 `http://localhost:11434`，`/v1/...` 路径与云端服务一致
- 模型权重以文件形式存放在本机磁盘上
- 请求、加载、计算、返回这条完整路径，都可以在自己的机器上观察
""").image_right("assets/ollama-logo.png", width_px=100)
    p.demo("启动推理服务", "ollama serve", timeout=0)
    p.demo("下载模型权重", "ollama pull llama3.2", timeout=0)
    p.demo("提交一次请求", "ollama run llama3.2 'How do u like CSAPP?'", timeout=0)


def recap_weights(p):
    p.gap(40)
    p.title("回顾：llama3.2 模型文件")
    p.demo("查看模型文件", """ls -lhS ~/.ollama/models/blobs | sed -n '2p'
file "$(ls -dS ~/.ollama/models/blobs/* | head -1)\"""",
           output="""-rw-r--r-- 1 ollama ollama 1.9G Sep  1 19:50 sha256-...
...sha256-...: data""")
    p.slide("""
我们以一次 `ollama run` 为例，自底向上考察了系统各层
- 模型权重是一个 1.9 GB 的文件，`file` 给出的结果是 `data`，即未能识别其类型
- 文件中不含任何可执行指令，全部内容是数据
- 操作系统按需把其中的页装入内存，但没有解释过任何一个字节的含义
""", reveal="items")
    p.highlight("这 1.9 GB 中的每一个字节，按什么规则解释？", tone="orange")
    p.notes("""
第一讲的落点是「模型只提供参数，执行过程由系统栈完成」。本节把「参数」这个词展开：
参数以什么格式写在文件里，读出来之后在内存中是什么，为什么是 1.9 GB 而不是 12.8 GB。
本讲的命令在 x86-64 的 Linux、arm64 的 macOS 与 Windows 的 WSL2 上都可以执行，
结果一致；查看汇编的两页随指令集变化，逐条差别见 README 的「跨平台」一节。
""")


def machine_model(p):
    p.title("CPU、内存与磁盘")
    p.slide("上一页那个 1.9 GB 文件在磁盘上。运行时按页装入内存，CPU 只按地址读写内存。")
    p.image("assets/cpu-memory-disk.svg", width_px=980,
            alt="CPU 与磁盘夹着内存：每个字节有地址，里面是 8 个 0/1")
    p.notes("""
接上一页：权重文件在磁盘上，操作系统按需把页装进内存，CPU 并不直接读磁盘。
图中每个格子是一位，一行 8 位就是一个带地址的字节。
强调两件事：每个字节有唯一地址；地址里存的不是「数」或「字」，而是 8 个 0/1。
后面讲位向量、十六进制、数据类型，都是在解释这些 0/1。
""")


def four_questions(p):
    p.gap(40)
    p.title("本节回答四个问题")
    p.slide("""
1. **什么是字节**：
   位与字节 · 二进制与十六进制 · C 数据类型的宽度 · 布尔值的存储
""")
    p.slide("""
2. **多字节组成数据对象**：
   内存按字节编址 · 字长与地址范围 · 字节序
""")
    p.slide("""
3. **整型与浮点编码**：
   无符号数与有符号数 · 强制转换与比较陷阱 · 位运算与移位
   IEEE 754 的规格化、非规格化与特殊值 · 舍入 · 低精度浮点格式
""")
    p.slide("""
4. **如何对浮点数进行量化**：
   Q4_0 的块结构与 4 位打包 · 量化粒度 · Q4_1 的偏移 · Q4_K 的两级缩放
""")


# ==============================================================================
# 第一部分 · 位与字节
# ==============================================================================

def hexdump(p):
    p.title("读一个模型文件的前 32 个字节")
    p.demo("按二进制查看", """cd examples
xxd -b -l 16 -c 8 tiny.gguf""",
           output="""00000000: 01000111 01000111 01010101 01000110 00000011 00000000 00000000 00000000  GGUF....
00000008: 00000001 00000000 00000000 00000000 00000000 00000000 00000000 00000000  ........""",
           description="每个字节写成 8 位二进制，一行 8 个字节")
    p.demo("按十六进制查看", """cd examples
xxd -l 32 tiny.gguf""",
           output="""00000000: 4747 5546 0300 0000 0100 0000 0000 0000  GGUF............
00000010: 0200 0000 0000 0000 1400 0000 0000 0000  ................""",
           description="每个字节写成 2 位十六进制")
    p.slide("""
`tiny.gguf` 按 GGUF 格式写出，只是规模很小，仅用作示例。
- 前四个字节 `47 47 55 46` 按 ASCII 解释为 `GGUF`，用于标识文件类型
- 其后的每一段字节表示什么，由格式规范逐字段规定
- 同一段字节脱离规范则没有确定的含义
""")
    p.notes("""
tiny.gguf 由 examples/make_gguf.py 生成，共 224 字节，含一个 8×4 的 fp16 张量与两条元数据。
课堂上可以让学生把这条命令换成自己机器上的真实权重文件，前 16 个字节的结构完全一致。
""")


def bits_to_value(p):
    p.title("位向量与它表示的数值")
    p.slide("""
$w$ 位的**位向量** $[x_{w-1},\\ldots,x_1,x_0]$ 按位权求和，得到一个非负整数：
""", autobold=False)
    p.slide(r"""
 $$
 B2U(\vec{x}) = \sum_{i=0}^{w-1} x_i \cdot 2^i
 $$
""", autobold=False)
    p.table(
        headers=["二进制", "按位权展开", "十进制", "十六进制"],
        rows=[
            ["`0000 1010`", "$2^3 + 2^1$", "10", "`0x0a`"],
            ["`0110 0110`", "$2^6+2^5+2^2+2^1$", "102", "`0x66`"],
            ["`1111 1111`", "$2^8-1$", "255", "`0xff`"],
        ],
        align=["left", "left", "right", "left"],
    )
    p.aside("反向转换按除二取余进行：102 = 51×2+0，51 = 25×2+1，依此类推。")


def hexadecimal(p):
    p.title("十六进制等多种进制")
    p.slide("""
十六进制以 ==4 位二进制为一组==，用 `0`–`9` 与 `a`–`f` 共 16 个符号书写。
一个字节有 8 位，对应十六进制有 2 位，取值 `0x00` 到 `0xff`。
""", autobold=False)
    p.code("text", HEX_GROUPS)
    p.table(
        headers=["进制", "C 的写法", "表示 42"],
        rows=[
            ["十进制", "无前缀", "`42`"],
            ["十六进制", "`0x` / `0X`", "`0x2a`"],
            ["八进制", "前导 `0`", "`052`"],
            ["二进制", "`0b` / `0B`", "`0b101010`"],
        ],
        align=["left", "left", "left"],
    )
    p.notes("""
八进制的前导零是一类真实的缺陷来源：文件权限写 `0644` 是对的，把某个十进制常量
误写成 `010` 就会变成 8。C2y 增加了 `0o` 前缀，gcc 15 已作为扩展支持，C23 及以前无效。
C23 还允许用 `'` 分组数字，如 `0b1111'1111`。输出侧只有 `%x` 与 `%o`，没有二进制的转换说明符。
""")



def other_formats(p):
    p.title("其他模型格式的标识")
    p.demo("看五个文件的前 16 个字节", """cd examples
for f in tiny.*; do printf '%-17s ' $f; xxd -l 16 -g 2 $f | cut -c11- | tr -s ' '; done""",
           output="""tiny.gguf         4747 5546 0300 0000 0100 0000 0000 0000 GGUF............
tiny.onnx         0808 120a 6c65 6374 7572 656b 6974 3a58 ....lecturekit:X
tiny.pkl          8002 7d71 0058 0600 0000 7765 6967 6874 ..}q.X....weight
tiny.safetensors  3e00 0000 0000 0000 7b22 7765 6967 6874 >.......{"weight
tiny.zip          504b 0304 1400 0000 0000 0000 2158 77a0 PK..........!Xw.""",
           files=["examples/make_formats.py"])
    p.slide("""
- **魔数**：GGUF 的 `47 47 55 46`、zip 的 `50 4b 03 04`；`.pt` 自 1.6 起就是 zip 归档
- **自描述头部**：safetensors 开头 8 字节是头长 `3e`（62），随后即 JSON：`7b 22` 是 `{\"`
- **无标识**：ONNX 是 protobuf，`08` 是字段 1 的标签；旧 `.pt` 是 pickle 流，`80 02` 是协议操作码
""", reveal="items")
    p.highlight("文件类型来自约定：魔数、自描述头部或扩展名。", tone="blue")
    p.notes("""
`file` 命令的判断依据就是魔数表，因此它认得 zip 而认不出 GGUF 与 safetensors。
安全上的差别值得一提：pickle 在反序列化时执行任意代码，safetensors 只读取一段 JSON 与
一片连续数据，不含可执行内容，这是它被提出的直接原因。
五个样例由 examples/make_formats.py 生成，都按各自格式的规则写出，不是手工拼的字节。
""")


def c_data_sizes(p):
    p.title("C 数据类型的大小")
    p.table(
        headers=["C 类型", "32 位平台", "64 位平台", "说明"],
        rows=[
            ["`char`", "1", "1", "一个字节，也用于表示字符"],
            ["`int`", "4", "4", "各平台一致"],
            ["`long`", "4", "8", "随平台变化"],
            ["`void *`", "4", "8", "宽度即字长"],
            ["`float` / `double`", "4 / 8", "4 / 8", "IEEE 754 单精度与双精度"],
            ["`size_t`", "4", "8", "无符号，用于表示长度与大小"],
        ],
        align=["left", "center", "center", "left"],
    )
    p.demo("在 PC 上核对", """cd examples
gcc -O1 -o sizes sizes.c && ./sizes""",
           output="""char 1  short 2  int 4  long 8  void* 8
float 4  double 8  size_t 8  int32_t 4  int64_t 8""",
           files=["examples/sizes.c"])
    p.notes("""
需要固定宽度时使用 `<stdint.h>` 中的 int32_t / uint64_t，它们在各平台上宽度一致。
课堂上值得强调的是 long 与指针随平台变化：把指针存进 int 的代码在 64 位平台上会丢掉高位。
64 位的 Linux 与 macOS 采用 LP64，`long` 为 8 字节；原生 Windows 采用 LLP64，
`long` 为 4 字节而指针为 8 字节，此时表中「64 位」一列的 `long` 应读作 4。
WSL2 中的程序是 Linux 程序，与表中的 64 位一列一致。
""")


def bool_storage(p):
    p.gap(50)
    p.title("拓展：布尔值占用的空间")
    p.slide("""
C 语言没有内置的布尔类型，真假由整数的零与非零表示；C99 引入 `_Bool`，
`<stdbool.h>` 把 `bool` 定义为它的别名。C++ 中 `bool` 是一个内置类型。
- 两种语言中 `sizeof(bool)` 都是 ==1 个字节==：字节是最小的可寻址单位
- 一个布尔值只需要 1 位信息，其余 7 位不参与取值
- 赋值时任何非零值都被转换为 1，`bool b = 17;` 在内存中留下的字节是 `0x01`
""", reveal="items")
    p.demo("同一份代码分别按 C 与 C++ 编译", """cd examples
gcc -O1 -o bool_c bool_size.c && g++ -O1 -x c++ -o bool_cc bool_size.c
./bool_c && ./bool_cc""",
           output="""C    sizeof(bool) 1  value 1  byte 01
C++  sizeof(bool) 1  value 1  byte 01""",
           files=["examples/bool_size.c"])
    p.aside("如需节省空间，通过按位存放大量布尔值时，需要程序自己打包：C 用位域或整数掩码，C++ 可通过 `std::vector<bool>` 按位存储。")
    p.notes("""
C 里按位存放的写法是位域：`struct { unsigned a : 1, b : 1; }`，位的排列次序由实现决定，
因此位域不适合用来描述文件格式，跨编译器读同一段字节会得到不同结果。
""")


# ==============================================================================
# 第二部分 · 字节序
# ==============================================================================



def memory_as_bytes(p):
    p.title("内存模型：以字节为单位编址的数组")
    p.slide("""
程序看到的内存是一个==以字节为单位编址的数组==：每个地址对应一个字节。
- 地址从 0 开始连续编号，全体地址构成**地址空间**
- 一个多字节对象占用一段连续地址，其地址取这段地址中的**最小值**
""", reveal="items")
    p.image("assets/memory-bytes.svg", width_px=900)
    p.aside("字节是最小的可寻址单位，因此比字节更小的数据必须打包进一个字节。")


def word_size(p):
    p.title("字长：指针的宽度与地址空间的上限")
    p.slide(r"""
**字长**指一个指针占用的位数。$n$ 位的指针能表示 $2^n$ 个不同的地址，
每个地址对应一个字节，地址空间的大小因此就是 $2^n$ 字节。字长由硬件与操作系统共同确定，
是一个不能由程序选择的系统参数。
""", autobold=False)
    p.table(
        headers=["字长", "地址范围", "地址个数", "地址空间大小"],
        rows=[
            ["32 位", "`0x00000000` – `0xffffffff`",
             "$2^{32}$", "$4\\,294\\,967\\,296$ 字节 = 4 GB"],
            ["64 位", "`0x0000000000000000` – `0xffffffffffffffff`",
             "$2^{64}$", "约 $1.8 \\times 10^{19}$ 字节 = 16 EB"],
        ],
        align=["center", "left", "center", "left"],
    )
    p.image("assets/address-space.svg", width_px=700)
    p.notes("""
上一讲的 mmap 把整个权重文件映射进地址空间。这一步在 32 位系统上做不到，
32 位系统上可用的地址数量不足，与物理内存的容量无关——这是「地址」与「内存」的区别。
""")


def endianness(p):
    p.title("字节序：多字节对象的排列规则")
    p.slide("""
一个 4 字节的整数占用四个连续地址。哪个字节放在最低地址，由==字节序==规定。
- **小端**：最低有效字节位于最低地址。x86-64 与 ARM 的默认配置
- **大端**：最低有效字节位于最高地址。TCP/IP 首部与部分平台采用
- 采用哪一种由==处理器==确定：访存指令按哪个次序把内存中的字节组装成寄存器中的值，是硬件的设计选择，程序无法更改
""")
    p.image("assets/byte-order.svg", width_px=820,
            caption="同一个值在两种排列下占用同样的四个地址，字节的次序相反")
    p.notes("""
字节内部的位序不由程序观察，因此不涉及这一问题；讨论的对象始终是字节的次序。
x86-64 只有小端一种。ARM、RISC-V、PowerPC 是双端序处理器，两种次序都能执行，
实际使用哪一种在启动时由控制寄存器固定，Linux 在这些平台上通常运行于小端
文件格式与网络协议是另一类约定：它们自行规定字节序，与运行它们的处理器无关。
""")


def read_the_field(p):
    p.title("同一段字节的两种读法")
    p.code("c", READ_FIELD)
    p.demo("按两种字节序读同一个字段", """cd examples
gcc -O1 -o gguf_head gguf_head.c && ./gguf_head""",
           output="""magic          GGUF
version   LE   3
version   BE   50331648""",
           files=["examples/gguf_head.c"])
    p.highlight("字节序是一项约定：读写双方采用同一约定，字节才有确定的值。", tone="orange")


def show_bytes_page(p):
    p.title("观察任意对象的字节表示")
    p.code("c", SHOW_BYTES)
    p.demo("逐字节打印", """cd examples
gcc -O1 -o show_bytes show_bytes.c && ./show_bytes""",
           output="""int 12345      39 30 00 00
float 12345    00 e4 40 46
&ival          e8 49 1e 07 fc 7f 00 00""",
           files=["examples/show_bytes.c"])
    p.slide("""
- `12345` 的四个字节为 `39 30 00 00`，最低有效字节在最低地址，即小端
- 同一数值的 `float` 表示为 `00 e4 40 46`，与整数的位模式完全不同
- 指针本身也是一个 8 字节的对象，同样可以逐字节观察
""")
    p.notes("""
把地址转换为 `unsigned char *` 是绕过类型系统的标准做法：机器级程序本来就把对象
看作一段已知长度的字节，`show_bytes` 只是把这一视角显式写出来。
""")


def endianness_visible(p):
    p.gap(80)
    p.title("字节序在什么场合可见")
    p.slide("""
- **文件格式**：GGUF、ELF、PNG 各自规定字节序，跨机器读取才有确定结果
- **网络传输**：TCP/IP 首部采用大端，`htons` / `htonl` 完成本机序与网络序的转换
- **强制类型转换**：以另一种类型解释同一段字节时，排列方式随之暴露
- **反汇编输出**：指令编码中的立即数与地址按小端存放
""", reveal="items")
    p.aside("单字节序列不受影响：ASCII 字符串与 UTF-8 文本在任何平台上的字节次序都一致。")


def text_and_tokens(p):
    p.title("字符编码与 token 编号")
    p.slide("""
文本进入程序之前需要一次编码，这一步把符号映射为整数序列。
- **字符编码**：ASCII 用 7 位表示一个字符，UTF-8 用 1 到 4 个字节表示一个码点
- **分词器（tokenizer）**：一段文本被切成若干 token，每个 token 对应词表中的一个整数编号
- 两者的形式一致：先约定一张映射表，再以整数序列在系统中传递
""", reveal="items")
    p.demo("一段中文的 UTF-8 编码", "printf '你好' | xxd",
           output="00000000: e4bd a0e5 a5bd                           ......")
    p.highlight("模型接收的是 token 编号序列，字符在进入模型之前已经完成编码。", tone="blue")
    p.notes("""
两级编码在课堂上值得点明：字符 → 字节由 UTF-8 规定，文本 → token 由分词器的词表规定。
两者都是约定，都可以查表验证，都不涉及模型本身。
""")


# ==============================================================================
# 第三部分 · 整数
# ==============================================================================

def two_readings(p):
    p.title("两种整型数的表示")
    p.slide(r"""
无符号数各位的权重都为正；有符号数把最高位的权重由 $+2^{w-1}$ 改为 $-2^{w-1}$，其余各位的权重不变。
""", autobold=False)
    p.slide(r"""
 $$
 B2U(\vec{x}) = \sum_{i=0}^{w-1} x_i 2^i
 \qquad\qquad
 B2T(\vec{x}) = -x_{w-1} 2^{w-1} + \sum_{i=0}^{w-2} x_i 2^i
 $$
""", autobold=False)
    p.table(
        headers=["位模式（$w=4$）", "无符号 $B2U$", "有符号 $B2T$", "两者之差"],
        rows=[
            ["`0111`", "$4+2+1 = 7$", "$4+2+1 = 7$", "0"],
            ["`1000`", "$8 = 8$", "$-8 = -8$", "16"],
            ["`1011`", "$8+2+1 = 11$", "$-8+2+1 = -5$", "16"],
            ["`1111`", "$8+4+2+1 = 15$", "$-8+4+2+1 = -1$", "16"],
        ],
        align=["left", "left", "left", "center"],
    )
    p.aside("最高位为 0 时两种解释一致，为 1 时相差 $2^w$。")
    p.notes("""
数字电子技术里有符号数的定义是「正数不变，负数取反加一」，用于让减法由加法电路完成。
这里给出的按位权求和是同一套编码的另一种描述，在读一段字节时更好用：
不需要先判断符号再决定怎么算，直接按权重相加即可。
两者等价可以当场验证：`1011` 取反得 `0100`，加一得 `0101` 即 5，所以原码表示 −5。
""")


def numeric_range(p):
    p.title("无符号数与有符号数的取值范围")
    p.table(
        headers=["", "最小值", "最大值", "$w=32$", "$w=64$"],
        rows=[
            ["无符号", "0", "$2^w-1$", "4294967295", "约 $1.8\\times10^{19}$"],
            ["有符号", "$-2^{w-1}$", "$2^{w-1}-1$", "2147483647", "约 $9.2\\times10^{18}$"],
        ],
        align=["left", "center", "center", "right", "right"],
    )
    p.slide("""
两条需要记住的关系：
- $|TMin| = TMax + 1$，负数比正数多一个
- `-1` 有符号数的位模式为全 1，与无符号的最大值位模式相同
""")


def casting(p):
    p.title("类型转换：位模式不变，解释改变")
    p.code("c", CAST_SAMPLE)
    p.slide("""
C 允许把一个类型的变量按另一个类型解释，转换有两种发生方式。
- **显式转换**：写出 `(unsigned short) x`
- **隐式转换**：赋值、函数传参、以及表达式中的类型提升都会发生
- 有符号与无符号之间的转换==不改变任何一位==，改变的只是解释规则
""")
    p.image("assets/same-bits.svg", width_px=690)


def comparison_trap(p):
    p.title("混合比较：有符号操作数被转换为无符号")
    p.slide("""
一个表达式中同时出现有符号与无符号操作数时，==有符号一侧被转换为无符号==，比较随之改变。
""", autobold=False)
    p.demo("核对几个比较的结果", """cd examples
gcc -O1 -o compare compare.c && ./compare""",
           output="""-1 < 0                 1
-1 < 0u                0
2147483647 > -2147483647-1     1
2147483647u > -2147483647-1    0
length - 1, as unsigned        4294967295
strlonger("ab", "abcd")        1""",
           files=["examples/compare.c"])
    p.aside("规则写在 C 标准里，编译器默认不为此报警，如需要检查可在编译时加上 `-Wsign-compare` 参数。")


def kernel_bug(p):
    p.title("找错误：内核向用户程序拷贝数据")
    p.slide("""
内核代用户程序拷贝数据：`copy_from_kernel` 把 `kbuf` 拷到 `user_dest`，最多 `maxlen` 字节。
""", autobold=False)
    p.code("c", KERNEL_BUG)
    p.aside("问题：调用方能否让它拷贝出 `kbuf` 这 1024 字节以外的内容？")


def kernel_bug_answer(p):
    p.title("找错误：内核向用户程序拷贝数据")
    p.slide("""
内核代用户程序拷贝数据：`copy_from_kernel` 把 `kbuf` 拷到 `user_dest`，最多 `maxlen` 字节。
""", autobold=False)
    p.code("c", KERNEL_BUG)
    p.slide("""
调用方传入负的 `maxlen` 时：
- `KSIZE < maxlen` 是有符号比较，结果为假，`len` 取到那个负值
- `memcpy` 的第三个参数是 `size_t`，`len` 转换成极大的无符号数，越过缓冲区边界
""")
    p.cite(title="Computer Systems: A Programmer's Perspective", author="Bryant, O'Hallaron",
           year="2015", venue="3rd edition, §2.2.6", key="csapp")
    p.notes("""
这段代码源自 FreeBSD 的 getpeername 实现中出现过的缺陷。`maxlen` 由调用方给出，
返回值是实际拷贝的字节数。课堂上可以指出：这类错误在编译时没有任何提示，
运行时也不一定立刻崩溃，只有在特定输入下才暴露。
审查生成的代码时，有符号与无符号的混用是需要优先检查的一类。
""")


def mixed_width_comparison(p):
    p.title("混合比较：宽度不同的操作数进行比较")
    p.slide("""
1. **确定目标类型**：先把窄于 `int` 的类型提升为 `int`；提升后宽度不同取较宽者，宽度相同、符号不同取无符号
2. ==补 0 还是补符号位由原类型决定==，得到的位模式按目标类型解释
""")
    p.table(
        headers=["比较", "目标类型", "被转换的操作数", "结果"],
        rows=[
            ["int −1 < unsigned short 1", "int", "1 补 0 得 `0x00000001`，值为 1", "1"],
            ["short −1 < unsigned short 1", "int", "两侧都提升为 `int`：−1 补符号位，1 补 0", "1"],
            ["short −1 < unsigned 1", "unsigned", "−1 补符号位得 `0xffffffff`，值为 $2^{32}-1$", "0"],
            ["int −1 < unsigned long long 1", "unsigned long long", "−1 补符号位得 64 个 1，值为 $2^{64}-1$", "0"],
            ["long long −1 < unsigned 1", "long long", "1 补 0 得 64 位的 1，值为 1", "1"],
        ],
        align=["left", "left", "left", "center"],
    )
    p.notes("""
决定补 0 还是补符号位的是操作数原来的类型，决定比较按有符号还是无符号进行的是目标类型，两件事分开。
最后一行成立的前提是 `long long` 能表示 `unsigned` 的全部取值，在三种平台上都满足；
若较宽的有符号类型不能表示较窄无符号类型的全部取值，两侧转为该有符号类型对应的无符号类型。
""")


def bit_operations(p):
    p.title("位运算")
    p.table(
        headers=["运算", "C 写法", "作用", "作为集合运算"],
        rows=[
            ["与", "`x & y`", "两位都为 1 时得 1", "交集"],
            ["或", "`x | y`", "任一位为 1 时得 1", "并集"],
            ["取反", "`~x`", "逐位取反", "补集"],
            ["异或", "`x ^ y`", "两位不同时得 1", "对称差"],
        ],
        align=["center", "left", "left", "left"],
    )
    p.slide("""
低 4 位从高到低：ICS、编译、操作系统、计算机图形学。甲 `1101`，乙 `0110`。
- 交集 `1101 & 0110 = 0100`：都选了编译
- 并集 `1101 | 0110 = 1111`：两人合起来四门都有
- 补集（只看这 4 位）`~1101 = 0010`：甲没选操作系统
- 对称差 `1101 ^ 0110 = 1011`：只一人选的课
""")


def bit_operations_masks(p):
    p.title("位运算：掩码计算")
    p.code("c", MASK_SAMPLE)
    p.slide("""
两个 IP 各自与掩码相与，结果相同则在同一局域网。
掩码 `255.255.255.0` 留下前 24 位，清掉后 8 位，与 `x & 0xffffff00` 相同。
""")
    p.table(
        headers=["IP", "`IP & 255.255.255.0`", "判定"],
        rows=[
            ["192.168.1.100", "192.168.1.0", "同一局域网"],
            ["192.168.1.200", "192.168.1.0", "同一局域网"],
            ["192.168.2.10", "192.168.2.0", "不同网段"],
        ],
        align=["left", "left", "left"],
    )
    p.aside("位运算与逻辑运算 `&&` `||` `!` 不同：后者把非零值视为真，结果只有 0 或 1，并且会短路求值。")


def bit_operations_readonly(p):
    p.title("位运算：文件权限")
    p.slide("""
权限是 9 个比特，每 3 位一组：所有者、所属组、其他人。一组之内读 = 4、写 = 2、执行 = 1，`r--` = 4。
新建文件默认是 `-rw-r--r--`。`chmod 444` 后变成 `-r--r--r--`，写位为 0，再写入会被内核拒绝。
""")
    p.demo("查看权限并写入", """rm -f /tmp/ics-readonly.txt
printf 'hello\\n' > /tmp/ics-readonly.txt
echo before:
ls -lh /tmp/ics-readonly.txt
chmod 444 /tmp/ics-readonly.txt
echo after:
ls -lh /tmp/ics-readonly.txt
printf 'more\\n' >> /tmp/ics-readonly.txt""",
           output="""before:
-rw-r--r--  1 user  staff     6B Sep 21 21:43 /tmp/ics-readonly.txt
after:
-r--r--r--  1 user  staff     6B Sep 21 21:43 /tmp/ics-readonly.txt
sh: /tmp/ics-readonly.txt: Permission denied""")


def shifts(p):
    p.title("移位运算")
    p.slide("""
- **左移** `x << k`：各位左移 $k$ 位，右侧补 0，高位移出后丢弃
- **逻辑右移** `x >> k`：左侧补 0，用于无符号数
- **算术右移** `x >> k`：左侧补符号位，用于有符号数，使负数右移后仍为负数
- 移位位数达到或超过字长时，C 标准未定义其行为
- 不溢出时，`x << k` 等于 $x \\times 2^k$，`x >> k` 等于 $\\lfloor x / 2^k \\rfloor$；移位比乘除法快，能用移位表示时通常使用移位
""", reveal="items")
    p.image("assets/shifts.svg", width_px=900)
    p.notes("""
右移对应向下取整，C 的整数除法向零取整，两者在负数上结果不同：`-7 >> 1` 为 −4，`-7 / 2` 为 −3。
编译器把 `x / 2^k` 换成移位时，会先给负数加上 $2^k - 1$ 的偏置，使结果与除法一致。
""")


def operator_precedence(p):
    p.title("位运算符的优先级")
    p.slide("""
位运算符 `&`、`^`、`|` 的优先级==低于==比较运算符，移位运算符 `<<`、`>>` 的优先级
低于加减。这两处与直觉相反，是一类常见的书写错误。
""", autobold=False)
    p.table(
        headers=["优先级", "运算符", "结合性", "容易写错的表达式"],
        rows=[
            ["高", "`~` `!` `-`（一元）", "右", "`~x + 1` 是 `(~x) + 1`"],
            ["", "`*` `/` `%`", "左", ""],
            ["", "`+` `-`", "左", "`x << 1 + 2` 是 `x << 3`"],
            ["", "`<<` `>>`", "左", ""],
            ["", "`<` `<=` `>` `>=`", "左", ""],
            ["", "`==` `!=`", "左", "`x & 1 == 0` 是 `x & (1 == 0)`"],
            ["", "`&`，其后 `^`，其后 `|`", "左", "`x & 3 | 4` 是 `(x & 3) | 4`"],
            ["低", "`&&`，其后 `||`", "左", ""],
        ],
        align=["center", "left", "center", "left"],
    )
    p.notes("""
表中最后三行的写法在实际代码里都出现过。
优先级的次序有历史原因：`&` 与 `|` 早于 `&&` 与 `||` 出现，当时它们同时承担
按位与逻辑两种用途，优先级按逻辑运算的位置设定；`&&` 与 `||` 加入之后，
为了不破坏既有代码，`&` 与 `|` 的位置保留至今。Ritchie 本人在回顾 C 的发展时
把这一点列为遗留的缺陷。记忆规则不如括号可靠。
""")


def precedence_in_practice(p):
    p.title("位运算符的优先级")
    p.slide("设 `x = 6`。这几个式子分别等于多少？")
    p.slide("""
- `x & 1 == 0`
- `x << 1 + 2`
- `x & 3 | 4`
""", reveal="items")
    p.demo("编译并运行 precedence.c", """cd examples
gcc -O1 -Wall -o precedence precedence.c 2>&1 | grep -o 'warning.*'
./precedence""",
           files=["examples/precedence.c"])
    p.highlight("`-Wall` 能报出这三处错误，但建议含位运算的表达式一律加括号。", tone="orange")
    p.notes("""
`x` 取 6。三处警告与三处错误一一对应，因此这类问题只要打开 `-Wall` 就不会漏掉；
但警告只覆盖它认得的组合，加括号是更可靠的做法。
""")


# ==============================================================================
# 第四部分 · 浮点数的编码
# ==============================================================================

FRAC_BITS = """\
unsigned result_bits = 0, current_bit = 0x80000000;
for (i = 0; i < 32; i++) {
    x *= 2;
    if (x >= 1) {
        result_bits |= current_bit;
        if (x == 1)
            break;          /* the fraction is exact */
        x -= 1;
    }
    current_bit >>= 1;
}"""


def fractional_binary(p):
    p.title("从十进制小数到二进制小数")
    p.slide(r"""
十进制小数 $d_m \cdots d_1 d_0 . d_{-1} \cdots d_{-n}$ 的第 $i$ 位权重为 $10^{i}$。
科学计数法把它写成 $d.dd\ldots \times 10^{k}$，例如 $12.375 = 1.2375 \times 10^{1}$。
二进制小数按同样的规则书写，第 $i$ 位权重为 $2^{i}$：$12.375 = 1100.011_2 = 1.100011_2 \times 2^{3}$。
""", autobold=False)
    p.table(
        headers=["$b_m$", "$b_{m-1}$", "$\\cdots$", "$b_2$", "$b_1$", "$b_0$", ".",
                 "$b_{-1}$", "$b_{-2}$", "$b_{-3}$", "$\\cdots$", "$b_{-n}$"],
        rows=[["$2^m$", "$2^{m-1}$", "$\\cdots$", "4", "2", "1", ".",
               "1/2", "1/4", "1/8", "$\\cdots$", "$2^{-n}$"]],
        align=["center"] * 12,
    )
    p.slide(r"""
- 二进制小数点右侧的位表示 2 的负次幂
- 表示的有理数为 $\sum_{i=-n}^{m} b_i \times 2^{i}$
""")
    p.slide("""
只准留下 4 个有效数字（计算器屏幕就是这样）：
- 固定写下每一位：`375000000` 要 9 格，`0.000000375` 要 10 格，都写不下
- 科学计数法：都写成 `3.75`，再记指数 8 或 −7，屏幕上是 `3.75E8`、`3.75E-7`
""")
    p.notes(r"""
表头是各位，下一行是该位的权重。小数点左侧每向左一位权重乘 2，右侧每向右一位权重除以 2。
$12.375$ 的整数部分 $12 = 8 + 4$，小数部分 $0.375 = \frac{1}{4} + \frac{1}{8}$，合起来是 $1100.011_2$。
科学计数法中小数点移到最高位之后：十进制移动 1 位得到 $10^{1}$，二进制移动 3 位得到 $2^{3}$。
课上先问：纸上只准写 4 个数字，375000000 和 0.000000375 怎么写？再指计算器上的 E。
""")


def frac_examples(p):
    p.title("乘二取整法：转换到二进制小数")
    p.slide(r"""
小数部分按==乘二取整==转换：反复乘以 2，依次取出整数部分作为小数点后的各位。以 0.2 为例：
""", autobold=False)
    p.table(
        headers=["乘以 2", "取出的位", "剩余的小数部分"],
        rows=[
            ["$0.2 \\times 2 = 0.4$", "0", "0.4"],
            ["$0.4 \\times 2 = 0.8$", "0", "0.8"],
            ["$0.8 \\times 2 = 1.6$", "1", "0.6"],
            ["$0.6 \\times 2 = 1.2$", "1", "0.2"],
        ],
        align=["left", "center", "left"],
    )
    p.slide(r"""
- 剩余部分回到 0.2，此后循环：$0.2 = 0.00110011[0011]\ldots_2$
- 二进制小数只能精确表示 $x/2^{k}$ 形式的数，其余的数的位模式无限循环
""")
    p.notes(r"""
$0.2 = 1/5$，分母含因子 5，方括号内的 0011 无限重复。
可以让学生当场算 $0.375$：$0.75$ 取 0，$1.5$ 取 1 剩 $0.5$，$1.0$ 取 1 剩 0 结束，得 $0.011_2$。
""")


def frac_to_bits(p):
    p.title("小数转换为二进制位")
    p.code("c", FRAC_BITS)
    p.demo("转换三个小数", """cd examples
gcc -O1 -o frac_bits frac_bits.c && ./frac_bits""",
           output="""0.75   0.11
0.625  0.101
0.2    0.00110011001100110011001100110011 ...""",
           files=["examples/frac_bits.c"])
    p.notes(r"""
这段循环就是上一页的乘二取整。
每次乘 2 后整数部分为 1，就在当前位写 1 并减去 1；`current_bit` 每轮右移一位，指向下一位。
乘 2 后恰好等于 1 时小数已经取尽，提前结束；0.2 在 32 轮内始终取不尽。
""")


def ieee_history(p):
    p.title("IEEE 754 标准的由来")
    p.slide("""
- 20 世纪 80 年代以前
  - 浮点格式各不相同：速度快，实现简单，精度较低
- IEEE 754
  - 由 W. Kahan 为 Intel 处理器设计，Kahan 于 1989 年获得图灵奖
  - 简洁，易于理解
""").image_right("assets/ext/kahan.jpg", width_px=155
    ).footnote("William Kahan 是 IEEE 754 的主要设计者，1989 年图灵奖得主。照片来自 Wikimedia Commons，摄影 George Bergman，CC BY-SA 4.0。")
    p.notes("""
标准出现以前，不同厂商的机器上同一段数值程序可能得到不同的结果。
IEEE 754 规定了格式、舍入与特殊值，此后主流处理器都遵循它。
""")


def ieee_form(p):
    p.title("IEEE 浮点表示的数值形式与编码")
    p.image("assets/bit-fields.svg", width_px=690)
    p.slide(r"""
数值形式为 $V = (-1)^{s} \times M \times 2^{E}$，编码分为 $s$、$exp$、$frac$ 三个字段：
- 符号位 $s$ 决定数的正负
- $exp$ 字段编码阶码 $E$，$E$ 以 2 的幂对数值加权
- $frac$ 字段编码尾数 $M$，$M$ 通常是区间 $[1.0, 2.0)$ 内的小数
""")
    p.table(
        headers=["精度", "总位数", "exp 位数", "frac 位数"],
        rows=[
            ["单精度", "32", "8", "23"],
            ["双精度", "64", "11", "52"],
        ],
        align=["center", "center", "center", "center"],
    )
    p.notes(r"""
图中是 3.1415927 的 FP32 编码：符号位 0，阶码字段 128，尾数 $M = 1.5708$，$E = 1$。
这与二进制科学计数法 $1.bb\ldots \times 2^{E}$ 的写法一致：符号单独拿出来，$M$ 限定在 $[1.0, 2.0)$ 内，
每个非零数因此只有一种写法。三个字段从高位到低位依次排列，符号位是最高位。
C 语言中 `float` 对应单精度，`double` 对应双精度。两个字段的编码规则在后面几页讲。
""")


def normalized_exp(p):
    p.title("规格化值：阶码")
    p.slide(r"""
- 条件：$exp \neq 000\ldots0$ 且 $exp \neq 111\ldots1$
- 阶码按偏置值编码：$E = Exp - Bias$
  - $Exp$：$exp$ 字段表示的无符号数
  - $Bias$：偏置值，一般为 $2^{m-1}-1$，$m$ 为 $exp$ 的位数
- 偏置之后，阶码字段越大，指数越大，两个正数可以按无符号整数比较 `exp`
""")
    p.table(
        headers=["精度", "$Bias$", "$Exp$ 的范围", "$E$ 的范围"],
        rows=[
            ["单精度", "127", "1 … 254", "−126 … 127"],
            ["双精度", "1023", "1 … 2046", "−1022 … 1023"],
        ],
        align=["center", "center", "center", "center"],
    )
    p.notes(r"""
$exp$ 全 0 与全 1 两种取值留给非规格化值和特殊值，规格化值只用中间的部分。
$Exp$ 是无符号数，减去偏置后 $E$ 覆盖正负两个方向，比较两个数的阶码时可以直接按无符号数比较。
""")


def normalized_frac(p):
    p.title("规格化值：尾数")
    p.slide(r"""
- 尾数带有隐含的前导 1：$M = 1.xxx\ldots x_2$
  - $xxx\ldots x$：$frac$ 字段的各位
  - $frac = 000\ldots0$ 时取最小值，$M = 1.0$
  - $frac = 111\ldots1$ 时取最大值，$M = 2.0 - \varepsilon$
  - 前导的 1 不占位，相当于多得到一位
""")
    p.notes(r"""
规格化的数最高位总是 1，存下来不携带信息，因此格式规定它隐含存在。
单精度的 23 位 $frac$ 因此表示的是 24 位有效数字中的后 23 位。
""")


def normalized_example(p):
    p.title("规格化编码的例子")
    p.demo("把 12345 编码为单精度浮点数", """cd examples
gcc -O1 -o normalized normalized.c && ./normalized""",
           output="""value      12345 (hex 0x3039)
binary     11000000111001
normalized 1.1000000111001 x 2^13
frac       10000001110010000000000
exp        10001100 (140 = 127 + 13)
encoding   0100 0110 0100 0000 1110 0100 0000 0000
hex        4640E400""",
           files=["examples/normalized.c"])
    p.slide(r"""
- 小数点左移 13 位，$E = 13$，$Exp = 13 + 127 = 140$
- 去掉前导 1，小数点后的 13 位补 0 到 23 位，得到 $frac$
- 依次拼接 $s = 0$、$exp$、$frac$，得到 `0x4640E400`
""")
    p.notes("""
程序用 `memcpy` 把 float 的 4 个字节拷进 unsigned，再按位取出三个字段，输出与手算的每一步对应。
""")


def denormalized(p):
    p.title("非规格化值")
    p.slide(r"""
- 条件：$exp = 000\ldots0$
- 阶码 $E = 1 - Bias$，尾数 $M = 0.xxx\ldots x_2$，$xxx\ldots x$ 为 $frac$ 字段的各位
- $frac = 000\ldots0$：表示 0，$+0$ 与 $-0$ 是两个不同的值
- $frac \neq 000\ldots0$：非常接近 0.0 的数
""")
    p.demo("解码三个阶码字段全 0 的单精度位模式", """cd examples
gcc -O1 -o denormalized denormalized.c && ./denormalized""",
           output="""bits       s  exp      frac                     M          E     as float
80000000   1  00000000 00000000000000000000000  0          -126  -0
00400000   0  00000000 10000000000000000000000  0.5        -126  5.87747e-39
00000001   0  00000000 00000000000000000000001  1.192e-07  -126  1.4013e-45""",
           files=["examples/denormalized.c"])
    p.notes(r"""
$E$ 取 $1 - Bias$ 而非 $0 - Bias$，使最大的非规格化数与最小的规格化数相邻，中间不出现间隙。
单精度中非规格化数的 $E = -126$，$M < 1$。
demo 的三行：符号位为 1、其余全 0 是 $-0$；$frac$ 最高位为 1 时 $M = 0.5$，值为 $2^{-127}$；
$frac$ 只有最低位为 1 时 $M = 2^{-23}$，值为 $2^{-149}$，是最小的正数。
""")


def special_values(p):
    p.title("特殊值")
    p.slide(r"""
- 条件：$exp = 111\ldots1$
- $frac = 000\ldots0$：表示无穷 $\infty$
  - 运算溢出时得到
  - 分为 $+\infty$ 与 $-\infty$
  - 例：$1.0/0.0 = -1.0/{-0.0} = +\infty$，$1.0/{-0.0} = -\infty$
- $frac \neq 000\ldots0$：非数（NaN，Not-a-Number）
  - 表示无法确定数值的运算结果
  - 例：$\sqrt{-1}$，$\infty - \infty$，$\infty \times 0$
- NaN 与任何值的 `<`、`<=`、`>`、`>=`、`==` 均为假，`!=` 为真，`x != x` 因此可用于判断 NaN
""")
    p.notes(r"""
单精度中 $+\infty$ 的位模式是 `0x7F800000`，$-\infty$ 是 `0xFF800000`。
NaN 的 $frac$ 可以取任意非零值，因此 NaN 对应的位模式有 $2 \times (2^{23} - 1)$ 个。
$1.0/{-0.0} = -\infty$ 说明 $+0$ 与 $-0$ 按浮点比较相等，参与除法时结果的符号不同。
""")


def float_encoding(p):
    p.title("浮点数编码小结")
    p.spacer(60)
    p.table(
        headers=["阶码字段", "名称", "$E$", "$M$", "覆盖的数"],
        rows=[
            ["非全 0 且非全 1", "规格化", "$e-127$", "$1.f$", "绝大多数数值"],
            ["全 0", "非规格化", "$-126$", "$0.f$", "零，以及接近零的极小数"],
            ["全 1，$f=0$", "无穷", "—", "—", "$+\\infty$ 与 $-\\infty$"],
            ["全 1，$f \\neq 0$", "NaN", "—", "—", "$0/0$ 一类无定义的结果"],
        ],
        align=["center", "center", "center", "center", "left"],
    )
    p.notes("""
全 0 与全 1 被保留，8 位阶码的实际指数范围是 −126 到 127。
非规格化数取消前导的 1，使零附近的可表示值保持均匀间隔，下溢因此逐步发生。
非规格化数的 $E$ 取 $-126$ 而非 $-127$，是为了让最大的非规格化数与最小的
规格化数首尾相接，中间不出现间隙。低精度格式沿用同一套规则：FP8 E4M3 的
可表示值里，非规格化数占了相当一部分。
""")


def tiny_float(p):
    p.title("小练习：8 位浮点编码")
    p.slide("自制 8 位浮点，规则与 IEEE 754 相同：")
    p.table(
        headers=["字段", "位数", "含义"],
        rows=[
            ["$s$", "1", "符号"],
            ["$exp$", "4", r"$Bias = 2^{4-1}-1 = 7$"],
            ["$frac$", "3", "规格化时隐含前导 1"],
        ],
        align=["center", "center", "left"],
    )
    p.slide("""
这几个怎么换？
- 位模式 `0 0110 100` 是哪个十进制数？
- 十进制 `2.5` 编成哪 8 位？
- 二进制小数 $0.011_2$ 编成哪 8 位？
""", reveal="items")
    p.notes(r"""
课上先让学生写 $E = Exp - 7$、$M = 1.fff$，再各自算。下一页对答案。
""")


def tiny_float_answer(p):
    p.title("小练习：8 位浮点编码")
    p.slide(r"""
8位编码：1+4+3，$Bias = 7$，$E = Exp - 7$，规格化 $M = 1.fff$。
- `0 0110 100`：$Exp = 6$，$E = -1$，$M = 1.100_2 = 1.5$，值为 $0.75$
- `2.5`：$10.1_2 = 1.01_2 \times 2^{1}$，$Exp = 8 = 1000$，$frac = 010$，编码 `0 1000 010`
- $0.011_2$：$1.1_2 \times 2^{-2}$，$Exp = 5 = 0101$，$frac = 100$，编码 `0 0101 100`，值为 $0.375$
""", reveal="items")
    p.notes("""
三道都是规格化值，没有用到全 0 / 全 1。
第一道从位读到十进制，后两道先写成二进制科学计数法再填字段。
""")


# ==============================================================================
# 第五部分 · 浮点数的精度特性、舍入机制与现代格式扩展
# ==============================================================================
#
# 本节文案来自仓库根目录的 part-5.md，共 9 页，逐字照录，这里不对其做任何改写。
# 排版上只做三件事：「幻灯片标题」接到 p.title；「正文要点」、表格与代码（连同它们
# 的标签行）按原有的层级接到 p.slide / p.table / p.code；「章节定位」「核心教学目标」
# 「讲师点拨与过渡」（第 8 页称「讲授导引与过渡」）进 p.notes。「推荐配图与示意图设计」
# 是给配图的说明，不上屏，每页按它画了图，生成脚本在 diagrams/ 下，图中的标注是配图
# 自己的文字，不属于 part-5.md。
#
# 一页放不下时拆成同名的几页（`*_cont`、`*_cont2`、`*_cont3`），拆分点都落在
# part-5.md 的要点分组之间；「核心教学目标」放在拆出的第一页，「讲师点拨与过渡」
# 放在最后一页。代码清单照录 part-5.md，注释是中文，是本讲唯一不按「代码注释统一
# 写英文」的地方；这些代码只上屏，不在 examples/ 下。

ABSORB_CODE = r"""// 示例 1: 增量吸收现象
float large = 16777216.0f; // 2^24
float result = large + 1.0f;
printf("large + 1.0f == large: %s\n", (result == large) ? "true" : "false");
// 输出: true。1.0f 右移 24 位后丢失，被舍入消除。

// 示例 2: 循环终止异常
for (float f = 1e8f; f < 1e8f + 10.0f; f += 1.0f) {
    // 1e8 附近的 ULP 为 8.0f，每次 +1.0f 均被吸收，循环陷入死循环
}"""

ROUND_BITS_TEXT = """数值真值       二进制表示      截断判决状态                 舍入结果
--------------------------------------------------------------------------
2 + 3/32     10.00 | 011...  截断部分 < 100... (下溢)   10.00 (数值 2.0)
2 + 3/16     10.00 | 110...  截断部分 > 100... (进位)   10.01 (数值 2.25)
2 + 5/8      10.10 | 100...  精确中点，前位为 0 (偶)     10.10 (数值 2.5)
2 + 7/8      10.11 | 100...  精确中点，前位为 1 (奇进位) 11.00 (数值 3.0，跨阶码进位)"""

CANCELLATION_CODE = r"""// 1. 结合律失效与精度丢失
float a = 1e20f, b = -1e20f, c = 3.14f;
printf("(a + b) + c = %f\n", (a + b) + c); // 输出 3.140000
printf("a + (b + c) = %f\n", a + (b + c)); // 输出 0.000000 (1e20 + 3.14 发生吸收)

// 2. 精度比较陷阱
double x = 0.1 + 0.2;
printf("0.1 + 0.2 == 0.3: %s\n", (x == 0.3) ? "true" : "false"); // 输出 false

// 3. 防御性编程范式：禁止直接使用 == 判定浮点相等，改用容差阈值
bool is_equal = fabs(x - 0.3) < 1e-9;"""

CASTS_CODE = r"""int x = 16777217; // 2^24 + 1
float f = (float)x;
printf("x == (int)f ? %s\n", (x == (int)f) ? "true" : "false"); // 输出: false (被舍入为 16777216)

double out_of_range = 3e9; // 超出 INT_MAX (~2.14e9)
int i = (int)out_of_range;
printf("(int)3e9 = %d\n", i); // 在 x86-64 下输出 -2147483648"""

BF16_CODE = r"""// FP32 转 BF16 的硬件位级仿真（严格执行向最近偶数舍入）
uint16_t float_to_bfloat16(float f) {
    uint32_t u;
    memcpy(&u, &f, sizeof(u)); // 读取 32 位底层位模式

    // 提取 BF16 尾数末位最低有效位
    uint32_t lsb = (u >> 16) & 1;
    // 计算舍入偏置：0x7FFF 对应中点边界，加上 lsb 使得当且仅当中点且末位为奇数时进位
    uint32_t rounding_bias = 0x7FFF + lsb;
    u += rounding_bias; // 进位溢出将自动跨字段传播至阶码

    // 截取高 16 位输出
    return (uint16_t)(u >> 16);
}"""


# part-5.md 第 1 页：实数轴上的非均匀离散分布与相对精度

def ulp_distribution(p):
    p.title("实数轴上的非均匀离散分布与相对精度")
    p.slide(r"""
- **几何分布特征**：
  - 阶码 $E$ 确定基准区间 $[2^E, 2^{E+1})$，尾数字段（23 位）将该区间均分为 $2^{23}$ 等份。
  - 每越过一个 2 的整次幂，相邻可表示值的间距翻倍，实数轴上的网格密度呈指数级衰减。
- **网格间距 ULP 与绝对误差**：
  - 区间 $[2^E, 2^{E+1})$ 内的网格间距为 $\text{ULP} = 2^{E-23}$。
  - 绝对误差上界随数值量级的增大呈指数增长。
- **相对精度的恒定性**：
  - 相对精度保持恒定：$\frac{\text{步长}}{\text{区间起点}} = \frac{2^{E-23}}{2^E} = 2^{-23} \approx 1.19 \times 10^{-7}$。
  - 对应十进制约 7 位有效数字。
""")
    p.notes(r"""
章节定位：本讲承接 IEEE 754 标准的基础编码格式（符号位 $s$、阶码字段 $\exp$、尾数字段 $\text{frac}$，以及规格化、非规格化与特殊值的划分），深入探讨浮点数在硬件实现中的离散特性、舍入判决逻辑、高阶编程语言中的行为陷阱，以及现代计算体系结构面向深度学习对该格式的演化与扩展。

核心教学目标：建立浮点数在实数轴上呈分段等比缩放分布的几何直觉，严格区分绝对精度与相对精度的数学定义。
""")


def ulp_distribution_cont(p):
    p.title("实数轴上的非均匀离散分布与相对精度")
    p.image("assets/ulp-spacing.svg", alt="浮点数在实数轴上的刻度：每跨过一个 2 的幂，间距翻倍",
            width_px=1000)
    p.slide("- **不同区间对比**：")
    p.table(
        headers=["数值区间", "阶码 $E$", "绝对精度", "相对精度 = 绝对精度 / 区间起点"],
        rows=[
            ["$[1, 2)$", "$0$", r"$2^{-23} \approx 1.19 \times 10^{-7}$", "$2^{-23}$"],
            ["$[2^{10}, 2^{11})$", "$10$", r"$2^{-13} \approx 1.22 \times 10^{-4}$", "$2^{-23}$"],
            ["$[2^{24}, 2^{25})$", "$24$", "$2^1 = 2.0$", "$2^{-23}$"],
        ],
        align=["left", "center", "center", "center"],
    )
    p.notes(r"""
讲师点拨与过渡：“请关注区间 $[2^{24}, 2^{25})$：在此区间内，相邻浮点数的间距已经达到了 2。这意味着所有奇数整数在此范围内均无法被精确表示。若对处于该区间的数值进行细粒度累加，硬件底层将发生什么现象？”
""")


# part-5.md 第 2 页：阶码对齐截断与浮点吸收现象

def fp_absorption(p):
    p.title("阶码对齐截断与浮点吸收现象")
    p.slide(r"""
- **浮点加法的对齐过程**：
  - 浮点加法执行前必须先进行阶码对齐：较小阶码的操作数尾数需向右移位。
  - 移位超出尾数寄存器物理位宽的部分不会被直接丢弃，而是保留为 GRS 位参与舍入判断。
- **浮点吸收现象**：
  - 当阶码差 $\Delta E$ 足够大（达到尾数位数量级）时，较小操作数经对齐后的贡献可能小于半个末位精度（0.5 ulp），舍入的最终结果与直接丢弃该操作数等效。
  - 数学表现：存在非零正数 $y > 0$，使得硬件计算结果满足 $x \oplus y = x$。
""")
    p.image("assets/fp-absorb.svg", alt="2^24 加 1.0：对阶时 1.0 的尾数右移 24 位，唯一的 1 被移出窗口",
            width_px=900)
    p.notes("""
核心教学目标：从浮点加法的微架构执行过程（阶码对齐至大阶码、小阶码尾数右移、GRS 舍入判断），
解释小增量在数值计算中被舍入"抹平"的机理，说明机器精度的物理界限。

边界细节（进阶）：ΔE ≥ p+1=25 时，无论尾数奇偶，吸收必然发生；
本例 ΔE 恰好等于 24，属于舍入的临界(tie)情况——y=1.0 恰好等于半个 ulp，
且 2^24 尾数末位为偶数，"取偶"规则使结果保持不变。
若 x 尾数末位为奇数（如 2^24+2），或 y 不是 2 的整数次幂，
则同样 ΔE=24 时通常会舍入进位，并不表现为吸收。
""")


def fp_absorption_cont(p):
    p.title("阶码对齐截断与浮点吸收现象")
    p.slide(r"""
- **软件系统中的异常隐患**：
  - 浮点计数器停滞：若单精度浮点累加器数值达到 $16,777,216$（即 $2^{24}$），增量为 $1$ 的累加运算将不再改变变量值，导致依赖浮点步长递增的循环无法终止。
- **代码示例与行为剖析**：
""")
    p.code("c", ABSORB_CODE)
    p.notes("""
讲师点拨与过渡：“既然实数运算的中间结果往往无法精确对齐到离散的网格点上，硬件就必须依据严格的规则将其映射到最近的可表示值上。这就是 IEEE 754 规定的舍入系统。”
""")


# part-5.md 第 3 页：IEEE 754 舍入模式与无偏性机制

def rounding_modes(p):
    p.title("IEEE 754 舍入模式与无偏性机制")
    p.slide("""
- **IEEE 754 标准规定的四种舍入模式**：
  1. **向零截断**：直接舍弃小数，方向始终朝向原点。
  2. **向下舍入**：取不大于真值的最大浮点数。
  3. **向上舍入**：取不小于真值的最小浮点数。
  4. **向最近偶数舍入**：系统默认工作模式。
- **传统四舍五入的系统缺陷**：
  - 当真值恰好落在正中时一律向上取整，在对大规模正数序列进行持续累加时，会引入正向的系统性统计偏差。
- **向偶数舍入的无偏性设计**：
  - 仅在真值恰好处于两个可表示值正中时生效：强制选择最低有效位为 0（偶数）的一侧。
  - 在统计上，中点边界处向上与向下舍入的概率各为 50%，累积舍入误差的数学期望为 0。
""")
    p.notes("""
核心教学目标：掌握 IEEE 754 的四种法定舍入模式，从统计学角度阐明为何系统默认采用向最近偶数舍入，而非传统的四舍五入。
""")


def rounding_modes_cont(p):
    p.title("IEEE 754 舍入模式与无偏性机制")
    p.image("assets/round-bias.svg", alt="中点 1.5 至 8.5 的舍入误差：四舍五入一路累积，向偶数舍入在 0 附近振荡",
            width_px=800)
    p.slide("- **舍入效果对比表**：")
    p.table(
        headers=["实数真值", "向零截断", "向上舍入", "传统四舍五入",
                 "**向最近偶数舍入（IEEE 默认）**"],
        rows=[
            ["`1.4`", "`1`", "`2`", "`1`", "`1`"],
            ["`1.6`", "`1`", "`2`", "`2`", "`2`"],
            ["**`1.5`（中点）**", "`1`", "`2`", "`2`", "**`2`**（偶数）"],
            ["**`2.5`（中点）**", "`2`", "`3`", "`3`", "**`2`**（偶数，向下取整）"],
        ],
        align=["center", "center", "center", "center", "center"],
    )
    p.notes("""
讲师点拨与过渡：“向偶数舍入在数学统计上是优雅的。但从微架构与组合逻辑的角度来看，处理器电路是如何在流水线内判定‘精确中点’并执行进位的？”
""")


# part-5.md 第 4 页：二进制小数的位级舍入判决与进位传播

def round_bits(p):
    p.title("微架构视角：二进制小数的位级舍入判决")
    p.slide(r"""
- **位级中点的物理表示**：
  - 设保留位数截止于某一位。若被截断部分的最高位为 `1`，且后续所有位全为 `0`（即位模式 `1000..._2`），该状态即为数学上的精确中点。
- **硬件的三级裁决状态机**：
  - 截断部分最高位为 `0` $\to$ 小于中点，直接舍弃。
  - 截断部分大于 `1000..._2` $\to$ 大于中点，向最低有效位进位（$+1$）。
  - 截断部分恰好为 `1000..._2` $\to$ 检查保留位末位：若为 `1` 则进位使其变为偶数（`0`）；若已为 `0` 则直接舍弃。
- **进位传播机制**：
  - 若尾数各位全为 `1`，舍入进位会导致尾数字段溢出清零，同时触发阶码字段自动加 1，数值跨入下一个 2 的幂次区间。
""")
    p.notes("""
核心教学目标：使学生掌握硬件判断精确中点的位模式规则，理解舍入进位跨越字段边界对阶码的影响。
""")


def round_bits_cont(p):
    p.title("微架构视角：二进制小数的位级舍入判决")
    p.image("assets/round-grs.svg", alt="尾数末位 LSB 与保护位 G、舍入位 R、粘滞位 S，以及判定进位的逻辑门",
            width_px=940)
    p.slide("- **二进制位级推演示例**（保留至小数点后第 2 位）：")
    p.code("text", ROUND_BITS_TEXT)
    p.notes("""
讲师点拨与过渡：“硬件正是通过这些位级组合逻辑完成了上述判决。然而，有限位宽带来的持续舍入，直接导致了传统代数公理在计算机上的彻底失效。”
""")


def cancellation(p):
    p.title("浮点算术对代数公理的违背与灾难性抵消")
    p.slide(r"""
- **加法结合律的失效**：
  - $(a \oplus b) \oplus c \neq a \oplus (b \oplus c)$。运算顺序改变了中间结果的阶码对齐截断量，破坏了结合律与分配律。
- **灾难性抵消**：
  - 当两个相近的浮点数执行减法时，高位有效数字完全抵消，尾部因有限精度产生的舍入误差被提升为主导有效位，导致有效数字位数断崖式下跌。
""")
    p.image("assets/cancellation.svg", alt="两个前 21 位相同的数相减，规格化后只剩 3 位有效精度",
            width_px=820)
    p.notes("""
核心教学目标：剖析浮点运算中加法结合律失效的原因，阐明灾难性抵消的物理机理与数值风险，确立浮点比较的防御性编程规范。
""")


def cancellation_cont(p):
    p.title("浮点算术对代数公理的违背与灾难性抵消")
    p.slide(r"""
- **十进制有限小数与二进制循环小数的鸿沟**：
  - $0.1_{10} = 0.0001100110011\dots_2$。字面量 `0.1` 存储在内存中即为不可逆的近似截断值。
- **代码验证与防御性规范**：
""")
    p.code("c", CANCELLATION_CODE)
    p.notes("""
讲师点拨与过渡：“除了算术运算不满足结合律之外，在 C/C++ 等系统级语言中，浮点数与整型之间的混合交互还存在着由硬件与编译器联合构成的未定义行为陷阱。”
""")


# part-5.md 第 6 页：C 语言浮点转换规则、精度损失与未定义行为

def casts_ub(p):
    p.title("C 语言浮点转换规则、精度损失与未定义行为")
    p.slide(r"""
- **转换矩阵与精度保真度分析**：
  - `int` 转 `double`：保真。`int` 具有 31 位精度，`double` 尾数位宽为 53 位，可精确容纳。
  - `int` 转 `float`：有损。`float` 仅提供 24 位有效精度，超过 $2^{24}$（16,777,216）的整型值将发生舍入。
  - `float` 转 `double`：精确无损扩展；`double` 转 `float`：面临数值溢出或低位舍入。
  - 浮点数转整数：统一采用向零截断。
- **越界未定义行为与底层硬件处理**：
  - 当浮点数数值超出目标整型表示范围（如 `(int)3e9`），ISO C 标准定义其为未定义行为。
  - x86-64 体系结构下，转换指令对越界或无效浮点数统一输出固定的特殊值：`0x80000000`（即 $-2147483648$）。
""")
    p.notes("""
核心教学目标：建立完整的类型转换行为矩阵，理解浮点转整数溢出时 C 标准未定义行为与 x86-64 指令集的硬件响应。
""")


def casts_ub_cont(p):
    p.title("C 语言浮点转换规则、精度损失与未定义行为")
    p.image("assets/cast-paths.svg", alt="int、float、double 之间的六条转换：绿色精确，黄色舍入，红色截断或越界",
            width_px=820)
    p.slide("- **测试代码与汇编级行为映射**：")
    p.code("c", CASTS_CODE)
    p.notes("""
讲师点拨与过渡：“如果说未定义行为在日常开发中可能仅导致异常崩溃，那么当不可避免的截断误差被物理时间与空间系统性放大时，将演变为灾难性的系统故障。”
""")


# part-5.md 第 7 页：系统工程案例：爱国者防空系统时钟截断累积事故

def patriot(p):
    p.title("系统工程案例：爱国者防空系统时钟截断累积事故")
    p.slide(r"""
- **事故背景**：1991 年 2 月海湾战争期间，美军部署于宰赫兰的爱国者防空系统未能拦截来袭的飞毛腿导弹，造成 28 名士兵丧生。
- **误差根源：时钟基准的二进制截断**：
  - 系统内部依靠每 0.1 秒递增一次的整型计数器计算运行时间，将计数乘以 0.1 转换为浮点秒数。
  - $0.1_{10}$ 展开为二进制无限循环小数，存入 24 位定点寄存器时发生截断，单步时钟截断误差 $\Delta t \approx 9.54 \times 10^{-8}\text{ s}$。
- **连续时间积分下的宏观漂移推导**：
  - 系统连续无重启运行 100 小时：
  - 计数器累加脉冲总数：$N = 100 \times 3600 \times 10 = 3,600,000$ 次。
  - 绝对时间漂移量：$\Delta T = 3.6 \times 10^6 \times 9.54 \times 10^{-8}\text{ s} \approx \mathbf{0.3433\text{ 秒}}$。
""")
    p.notes("""
核心教学目标：通过真实工程事故，严密推导微小的单步浮点截断误差如何在时间积分与物理速度耦合下造成致命的系统性失效。
""")


def patriot_cont(p):
    p.title("系统工程案例：爱国者防空系统时钟截断累积事故")
    p.slide(r"""
- **目标跟踪测距失效机制**：
  - 飞毛腿导弹末端飞行速度达 $v \approx 2000\text{ m/s}$。
  - 空间位置预测漂移：$\Delta S = v \times \Delta T \approx 2000 \times 0.3433 \approx \mathbf{687\text{ 米}}$。
  - 目标落入雷达测距跟踪窗口之外，雷达判定目标虚标并不予引导发射。
- **系统工程教训**：
  - 研发团队在事故前修改了部分子程序的高精度时间转换例程，但未同步更新所有调用模块，导致混合精度模块间残差无法相互抵消。
""")
    p.row()\
     .image("assets/patriot-drift.svg", alt="时钟误差随运行时间线性增长，100 小时后为 0.3433 秒",
            width_px=540)\
     .image("assets/patriot-gate.svg",
            alt="雷达按滞后 0.3433 秒的时钟计算跟踪窗口，导弹实际位置在窗口前方 687 米",
            width_px=560)
    p.notes("""
讲师点拨与过渡：“爱国者导弹案例警示我们：高精度是复杂系统可靠性的基石。但在近十年的计算体系结构演进中，一个全新的领域——现代深度学习——却对这一传统认知发起了挑战，催生了一场‘牺牲精度换取动态范围与吞吐’的格式演化。”
""")


# part-5.md 第 8 页：现代体系结构扩展：动态范围与相对精度的折衷

def bf16_tradeoff(p):
    p.title("现代体系结构扩展：动态范围与相对精度的折衷")
    p.slide(r"""
- **位宽约束下的设计权衡公理**：
  - 在总位宽固定为 16 位的物理约束下：阶码字段位数决定动态范围（数值上下界）；尾数字段位数决定相对精度。
- **传统标准半精度浮点 FP16 的工业局限**：
  - 阶码仅 5 位，最大表示值受限于 $65504$。
  - 深度学习反向传播计算梯度时极易触发数值上溢（溢出为无穷大）与下溢（下溢为零），必须依赖复杂的混合精度动态缩放软件栈支撑。
""")
    p.image("assets/fp16-bf16-fields.svg", alt="FP32、FP16、BF16 的符号、阶码、尾数三段，BF16 与 FP32 的高 16 位对齐",
            width_px=920)
    p.notes("""
核心教学目标：深入理解专用加速器在 16 位位宽约束下的设计权衡空间，讲透为什么现代大模型训练选择放弃 FP16 而全面转向 BF16。
""")


def bf16_tradeoff_cont(p):
    p.title("现代体系结构扩展：动态范围与相对精度的折衷")
    p.slide(r"""
- **BF16 的设计突破**：
  - 维持与 FP32 完全相同的 8 位阶码位宽，将尾数压缩至 7 位。
  - 核心权衡：接受相对精度的降低（十进制有效数字降至约 2.4 位），换取与 FP32 完全等价的动态范围（$\pm 3.4 \times 10^{38}$），彻底消除训练梯度溢出风险。
""")
    p.slide("- **核心格式规格对照表**：")
    p.table(
        headers=["格式", "位分配 (s/exp/frac)", "动态范围上界",
                 "十进制有效位", "核心应用领域"],
        rows=[
            ["**FP32**", "1/**8**/23", r"$\approx \pm 3.4 \times 10^{38}$",
             r"$\approx 7.2$", "科学计算、基准训练"],
            ["**FP16**", "1/**5**/10", "**$65504$（易溢出）**",
             r"$\approx 3.3$", "图形学、边缘推理"],
            ["**BF16**", "1/**8**/7", r"$\approx \pm 3.4 \times 10^{38}$",
             r"$\approx 2.4$", "**大模型训练与推理**"],
        ],
        align=["left", "center", "center", "center", "left"],
    )


def bf16_mapping(p):
    p.title("微架构实现友好性：FP32 到 BF16 的位级映射")
    p.slide("""
- **位布局天然对齐优势**：
  - FP32 与 BF16 的符号位、阶码位在物理位置上严格一致（位 31 至位 16）。
  - 最精简的转换仅需单条移位指令截取 FP32 的高 16 位，反向扩展仅需低位补零。对比 FP16（需动态重新计算偏置、处理非规格化移位），逻辑门开销极低。
""")
    p.image("assets/bf16-datapath.svg", alt="FP32 转 BF16 只需高 16 位直通加一次进位；FP32 转 FP16 需要逐项改写",
            width_px=940)
    p.notes("""
核心教学目标：通过位操作代码对比硬件复杂度，论证为何 BF16 能够大幅削减加速器芯片的数据通路面积与功耗开销，并总结本节的体系结构权衡思想。
""")


# ==============================================================================
# 综合
# ==============================================================================

def summary(p):
    p.gap(30)
    p.title("课程小结")
    p.slide("""
- **字节概念与寻址单位**：8 个二进制 Bits 组合，对应 2 位十六进制数，是内存寻址的最小基本单位
- **多字节存储与字节序**：由体系结构字节序规定，现代通用处理器（x86-64、ARM64）及 GGUF 格式普遍采用小端序
- **整数二进制编码规则**：补码赋予最高符号位负权重，有符号数与无符号数之间的隐式转换是常见的安全缺陷来源
- **实数表示与模型量化**：浮点数分配符号、阶码与尾数；分组量化技术将权重压缩至 4 位宽，大幅降低内存访存瓶颈
""", reveal="items")
    p.slide("""
数据比特本身不具有固有语义，其含义完全取决于解释规则。
类型转换与位操作边界处的隐式语义不会触发编译报错，需要开发者保持严谨的代码审查意识。
""", autobold=False)
    p.highlight("解释字节序列之前，必须首先明确其数据类型与解析规范。", tone="blue")
    p.notes("""
这四条合起来就是把 1.9 GB 字节读成一个模型所需的全部约定。
还可以补一句与后续内容的连接：机器级程序里立即数与地址同样按小端存放，
存储层次一节里数据的位宽直接决定每个 token 的访存量，
链接与动态加载处理的目标文件同样是有规范的字节序列，用的是同一套方法。
""")
