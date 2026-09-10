/* nano_quant.h —— 实验要求实现的全部函数。
 *
 * 这个头文件是学生代码与框架之间唯一的接口。框架只调用这里声明的函数，
 * 不读学生源文件里的其他任何名字。实现写在 student/student.cpp 里，
 * 不要修改本文件。
 *
 * 全部函数都是纯函数：只读入参、只写出参，不分配内存，不打印，不退出。
 * 所有多字节整数按小端解释。所有浮点计算用 float，不要用 double 中转，
 * 否则逐字节判定会失败。
 */
#ifndef NANO_QUANT_H
#define NANO_QUANT_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* 就近取整，平局取偶。三种格式的量化都要用它，不要换成别的写法：
   (int)(x + 0.5f) 在负数处与在平局处都与它不同，逐字节判定会失败。
   实现是一个浮点惯用法：加上 2^23 + 2^22 把尾数对齐到整数位，
   硬件的默认舍入方式正好是就近舍入、平局取偶，取低位即得结果。
   参数的绝对值需要小于 2^22。 */
static inline int nq_round(float f) {
    float v = f + 12582912.0f;
    int32_t i;
    memcpy(&i, &v, sizeof i);
    return (i & 0x007fffff) - 0x00400000;
}

/* ---------- A 部分：读位与拼位 ---------- */

/* 把 p 指向的 8 个字节按小端合成一个 64 位无符号数。
   safetensors 文件的头长度就用这个函数读出来。 */
uint64_t rd_u64le(const uint8_t *p);

/* BF16 的 16 位表示转成 float。BF16 与 FP32 的符号位和指数域完全一致，
   尾数是 FP32 尾数的高 7 位，所以这一步是一次左移。 */
float bf16_to_f32(uint16_t h);

/* IEEE 754 半精度（1 位符号、5 位指数、10 位尾数）转成 float。
   要处理规格化数、次规格化数、零、无穷与 NaN 四类。 */
float fp16_to_f32(uint16_t h);

/* float 转成 IEEE 754 半精度，舍入方式是就近舍入、平局取偶。
   超出半精度范围的按无穷处理，太小的按次规格化数或零处理。
   三种块格式的缩放系数都用这个函数写出，它错了整个产物都对不上。 */
uint16_t f32_to_fp16(float f);

/* ---------- B 部分：三种块格式 ---------- */

/* 块的字节数。写出的字节序列必须与 ggml 的同名格式逐字节相同。 */
#define NQ_Q4_0_BLOCK_ELEMS  32
#define NQ_Q4_0_BLOCK_BYTES  18   /* d:fp16, qs[16] */
#define NQ_Q4_1_BLOCK_ELEMS  32
#define NQ_Q4_1_BLOCK_BYTES  20   /* d:fp16, m:fp16, qs[16] */
#define NQ_Q4_K_BLOCK_ELEMS 256
#define NQ_Q4_K_BLOCK_BYTES 144   /* d:fp16, dmin:fp16, scales[12], qs[128] */

/* Q4_0：对称，一个 fp16 步长，16 个级摊在 [-8d, 7d] 上。
   x 有 32 个元素，blk 有 18 字节。 */
void q4_0_quantize(const float *x, uint8_t *blk);
void q4_0_dequantize(const uint8_t *blk, float *x);

/* Q4_1：带偏移，x̂ = d * q + m，d 与 m 都是 fp16。
   x 有 32 个元素，blk 有 20 字节。 */
void q4_1_quantize(const float *x, uint8_t *blk);
void q4_1_dequantize(const uint8_t *blk, float *x);

/* Q4_K：256 个元素一个超块，分成 8 个 32 元素的子块。
   超块存 d 与 dmin 两个 fp16，8 组 (缩放, 偏移) 各量化成 6 位，
   16 个 6 位数压进 12 字节。子块 j 的还原式是
       x̂ = (d * sc_j) * q - (dmin * m_j)
   x 有 256 个元素，blk 有 144 字节。 */
void q4_k_quantize(const float *x, uint8_t *blk);
void q4_k_dequantize(const uint8_t *blk, float *x);

/* Q4_K 的 12 字节里第 j 组 6 位缩放与 6 位偏移的存取。
   put_scale_min 用 |= 写入高两位，所以调用前 q 的 12 字节必须清零。
   get_scale_min 已经给出，照它写出 put_scale_min，使两者互逆。 */
void get_scale_min(int j, const uint8_t *q, uint8_t *sc, uint8_t *m);
void put_scale_min(int j, uint8_t *q, uint8_t sc, uint8_t m);

/* ---------- C 部分：按配方分派 ---------- */

/* 张量在产物里的类型。数值与 GGUF 的类型编号一致，不要改。 */
typedef enum {
    NQ_TYPE_F32  = 0,
    NQ_TYPE_F16  = 1,
    NQ_TYPE_Q4_0 = 2,
    NQ_TYPE_Q4_1 = 3,
    NQ_TYPE_Q4_K = 12,
    NQ_TYPE_Q6_K = 14,
} nq_type;

/* 配方。框架在 src/recipe.cpp 里给出三张表，见 nq.h 的说明。 */
typedef enum {
    NQ_RECIPE_Q4_0,     /* 所有二维张量走 Q4_0 */
    NQ_RECIPE_Q4_1,     /* 所有二维张量走 Q4_1 */
    NQ_RECIPE_Q4_K,     /* 所有二维张量走 Q4_K */
    NQ_RECIPE_Q4_K_M,   /* llama.cpp 的 Q4_K_M：词表与部分层升到 Q6_K */
} nq_recipe;

/* 这个张量要不要写进产物。
   模型文件里有 625 个张量，其中 315 个属于视觉塔，本实验只处理语言模型的
   310 个。name 是 safetensors 头里的原名。 */
bool tensor_selected(const char *name);

/* 这个张量在给定配方下用什么类型。
   n_dims 是维数，layer 是层号（不属于某一层的张量传 -1），
   n_layers 是模型总层数。判据见 README 的配方表一节。 */
nq_type recipe_pick(nq_recipe r, const char *name, int n_dims, int layer, int n_layers);

/* ---------- D 部分：名字映射 ---------- */

/* 把 safetensors 里的张量名改写成 GGUF 里的名字，写进 buf。
   成功返回 0，buf 不够或名字不认识返回 -1。
   对应关系见 README 的名字映射表一节。 */
int gguf_name(const char *hf_name, char *buf, size_t buf_size);

/* 从张量名里取出层号；不属于某一层的返回 -1。
   例如 model.language_model.layers.17.mlp.down_proj.weight 的层号是 17。 */
int layer_of(const char *name);

#endif /* NANO_QUANT_H */
