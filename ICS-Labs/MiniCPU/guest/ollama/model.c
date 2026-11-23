/* model.c -- bring a GGUF file into memory and find the tensors in it.

   The file is standard input, mapped with fstat and mmap, so the program
   opens nothing. die() is in libc.c. MAX_WIDTH is 8192, for a stack of
   256 KiB.

   GGUF version 3, every integer little endian:

     "GGUF", uint32 version, uint64 number of tensors, uint64 number of pairs
     the key-value pairs:    string key, uint32 type, value
     the tensor directory:   string name, uint32 n_dims, uint64 dims[n_dims],
                             uint32 type, uint64 offset in the data area
     padding to a multiple of general.alignment, then the data area

   A string is a uint64 length followed by that many bytes. The file is
   kept as it is: tensors, token texts and merges point into it. The
   integers are read with memcpy, so the machine has to be little endian. */
#include "libc.h"
#include "mini_ollama.h"

enum { MAX_WIDTH = 1 << 13 };           /* the longest vector kept on the stack */

float fp16[1 << 16];

/* ---------------- reading the bytes ---------------- */

struct reader {
    const uint8_t *next, *end;
};

static const uint8_t *take(struct reader *reader, uint64_t nbytes)
{
    const uint8_t *taken = reader->next;
    if (nbytes > (uint64_t)(reader->end - reader->next))
        die("the file ends inside its header");
    reader->next += nbytes;
    return taken;
}

static uint32_t rd_u32(struct reader *reader)
{
    uint32_t value;
    memcpy(&value, take(reader, 4), 4);
    return value;
}

static uint64_t rd_u64(struct reader *reader)
{
    uint64_t value;
    memcpy(&value, take(reader, 8), 8);
    return value;
}

static struct str rd_str(struct reader *reader)
{
    struct str str;
    str.len = rd_u64(reader);
    str.chars = (const char *)take(reader, str.len);
    return str;
}

static int str_is(struct str str, const char *name)
{
    return str.len == strlen(name) && memcmp(str.chars, name, str.len) == 0;
}

static int str_ends(struct str str, const char *tail)
{
    size_t tail_len = strlen(tail);
    return str.len >= tail_len &&
           memcmp(str.chars + str.len - tail_len, tail, tail_len) == 0;
}

/* ---------------- the key-value pairs ---------------- */

enum { U8, I8, U16, I16, U32, I32, F32, BOOL, STR, ARR, U64, I64, F64, N_TYPES };
static const int value_size[N_TYPES] = { 1, 1, 2, 2, 4, 4, 4, 1, 0, 0, 8, 8, 8 };

static void skip_value(struct reader *reader, uint32_t type)
{
    if (type == STR) {
        rd_str(reader);
    } else if (type == ARR) {
        uint32_t elem = rd_u32(reader);
        for (uint64_t remaining = rd_u64(reader); remaining > 0; remaining--)
            skip_value(reader, elem);
    } else if (type < N_TYPES) {
        take(reader, value_size[type]);
    } else {
        die("unknown value type %u", (unsigned)type);
    }
}

static int rd_int(struct reader *reader, uint32_t type, struct str key)
{
    if (type != U32)
        die("%.*s is not a uint32", (int)key.len, key.chars);
    return (int)rd_u32(reader);
}

static float rd_float(struct reader *reader, uint32_t type, struct str key)
{
    float value;
    if (type != F32)
        die("%.*s is not a float", (int)key.len, key.chars);
    memcpy(&value, take(reader, 4), 4);
    return value;
}

static struct str *rd_strings(struct reader *reader, uint32_t type, struct str key,
                              int *n_strings)
{
    if (type != ARR || rd_u32(reader) != STR)
        die("%.*s is not an array of strings", (int)key.len, key.chars);
    uint64_t count = rd_u64(reader);
    if (count > (uint64_t)(reader->end - reader->next) / 8)
        die("%.*s is longer than the file", (int)key.len, key.chars);
    struct str *strings = malloc((count + 1) * sizeof *strings);
    if (!strings)
        die("out of memory");
    for (uint64_t idx = 0; idx < count; idx++)
        strings[idx] = rd_str(reader);
    *n_strings = (int)count;
    return strings;
}

/* The numbers that describe the model carry the name of the architecture
   in front (qwen3vl.block_count), so they are matched by their ending. */
static void rd_pair(struct model *model, struct reader *reader, uint32_t *alignment)
{
    struct str key = rd_str(reader);
    uint32_t type = rd_u32(reader);

    if (str_ends(key, ".block_count"))
        model->n_layer = rd_int(reader, type, key);
    else if (str_ends(key, ".embedding_length"))
        model->n_embd = rd_int(reader, type, key);
    else if (str_ends(key, ".feed_forward_length"))
        model->n_ff = rd_int(reader, type, key);
    else if (str_ends(key, ".attention.head_count"))
        model->n_head = rd_int(reader, type, key);
    else if (str_ends(key, ".attention.head_count_kv"))
        model->n_head_kv = rd_int(reader, type, key);
    else if (str_ends(key, ".attention.key_length"))
        model->head_dim = rd_int(reader, type, key);
    else if (str_ends(key, ".attention.layer_norm_rms_epsilon"))
        model->eps = rd_float(reader, type, key);
    else if (str_ends(key, ".rope.freq_base"))
        model->rope_base = rd_float(reader, type, key);
    else if (str_is(key, "general.alignment"))
        *alignment = (uint32_t)rd_int(reader, type, key);
    else if (str_is(key, "tokenizer.ggml.eos_token_id"))
        model->eos = rd_int(reader, type, key);
    else if (str_is(key, "tokenizer.ggml.tokens"))
        model->vocab = rd_strings(reader, type, key, &model->n_vocab);
    else if (str_is(key, "tokenizer.ggml.merges"))
        model->merges = rd_strings(reader, type, key, &model->n_merges);
    else
        skip_value(reader, type);
}

/* ---------------- the tensor directory ---------------- */

enum { TYPE_F32 = 0, TYPE_Q4_0 = 2 };

struct entry {
    struct str name;
    uint32_t   n_dims, type;
    uint64_t   dims[2], offset;
};

struct directory {
    struct entry  *entry;
    uint64_t       count;
    const uint8_t *data, *end;          /* the data area */
};

static void rd_entry(struct reader *reader, struct entry *entry)
{
    entry->name = rd_str(reader);
    entry->n_dims = rd_u32(reader);
    if (entry->n_dims < 1 || entry->n_dims > 2)
        die("%.*s has %u dimensions", (int)entry->name.len, entry->name.chars,
            (unsigned)entry->n_dims);
    entry->dims[1] = 1;
    for (uint32_t dim = 0; dim < entry->n_dims; dim++)
        entry->dims[dim] = rd_u64(reader);
    entry->type = rd_u32(reader);
    entry->offset = rd_u64(reader);
}

/* The tensor called name (layer is put in for %d), which has to be a Q4_0
   matrix of rows x cols, or a float vector of cols when rows is 1. */
static struct tensor find(const struct directory *dir, const char *name, int layer,
                          int cols, int rows)
{
    char full[64];
    snprintf(full, sizeof full, name, layer);

    for (uint64_t idx = 0; idx < dir->count; idx++) {
        const struct entry *entry = &dir->entry[idx];
        if (!str_is(entry->name, full))
            continue;
        if (entry->dims[0] != (uint64_t)cols || entry->dims[1] != (uint64_t)rows)
            die("%s is %llu x %llu, expected %d x %d", full,
                (unsigned long long)entry->dims[0], (unsigned long long)entry->dims[1],
                cols, rows);
        if (rows > 1 && entry->type != TYPE_Q4_0)
            die("%s is not Q4_0: use a model with every matrix in Q4_0", full);
        if (rows == 1 && entry->type != TYPE_F32)
            die("%s is not a float vector", full);
        if (rows > 1 && cols % 32 != 0)
            die("%s: a row is not a whole number of Q4_0 blocks", full);

        uint64_t bytes = rows > 1 ? (uint64_t)rows * (cols / 32) * 18 : (uint64_t)cols * 4;
        uint64_t room = (uint64_t)(dir->end - dir->data);
        if (entry->offset > room || bytes > room - entry->offset)
            die("%s lies outside the file", full);
        struct tensor tensor = { (size_t)cols, (size_t)rows,
                                 dir->data + entry->offset };
        return tensor;
    }
    die("the file has no tensor %s", full);
    return (struct tensor){ 0, 0, NULL };
}

static void find_tensors(struct model *model, const struct directory *dir)
{
    int n_embd = model->n_embd, n_ff = model->n_ff, head_dim = model->head_dim;
    int query_dim = model->n_head * head_dim, kv_dim = model->n_head_kv * head_dim;

    model->token_embd  = find(dir, "token_embd.weight", 0, n_embd, model->n_vocab);
    model->output_norm = find(dir, "output_norm.weight", 0, n_embd, 1);
    model->logits = malloc((size_t)model->n_vocab * sizeof *model->logits);
    model->layer = calloc((size_t)model->n_layer, sizeof *model->layer);
    if (!model->logits || !model->layer)
        die("out of memory");

    /* GGUF calls a layer a block: blk.0, blk.1, ... */
    for (int layer = 0; layer < model->n_layer; layer++) {
        struct layer *block = &model->layer[layer];
        block->attn_norm   = find(dir, "blk.%d.attn_norm.weight", layer, n_embd, 1);
        block->attn_q      = find(dir, "blk.%d.attn_q.weight", layer, n_embd, query_dim);
        block->attn_k      = find(dir, "blk.%d.attn_k.weight", layer, n_embd, kv_dim);
        block->attn_v      = find(dir, "blk.%d.attn_v.weight", layer, n_embd, kv_dim);
        block->attn_q_norm = find(dir, "blk.%d.attn_q_norm.weight", layer, head_dim, 1);
        block->attn_k_norm = find(dir, "blk.%d.attn_k_norm.weight", layer, head_dim, 1);
        block->attn_output = find(dir, "blk.%d.attn_output.weight", layer, query_dim, n_embd);
        block->ffn_norm    = find(dir, "blk.%d.ffn_norm.weight", layer, n_embd, 1);
        block->ffn_gate    = find(dir, "blk.%d.ffn_gate.weight", layer, n_embd, n_ff);
        block->ffn_up      = find(dir, "blk.%d.ffn_up.weight", layer, n_embd, n_ff);
        block->ffn_down    = find(dir, "blk.%d.ffn_down.weight", layer, n_ff, n_embd);
        block->k_cache = malloc((size_t)MAX_CTX * kv_dim * sizeof(float));
        block->v_cache = malloc((size_t)MAX_CTX * kv_dim * sizeof(float));
        if (!block->k_cache || !block->v_cache)
            die("out of memory");
    }
}

/* ---------------- the whole file ---------------- */

/* Half precision: 1 sign bit, 5 exponent bits, 10 fraction bits. Q4_0 scales
   are finite, so infinity and NaN are not told apart from large numbers. */
static void fill_fp16(void)
{
    for (int bits = 0; bits < 1 << 16; bits++) {
        int exp = bits >> 10 & 31, frac = bits & 1023;
        float value = exp == 0 ? ldexpf((float)frac, -24)
                               : ldexpf((float)(frac + 1024), exp - 25);
        fp16[bits] = bits >> 15 ? -value : value;
    }
}

/* Map standard input read-only. Nothing is copied: a page of the file comes
   into memory when the program first touches it. */
static const uint8_t *load_file(size_t *size)
{
    struct stat st;
    if (fstat(0, &st) != 0)
        die("standard input is not open");
    void *mapping = st.st_size > 0
                  ? mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_PRIVATE, 0, 0)
                  : MAP_FAILED;
    if (mapping == MAP_FAILED)
        die("cannot map standard input: it has to be a file, < MODEL.gguf");
    *size = (size_t)st.st_size;
    return mapping;
}

struct model *load_model(void)
{
    size_t size;
    const uint8_t *file = load_file(&size);
    struct reader reader = { file, file + size };
    struct model *model = calloc(1, sizeof *model);
    struct directory dir;
    uint32_t alignment = 32;

    if (!model)
        die("out of memory");
    if (memcmp(take(&reader, 4), "GGUF", 4) != 0 || rd_u32(&reader) != 3)
        die("standard input is not a GGUF version 3 file");
    dir.count = rd_u64(&reader);
    uint64_t n_pairs = rd_u64(&reader);

    model->eos = -1;
    for (uint64_t pair = 0; pair < n_pairs; pair++)
        rd_pair(model, &reader, &alignment);
    if (model->n_layer <= 0 || model->n_embd <= 0 || model->n_ff <= 0 ||
        model->n_head <= 0 || model->n_head_kv <= 0 || model->head_dim <= 0 ||
        model->n_head % model->n_head_kv != 0 || model->head_dim % 2 != 0 ||
        model->rope_base <= 0 || !model->vocab || !model->merges ||
        model->eos < 0 || model->eos >= model->n_vocab || alignment == 0 ||
        model->n_embd > MAX_WIDTH || model->n_ff > MAX_WIDTH ||
        model->n_head > MAX_WIDTH / model->head_dim)
        die("the file does not describe a model this program can run");

    if (dir.count > (uint64_t)(reader.end - reader.next) / 24)
        die("the tensor directory is longer than the file");
    dir.entry = malloc((dir.count + 1) * sizeof *dir.entry);
    if (!dir.entry)
        die("out of memory");
    for (uint64_t idx = 0; idx < dir.count; idx++)
        rd_entry(&reader, &dir.entry[idx]);

    uint64_t header = (uint64_t)(reader.next - file);
    uint64_t start = (header + alignment - 1) / alignment * alignment;
    if (start > size)
        die("the file ends before its data area");
    dir.data = file + start;
    dir.end = file + size;

    find_tensors(model, &dir);
    free(dir.entry);
    fill_fp16();
    tokenizer_init(model);
    return model;
}
