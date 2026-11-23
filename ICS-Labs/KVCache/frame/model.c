/* model.c -- bring a GGUF file into memory and find the tensors in it.

   GGUF version 3, every integer little endian:

     "GGUF", uint32 version, uint64 number of tensors, uint64 number of pairs
     the key-value pairs:    string key, uint32 type, value
     the tensor directory:   string name, uint32 n_dims, uint64 dims[n_dims],
                             uint32 type, uint64 offset in the data area
     padding to a multiple of general.alignment, then the data area

   A string is a uint64 length followed by that many bytes. The whole file
   is read into memory and kept as it is: tensors, token texts and merges
   point into it. The integers are read with memcpy, so the machine has to
   be little endian. */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mini_ollama.h"

enum { MAX_WIDTH = 1 << 16 };           /* the longest vector of a model */

void die(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "%s: ", program_name);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(1);
}

/* ---------------- reading the bytes ---------------- */

struct reader {
    const uint8_t *next, *end;
};

static const uint8_t *take(struct reader *reader, uint64_t count)
{
    const uint8_t *start = reader->next;
    if (count > (uint64_t)(reader->end - reader->next))
        die("the file ends inside its header");
    reader->next += count;
    return start;
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
    str.text = (const char *)take(reader, str.len);
    return str;
}

static int str_is(struct str str, const char *name)
{
    return str.len == strlen(name) && memcmp(str.text, name, str.len) == 0;
}

static int str_ends(struct str str, const char *tail)
{
    size_t len = strlen(tail);
    return str.len >= len && memcmp(str.text + str.len - len, tail, len) == 0;
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
        for (uint64_t count = rd_u64(reader); count > 0; count--)
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
        die("%.*s is not a uint32", (int)key.len, key.text);
    return (int)rd_u32(reader);
}

static float rd_float(struct reader *reader, uint32_t type, struct str key)
{
    float value;
    if (type != F32)
        die("%.*s is not a float", (int)key.len, key.text);
    memcpy(&value, take(reader, 4), 4);
    return value;
}

static struct str *rd_strings(struct reader *reader, uint32_t type, struct str key, int *count)
{
    if (type != ARR || rd_u32(reader) != STR)
        die("%.*s is not an array of strings", (int)key.len, key.text);
    uint64_t total = rd_u64(reader);
    if (total > (uint64_t)(reader->end - reader->next) / 8)
        die("%.*s is longer than the file", (int)key.len, key.text);
    struct str *strings = malloc((total + 1) * sizeof *strings);
    if (!strings)
        die("out of memory");
    for (uint64_t i = 0; i < total; i++)
        strings[i] = rd_str(reader);
    *count = (int)total;
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
        die("%.*s has %u dimensions", (int)entry->name.len, entry->name.text,
            (unsigned)entry->n_dims);
    entry->dims[1] = 1;
    for (uint32_t i = 0; i < entry->n_dims; i++)
        entry->dims[i] = rd_u64(reader);
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

    for (uint64_t i = 0; i < dir->count; i++) {
        const struct entry *entry = &dir->entry[i];
        if (!str_is(entry->name, full))
            continue;
        if (entry->dims[0] != (uint64_t)cols || entry->dims[1] != (uint64_t)rows)
            die("%s is %llu x %llu, expected %d x %d", full,
                (unsigned long long)entry->dims[0], (unsigned long long)entry->dims[1], cols, rows);
        if (rows > 1 && entry->type != TYPE_Q4_0)
            die("%s is not Q4_0: use q4_0.gguf (README.md)", full);
        if (rows == 1 && entry->type != TYPE_F32)
            die("%s is not a float vector", full);
        if (rows > 1 && cols % 32 != 0)
            die("%s: a row is not a whole number of Q4_0 blocks", full);

        uint64_t bytes = rows > 1 ? (uint64_t)rows * (cols / 32) * 18 : (uint64_t)cols * 4;
        uint64_t room = (uint64_t)(dir->end - dir->data);
        if (entry->offset > room || bytes > room - entry->offset)
            die("%s lies outside the file", full);
        struct tensor tensor = { (size_t)cols, (size_t)rows, dir->data + entry->offset };
        return tensor;
    }
    die("the file has no tensor %s", full);
    return (struct tensor){ 0, 0, NULL };
}

static void find_tensors(struct model *model, const struct directory *dir)
{
    int embd = model->n_embd, ff = model->n_ff, head_dim = model->head_dim;
    int q_dim = model->n_head * head_dim, kv_dim = model->n_head_kv * head_dim;

    model->token_embd  = find(dir, "token_embd.weight", 0, embd, model->n_vocab);
    model->output_norm = find(dir, "output_norm.weight", 0, embd, 1);
    model->layer = calloc((size_t)model->n_layer, sizeof *model->layer);
    if (!model->layer)
        die("out of memory");

    for (int layer = 0; layer < model->n_layer; layer++) {
        struct layer *weights = &model->layer[layer];
        weights->attn_norm   = find(dir, "blk.%d.attn_norm.weight", layer, embd, 1);
        weights->attn_q      = find(dir, "blk.%d.attn_q.weight", layer, embd, q_dim);
        weights->attn_k      = find(dir, "blk.%d.attn_k.weight", layer, embd, kv_dim);
        weights->attn_v      = find(dir, "blk.%d.attn_v.weight", layer, embd, kv_dim);
        weights->attn_q_norm = find(dir, "blk.%d.attn_q_norm.weight", layer, head_dim, 1);
        weights->attn_k_norm = find(dir, "blk.%d.attn_k_norm.weight", layer, head_dim, 1);
        weights->attn_output = find(dir, "blk.%d.attn_output.weight", layer, q_dim, embd);
        weights->ffn_norm    = find(dir, "blk.%d.ffn_norm.weight", layer, embd, 1);
        weights->ffn_gate    = find(dir, "blk.%d.ffn_gate.weight", layer, embd, ff);
        weights->ffn_up      = find(dir, "blk.%d.ffn_up.weight", layer, embd, ff);
        weights->ffn_down    = find(dir, "blk.%d.ffn_down.weight", layer, ff, embd);
    }
}

/* ---------------- the whole file ---------------- */

/* Read the whole file into memory. ftell() returns a long, which is 32 bits
   on Windows, so the file has to be shorter than 2 GB. */
static const uint8_t *load_file(const char *path, size_t *size)
{
    FILE *file = fopen(path, "rb");
    if (!file)
        die("cannot open %s", path);
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    rewind(file);
    uint8_t *buf = length > 0 ? malloc((size_t)length) : NULL;
    if (!buf || fread(buf, 1, (size_t)length, file) != (size_t)length)
        die("cannot read %s", path);
    fclose(file);
    *size = (size_t)length;
    return buf;
}

struct model *load_model(const char *path)
{
    size_t size;
    const uint8_t *file = load_file(path, &size);
    struct reader reader = { file, file + size };
    struct model *model = calloc(1, sizeof *model);
    struct directory dir;
    uint32_t alignment = 32;

    if (!model)
        die("out of memory");
    model->file_bytes = size;
    if (memcmp(take(&reader, 4), "GGUF", 4) != 0 || rd_u32(&reader) != 3)
        die("%s is not a GGUF version 3 file", path);
    dir.count = rd_u64(&reader);
    uint64_t n_pairs = rd_u64(&reader);

    model->eos = -1;
    for (uint64_t i = 0; i < n_pairs; i++)
        rd_pair(model, &reader, &alignment);
    if (model->n_layer <= 0 || model->n_embd <= 0 || model->n_ff <= 0 || model->n_head <= 0 ||
        model->n_head_kv <= 0 || model->head_dim <= 0 || model->n_head % model->n_head_kv != 0 ||
        model->head_dim % 2 != 0 || model->rope_base <= 0 || !model->vocab || !model->merges ||
        model->eos < 0 || model->eos >= model->n_vocab || alignment == 0 ||
        model->n_embd > MAX_WIDTH || model->n_ff > MAX_WIDTH ||
        model->n_head > MAX_WIDTH / model->head_dim)
        die("%s does not describe a model this program can run", path);

    if (dir.count > (uint64_t)(reader.end - reader.next) / 24)
        die("the tensor directory is longer than the file");
    dir.entry = malloc((dir.count + 1) * sizeof *dir.entry);
    if (!dir.entry)
        die("out of memory");
    for (uint64_t i = 0; i < dir.count; i++)
        rd_entry(&reader, &dir.entry[i]);

    uint64_t header = (uint64_t)(reader.next - file);
    uint64_t start = (header + alignment - 1) / alignment * alignment;
    if (start > size)
        die("the file ends before its data area");
    dir.data = file + start;
    dir.end = file + size;

    find_tensors(model, &dir);
    free(dir.entry);
    tokenizer_init(model);
    return model;
}
