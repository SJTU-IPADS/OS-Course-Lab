/* mini-ollama: answers the turns of one conversation with a quantized
   model, and keeps the keys and values of its tokens in a KV cache: one
   block per token, in one piece of memory of limited size.

     make
     ./mini-ollama [OPTIONS] MODEL.gguf WORKLOAD.txt

   MODEL.gguf is q4_0.gguf (README.md): the language model of
   Qwen3-VL-2B-Instruct, every matrix in Q4_0. WORKLOAD.txt holds one turn
   per line, UTF-8: the message of the user. Empty lines and lines that
   start with # are skipped. The turns make one conversation: each turn
   continues the text of the turns before it.

   OPTIONS
     --kv-limit SIZE        the memory of the KV cache, in bytes, or with the
                            suffix K, M or G (default 32M); it holds as many
                            whole blocks as fit in it, one per token
     --max-new N            answer tokens of a turn, at most (default 8)
     --no-cache             keep no keys and values: every step computes the
                            keys and values of all earlier tokens again, from
                            their vectors at the input of each layer; the
                            answers are the same bytes as with the cache
     --trace FILE           write to FILE the blocks of the cache and, for
                            every turn, the arguments of kv_reserve and the
                            capacity it left, for tools/kvmodel.py

   The answers go to standard output; the numbers of each turn and of the
   whole run go to standard error.

   A turn appends the message, in the chat format of the model, to the text
   of the conversation, computes the tokens of the text that are not in the
   cache yet, and generates the answer token by token (serve). With the KV
   cache each token is taken through the model by compute(), given in
   src/kvcache.c as an example: it calls embed(), attention() and
   feed_forward() of this file (kvcache.h), and the functions kv_key(),
   kv_value() and kv_append() that you write. How many blocks a turn
   reserves is decided in src/kvcache.c as well, by kv_reserve(). --no-cache
   has a path of its own, compute_no_cache(), allocates no memory for the
   cache and calls nothing of src/kvcache.c. The next token is the one with
   the largest score, which is what ollama does at temperature 0. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "kvcache.h"
#include "mini_ollama.h"
#include "ops.h"

const char *program_name = "mini-ollama";

static struct {
    size_t      kv_limit;
    int         max_new;
    int         no_cache;
    const char *trace;
} options = { (size_t)32 << 20, 8, 0, NULL };

static struct model   *model;
static struct kv_cache cache;
static size_t          block_bytes;     /* with the KV cache: kv_block_bytes() of it */
static int             peak;            /* with the KV cache: the largest capacity so far */
static FILE           *trace;

/* The conversation as the framework sees it: its text as tokens. The keys
   and values of its tokens are in the KV cache. */
static struct {
    int    tokens[KV_MAX_TOKENS];
    int    n_tokens;                    /* the text so far: messages and answers */
    int    n_computed;                  /* tokens taken through the model */
    float *inputs;                      /* --no-cache: the state at the input of every layer,
                                           n_layer * n_embd floats per token */
    int    inputs_room;                 /* tokens inputs has room for */
} conv;

/* Tokens computed, in one turn and in the whole run. */
struct counts {
    long computed;                      /* tokens taken through the model */
    long kv_recomputed;                 /* --no-cache: keys and values of earlier
                                           tokens computed again */
};
static struct counts turn_counts, total;

/* The vectors of one pass through the model. */
static struct {
    float *state;               /* n_embd: the vector of the token */
    float *norm;                /* n_embd: the state normalized */
    float *query, *heads;       /* n_head * head_dim: the queries, what the heads take */
    float *score;               /* n_head * KV_MAX_TOKENS: row head, the scores of that head */
    float *out;                 /* n_embd: what a layer adds to state */
    float *gate, *up;           /* n_ff */
    float *logits;              /* n_vocab: one score per token */
} act;
static size_t act_bytes;

/* --no-cache: the keys and values of one layer, KV_MAX_TOKENS rows each,
   computed again at every step, and the addresses of these rows. */
static struct {
    float *keys[KV_MAX_TOKENS], *values[KV_MAX_TOKENS];
} scratch;
static size_t scratch_bytes;

static size_t kv_dim(void)
{
    return (size_t)model->n_head_kv * model->head_dim;
}

/* ---------------- memory and weights ---------------- */

static float *alloc_floats(size_t count, size_t *bytes)
{
    float *floats = calloc(count ? count : 1, sizeof *floats);
    if (!floats)
        die("out of memory");
    *bytes += count * sizeof *floats;
    return floats;
}

/* The bytes of a tensor in the file: a matrix is rows * cols / 32 blocks
   of 18 bytes, a vector cols floats. */
static size_t tensor_bytes(const struct tensor *tensor)
{
    return tensor->rows > 1 ? tensor->rows * (tensor->cols / Q4_0_BLOCK_WEIGHTS) * Q4_0_BLOCK_BYTES
                            : tensor->cols * sizeof(float);
}

static size_t weight_bytes(void)
{
    size_t bytes = tensor_bytes(&model->token_embd) + tensor_bytes(&model->output_norm);
    for (int layer = 0; layer < model->n_layer; layer++) {
        const struct layer *weights = &model->layer[layer];
        const struct tensor *tensors[] = {
            &weights->attn_norm, &weights->attn_q, &weights->attn_k, &weights->attn_v,
            &weights->attn_output, &weights->attn_q_norm, &weights->attn_k_norm,
            &weights->ffn_norm, &weights->ffn_gate, &weights->ffn_up, &weights->ffn_down };
        for (size_t i = 0; i < sizeof tensors / sizeof tensors[0]; i++)
            bytes += tensor_bytes(tensors[i]);
    }
    return bytes;
}

static void place_vectors(void)
{
    size_t q_dim = (size_t)model->n_head * model->head_dim;

    act.state  = alloc_floats(model->n_embd, &act_bytes);
    act.norm   = alloc_floats(model->n_embd, &act_bytes);
    act.query  = alloc_floats(q_dim, &act_bytes);
    act.heads  = alloc_floats(q_dim, &act_bytes);
    act.score  = alloc_floats((size_t)model->n_head * KV_MAX_TOKENS, &act_bytes);
    act.out    = alloc_floats(model->n_embd, &act_bytes);
    act.gate   = alloc_floats(model->n_ff, &act_bytes);
    act.up     = alloc_floats(model->n_ff, &act_bytes);
    act.logits = alloc_floats(model->n_vocab, &act_bytes);
    if (options.no_cache) {
        float *key_rows   = alloc_floats(KV_MAX_TOKENS * kv_dim(), &scratch_bytes);
        float *value_rows = alloc_floats(KV_MAX_TOKENS * kv_dim(), &scratch_bytes);
        for (int row = 0; row < KV_MAX_TOKENS; row++) {
            scratch.keys[row]   = key_rows + (size_t)row * kv_dim();
            scratch.values[row] = value_rows + (size_t)row * kv_dim();
        }
    }
}

/* ---------------- one token through the model ---------------- */

/* out = the matrix times in. */
static void matvec(float *out, const struct tensor *matrix, const float *in)
{
    matvec_q4_0(out, matrix->data, in, matrix->rows, matrix->cols);
}

/* The floats of a vector tensor: the gains of a normalization. */
static const float *gain(const struct tensor *vector)
{
    return (const float *)vector->data;
}

/* state = the row of the token in the embedding matrix. A row of cols weights
   is cols / 32 blocks of 18 bytes. */
void embed(int token, float *state)
{
    const struct tensor *matrix = &model->token_embd;
    size_t row_bytes = matrix->cols / Q4_0_BLOCK_WEIGHTS * Q4_0_BLOCK_BYTES;
    embed_q4_0(state, matrix->data + (size_t)token * row_bytes, matrix->cols);
}

/* The query of the token at position pos, from state, its vector at the input
   of the layer: n_head vectors of head_dim floats. */
static void query(int layer, int pos, const float *state, float *query_out)
{
    const struct layer *weights = &model->layer[layer];

    rmsnorm(act.norm, state, gain(&weights->attn_norm), 1, model->n_embd, model->eps);
    matvec(query_out, &weights->attn_q, act.norm);
    rmsnorm(query_out, query_out, gain(&weights->attn_q_norm), model->n_head, model->head_dim, model->eps);
    rope(query_out, model->n_head, model->head_dim, pos, model->rope_base);
}

/* The key and the value of the token at position pos, from state, its vector at
   the input of the layer: one row of kv_dim floats each. */
static void key_value(int layer, int pos, const float *state, float *key, float *value)
{
    const struct layer *weights = &model->layer[layer];

    rmsnorm(act.norm, state, gain(&weights->attn_norm), 1, model->n_embd, model->eps);
    matvec(key, &weights->attn_k, act.norm);
    matvec(value, &weights->attn_v, act.norm);
    rmsnorm(key, key, gain(&weights->attn_k_norm), model->n_head_kv, model->head_dim, model->eps);
    rope(key, model->n_head_kv, model->head_dim, pos, model->rope_base);
}

/* With the KV cache: stops the program unless the key and the value of each
   token 0 .. pos lie inside blocks 0 .. capacity - 1 of the cache, so that a
   wrong address from kv_key() or kv_value() is reported before anything is
   written there. */
static void check_rows(int layer, int pos, float *const *keys, float *const *values)
{
    uintptr_t start = (uintptr_t)cache.base;
    size_t span = (size_t)cache.capacity * block_bytes, row_bytes = kv_dim() * sizeof(float);

    for (int tok = 0; tok <= pos; tok++)
        for (int kind = 0; kind < 2; kind++) {
            uintptr_t row = (uintptr_t)(kind ? values[tok] : keys[tok]);
            if (row < start || row - start > span || span - (row - start) < row_bytes ||
                (row - start) % sizeof(float) != 0)
                die("layer %d, token %d: kv_%s gave an address that is not inside blocks 0 .. %d "
                    "(capacity - 1) of the KV cache, %zu bytes each from base", layer, tok,
                    kind ? "value" : "key", cache.capacity - 1, block_bytes);
        }
}

/* state += what the token at position pos takes from the tokens 0 .. pos of
   the conversation.

   keys[tok] and values[tok] are the addresses of the key and the value of
   token tok in this layer. The new token's key and value go where keys[pos]
   and values[pos] point. Its query is compared with the keys of the tokens
   0 .. pos, and their values are added up, each weighted by how well its
   key matches the query. The rows of the tokens 0 .. pos - 1 are only read
   here: with the KV cache they were written when their tokens were
   computed, in this turn or in an earlier one. */
void attention(int layer, int pos, float *state, float *const *keys, float *const *values)
{
    const struct layer *weights = &model->layer[layer];
    int n_head = model->n_head, n_head_kv = model->n_head_kv, head_dim = model->head_dim;

    if (!options.no_cache)
        check_rows(layer, pos, keys, values);
    query(layer, pos, state, act.query);                    /* the new token's query */
    key_value(layer, pos, state, keys[pos], values[pos]);   /* its key and value, into its rows */

    attn_scores(act.score, act.query, keys, n_head, n_head_kv, head_dim, pos + 1);
    softmax(act.score, n_head, pos + 1);                    /* tokens 0 .. pos */
    attn_values(act.heads, act.score, values, n_head, n_head_kv, head_dim, pos + 1);
    matvec(act.out, &weights->attn_output, act.heads);
    add(state, act.out, model->n_embd);
}

/* state += W_down (silu(W_gate state) * W_up state), with
   silu(val) = val / (1 + e^-val). */
void feed_forward(int layer, float *state)
{
    const struct layer *weights = &model->layer[layer];

    rmsnorm(act.norm, state, gain(&weights->ffn_norm), 1, model->n_embd, model->eps);
    matvec(act.gate, &weights->ffn_gate, act.norm);
    matvec(act.up, &weights->ffn_up, act.norm);
    swiglu(act.gate, act.up, model->n_ff);
    matvec(act.out, &weights->ffn_down, act.gate);
    add(state, act.out, model->n_embd);
}

/* The token whose row of the embedding matrix has the largest product with
   state: the embedding matrix is also the output layer of this model. */
static int next_token(const float *state)
{
    rmsnorm(act.norm, state, gain(&model->output_norm), 1, model->n_embd, model->eps);
    matvec(act.logits, &model->token_embd, act.norm);
    return argmax(act.logits, model->n_vocab);
}

/* --no-cache: state at the input of the layer for the token at position pos. */
static float *layer_input(int pos, int layer)
{
    return conv.inputs + ((size_t)pos * model->n_layer + layer) * model->n_embd;
}

/* --no-cache, the step without a cache: keep state as the input of the layer for
   the token at position pos, then compute the keys and values of tokens
   0 .. pos - 1 again, from their inputs of this layer. */
static void recompute_earlier(int layer, int pos)
{
    memcpy(layer_input(pos, layer), act.state, (size_t)model->n_embd * sizeof(float));
    for (int tok = 0; tok < pos; tok++)
        key_value(layer, tok, layer_input(tok, layer), scratch.keys[tok], scratch.values[tok]);
}

/* --no-cache: room for the inputs of n_tokens tokens. compute_no_cache()
   asks for the tokens 0 .. pos, one token more at each step, so the inputs
   take the memory of the computed tokens alone, as the KV cache does. */
static void inputs_room(int n_tokens)
{
    if (n_tokens <= conv.inputs_room)
        return;
    size_t floats = (size_t)n_tokens * model->n_layer * model->n_embd;
    float *inputs = realloc(conv.inputs, floats * sizeof *inputs);
    if (!inputs)
        die("out of memory");
    conv.inputs = inputs;
    conv.inputs_room = n_tokens;
}

/* --no-cache, the reference of make test: the token at position pos, with
   the rows of the earlier tokens computed again into scratch. */
static void compute_no_cache(int pos)
{
    inputs_room(pos + 1);
    embed(conv.tokens[pos], act.state);
    for (int layer = 0; layer < model->n_layer; layer++) {
        recompute_earlier(layer, pos);
        attention(layer, pos, act.state, scratch.keys, scratch.values);
        feed_forward(layer, act.state);
    }
}

/* The token at position pos of the conversation in either mode; act.state
   is then its state after the last layer. With the KV cache compute(),
   given in src/kvcache.c, does the work, and the checks around the call
   find mistakes of kv_reserve() and kv_append() before they become wrong
   answers. */
static void compute_token(int pos)
{
    if (options.no_cache) {
        compute_no_cache(pos);
        turn_counts.kv_recomputed += pos;
    } else {
        if (pos != cache.n_cached || pos >= cache.capacity)
            die("token %d is next, but %d tokens are cached and %d blocks reserved (kv_reserve has to "
                "reserve a block for every token that compute caches in the turn)",
                pos, cache.n_cached, cache.capacity);
        compute(&cache, conv.tokens[pos], act.state);
        if (cache.n_cached != pos + 1)
            die("after compute of token %d, %d tokens are cached, not %d: kv_append, which compute "
                "calls once after the last layer, has to add 1 to n_cached", pos, cache.n_cached, pos + 1);
    }
    turn_counts.computed++;
    conv.n_computed = pos + 1;
}

/* ---------------- sizes and times ---------------- */

static void print_size(FILE *out, size_t bytes)
{
    if (bytes >= (size_t)1 << 20)
        fprintf(out, "%.2f MiB", bytes / 1048576.0);
    else
        fprintf(out, "%.2f KiB", bytes / 1024.0);
}

/* SIZE of --kv-limit: bytes, or with the suffix K, M or G (powers of 1024). */
static size_t parse_size(const char *text)
{
    char *end;
    unsigned long long value = strtoull(text, &end, 10);
    int shift = 0;
    if (*end == 'K' || *end == 'k')
        shift = 10;
    else if (*end == 'M' || *end == 'm')
        shift = 20;
    else if (*end == 'G' || *end == 'g')
        shift = 30;
    if (end == text || (shift && *++end) || (!shift && *end))
        die("--kv-limit %s: give a number of bytes, with K, M or G after it if you like", text);
    return (size_t)(value << shift);
}

static double now(void)
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/* ---------------- a turn ---------------- */

/* The message in the chat format of the model, appended to the text of the
   conversation. From the second turn on, <|im_end|> first closes the answer
   before it. One token of room is left for the answer. */
static void append_message(const char *message)
{
    static const char format[] = "%s<|im_start|>user\n%s<|im_end|>\n<|im_start|>assistant\n";
    size_t size = strlen(message) + sizeof format + 16;
    char *text = malloc(size);
    if (!text)
        die("out of memory");
    snprintf(text, size, format, conv.n_tokens > 0 ? "<|im_end|>\n" : "", message);

    int count = tokenize(model, text, conv.tokens + conv.n_tokens, KV_MAX_TOKENS - 1 - conv.n_tokens);
    free(text);
    if (count < 0)
        die("the text of the conversation would be longer than %d tokens", KV_MAX_TOKENS - 1);
    conv.n_tokens += count;
}

/* Has kv_reserve() make room for the turn, and writes what it got and left
   to the trace. When kv_reserve() returns 0 the program stops: the turn does
   not fit. The checks after the call, and those around compute(), find
   mistakes of the functions you write before they become wrong answers. */
static void reserve(int n_prompt, int max_answer, int turn)
{
    struct kv_cache before = cache;
    int room = kv_reserve(&cache, n_prompt, max_answer);

    if (trace)
        fprintf(trace, "turn %d n_prompt %d max_answer %d capacity %d%s\n", turn, n_prompt, max_answer,
                cache.capacity, room ? "" : " refused");
    if (cache.n_layer != before.n_layer || cache.kv_dim != before.kv_dim || cache.base != before.base ||
        cache.n_blocks != before.n_blocks || cache.n_cached != before.n_cached)
        die("kv_reserve(n_prompt %d, max_answer %d) changed a field other than capacity: n_cached went "
            "from %d to %d; n_layer, kv_dim, base and n_blocks belong to the framework", n_prompt,
            max_answer, before.n_cached, cache.n_cached);
    if (!room) {
        if (cache.capacity != before.capacity)
            die("kv_reserve(n_prompt %d, max_answer %d) returned 0, but changed the capacity from %d to "
                "%d blocks: a turn that does not fit changes nothing", n_prompt, max_answer,
                before.capacity, cache.capacity);
        die("turn %d does not fit: kv_reserve(n_prompt %d, max_answer %d) returned 0 at a capacity of "
            "%d blocks; the KV cache has %d blocks under the limit of %zu bytes; the program stops "
            "here (give a larger --kv-limit)", turn, n_prompt, max_answer, cache.capacity,
            cache.n_blocks, options.kv_limit);
    }
    if (cache.capacity < before.capacity || cache.capacity < n_prompt || cache.capacity > cache.n_blocks)
        die("kv_reserve(n_prompt %d, max_answer %d) returned 1 and changed the capacity from %d to %d "
            "blocks: it only grows, to n_prompt blocks at least and to the %d blocks of the KV cache "
            "at most", n_prompt, max_answer, before.capacity, cache.capacity, cache.n_blocks);
    if (cache.capacity > peak)
        peak = cache.capacity;
}

/* The numbers of the turn, on standard error, and into the totals. */
static void count_turn(int turn)
{
    fprintf(stderr, "    turn %d: %ld tokens computed, ", turn, turn_counts.computed);
    if (options.no_cache) {
        fprintf(stderr, "keys and values of %ld earlier tokens computed again\n",
                turn_counts.kv_recomputed);
    } else {
        fprintf(stderr, "%d cached; %d of %d blocks reserved, ", cache.n_cached, cache.capacity,
                cache.n_blocks);
        print_size(stderr, (size_t)cache.capacity * block_bytes);
        fprintf(stderr, "\n");
    }
    total.computed += turn_counts.computed;
    total.kv_recomputed += turn_counts.kv_recomputed;
}

/* One turn: the message joins the text, the tokens of the text that are
   not computed yet go through the model one after another, each by one
   compute_token(), and the answer is generated until it has max_answer
   tokens or the model gives its end token. The next turn goes on from the
   first token that is not computed. */
static void serve(const char *message, int turn)
{
    memset(&turn_counts, 0, sizeof turn_counts);
    printf("%s[turn %d] %s\n", turn > 1 ? "\n" : "", turn, message);
    fflush(stdout);

    append_message(message);
    int n_prompt = conv.n_tokens;                   /* the text with this message */
    int max_answer = options.max_new < KV_MAX_TOKENS - n_prompt ? options.max_new
                                                                : KV_MAX_TOKENS - n_prompt;

    if (!options.no_cache)
        reserve(n_prompt, max_answer, turn);
    for (int pos = conv.n_computed; pos < n_prompt; pos++)
        compute_token(pos);

    for (int n_answer = 0; n_answer < max_answer; ) {
        int token = next_token(act.state);
        if (token == model->eos)
            break;
        print_token(model, token);
        conv.tokens[conv.n_tokens++] = token;
        if (++n_answer < max_answer)
            compute_token(conv.n_tokens - 1);
    }
    printf("\n");
    fflush(stdout);
    count_turn(turn);
}

/* ---------------- the workload ---------------- */

/* The whole file, with a 0 after it. A file keeps the messages in UTF-8 on
   every system; a command-line argument does not on Windows. */
static char *read_file(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (!file)
        die("cannot open %s", path);
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    rewind(file);
    char *text = length >= 0 ? malloc((size_t)length + 1) : NULL;
    if (!text || fread(text, 1, (size_t)length, file) != (size_t)length)
        die("cannot read %s", path);
    fclose(file);
    text[length] = 0;
    return text;
}

/* Every line of the workload is one turn: the message of the user. */
static int serve_workload(char *text)
{
    int turn = 0;

    for (char *line = text; *line; ) {
        char *end = line + strcspn(line, "\n"), *next = *end ? end + 1 : end;
        *end = 0;
        if (end > line && end[-1] == '\r')
            *--end = 0;
        if (*line && *line != '#')
            serve(line, ++turn);
        line = next;
    }
    return turn;
}

/* ---------------- the whole run ---------------- */

static void usage(void)
{
    die("usage: %s [--kv-limit SIZE] [--max-new N] [--no-cache] [--trace FILE] MODEL.gguf WORKLOAD.txt",
        program_name);
}

static void parse_options(int argc, char **argv, const char **model_path, const char **workload_path)
{
    int arg = 1;
    for (; arg < argc && strncmp(argv[arg], "--", 2) == 0; arg++) {
        const char *name = argv[arg], *value = arg + 1 < argc ? argv[arg + 1] : NULL;
        if (strcmp(name, "--no-cache") == 0) {
            options.no_cache = 1;
            continue;
        }
        if (!value)
            usage();
        arg++;
        if (strcmp(name, "--kv-limit") == 0)
            options.kv_limit = parse_size(value);
        else if (strcmp(name, "--max-new") == 0 && atoi(value) > 0)
            options.max_new = atoi(value);
        else if (strcmp(name, "--trace") == 0)
            options.trace = value;
        else
            usage();
    }
    if (argc - arg != 2)
        usage();
    if (options.no_cache && options.trace)
        die("--trace records the KV cache, which --no-cache does not use");
    *model_path = argv[arg];
    *workload_path = argv[arg + 1];
}

static void report(int turns, double seconds)
{
    fprintf(stderr, "\n");
    fprintf(stderr, "turns:            %d\n", turns);
    if (options.no_cache) {
        size_t inputs = (size_t)conv.inputs_room * model->n_layer * model->n_embd * sizeof(float);
        fprintf(stderr, "tokens computed:  %ld; keys and values of %ld earlier tokens computed again\n",
                total.computed, total.kv_recomputed);
        fprintf(stderr, "KV cache:         none (--no-cache)\n");
        fprintf(stderr, "layer inputs:     ");
        print_size(stderr, inputs);
        fprintf(stderr, ", kept to compute keys and values again\n");
        fprintf(stderr, "rows of a layer:  ");
        print_size(stderr, scratch_bytes);
        fprintf(stderr, ", the keys and values computed again at each step\n");
    } else {
        fprintf(stderr, "tokens computed:  %ld\n", total.computed);
        fprintf(stderr, "KV cache:         peak %d blocks, ", peak);
        print_size(stderr, (size_t)peak * block_bytes);
        fprintf(stderr, "; limit ");
        print_size(stderr, options.kv_limit);
        fprintf(stderr, ", %d blocks of %zu bytes, one block per token\n", cache.n_blocks, block_bytes);
    }
    fprintf(stderr, "weights:          ");
    print_size(stderr, weight_bytes());
    fprintf(stderr, ", in the model file of ");
    print_size(stderr, model->file_bytes);
    fprintf(stderr, " read into memory\n");
    fprintf(stderr, "activations:      ");
    print_size(stderr, act_bytes);
    fprintf(stderr, "\n");
    fprintf(stderr, "time:             %.1f s\n", seconds);
}

int main(int argc, char **argv)
{
    const char *model_path, *workload_path;

    parse_options(argc, argv, &model_path, &workload_path);
    model = load_model(model_path);
    char *workload = read_file(workload_path);
    place_vectors();

    cache.n_layer = model->n_layer;
    cache.kv_dim = (int)kv_dim();
    if (options.no_cache) {
        fprintf(stderr, "mode: --no-cache, %d answer tokens per turn at most\n", options.max_new);
    } else {
        block_bytes = kv_block_bytes(&cache);
        kv_memory_init(&cache, block_bytes, options.kv_limit);
        fprintf(stderr, "mode: KV cache, limit ");
        print_size(stderr, options.kv_limit);
        fprintf(stderr, ", %d blocks of %zu bytes, %d answer tokens per turn at most\n",
                cache.n_blocks, block_bytes, options.max_new);
    }
    if (options.trace) {
        if (!(trace = fopen(options.trace, "wb")))
            die("cannot write %s", options.trace);
        fprintf(trace, "blocks %d\n", cache.n_blocks);
    }

    double start = now();
    int turns = serve_workload(workload);
    double seconds = now() - start;
    free(workload);
    report(turns, seconds);

    if (trace) {
        fprintf(trace, "peak %d\n", peak);
        fclose(trace);
    }
    if (!options.no_cache)
        kv_memory_free(&cache);
    return 0;
}
