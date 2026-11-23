/* tokenizer.c -- text to token numbers and back.

   libc.h stands in for the standard headers. print_token() ends with
   flush() of libc.c, one system call for a token.

   The vocabulary is byte-level BPE, as in GPT-2. The text of a token is
   written with one character per byte: a printable byte stands for itself
   and each of the other 68 bytes for one character from U+0100 on (a space
   is U+0120). A text is first cut into its bytes, each a token of its own;
   then the pair of neighbours that comes earliest in the list of merges is
   joined into one token, again and again until no pair is in the list.

   The tokenizer of the model first cuts the text into words with a regular
   expression and joins pairs inside one word only. That step is left out
   here, except for its rule that a digit stays a token of its own. Ordinary
   sentences come out the same; a run of spaces in front of a word may be
   cut in a different place. */
#include "libc.h"
#include "mini_ollama.h"

/* ---------------- a table from strings to their numbers ---------------- */

struct table {
    const struct str *key;
    int              *slot;             /* number + 1, or 0 for an empty slot */
    uint32_t          mask;
};

static uint32_t hash(const char *text, size_t len)
{
    uint32_t digest = 2166136261u;
    for (size_t pos = 0; pos < len; pos++)
        digest = (digest ^ (unsigned char)text[pos]) * 16777619u;
    return digest;
}

/* The number of the key equal to text[0..len), or -1. */
static int lookup(const struct table *table, const char *text, size_t len)
{
    for (uint32_t idx = hash(text, len) & table->mask; table->slot[idx];
         idx = (idx + 1) & table->mask) {
        const struct str *key = &table->key[table->slot[idx] - 1];
        if (key->len == len && memcmp(key->chars, text, len) == 0)
            return table->slot[idx] - 1;
    }
    return -1;
}

static struct table *index_of(const struct str *key, int count)
{
    struct table *table = malloc(sizeof *table);
    uint32_t size = 16;
    while (size < 2 * (uint32_t)count)
        size *= 2;
    if (!table || !(table->slot = calloc(size, sizeof *table->slot)))
        die("out of memory");
    table->key = key;
    table->mask = size - 1;
    for (int number = 0; number < count; number++) {
        uint32_t idx = hash(key[number].chars, key[number].len) & table->mask;
        while (table->slot[idx])
            idx = (idx + 1) & table->mask;
        table->slot[idx] = number + 1;
    }
    return table;
}

/* ---------------- bytes and their characters ---------------- */

enum { N_CHARS = 256 + 68 };

static int           byte_token[256];   /* the token of a single byte */
static unsigned char char_byte[N_CHARS];/* the byte a character stands for */

static int printable(int byte)
{
    return (byte >= 33 && byte <= 126) || (byte >= 161 && byte <= 172) ||
           byte >= 174;
}

void tokenizer_init(struct model *model)
{
    model->vocab_index = index_of(model->vocab, model->n_vocab);
    model->merge_index = index_of(model->merges, model->n_merges);

    for (int ch = 0; ch < 256; ch++)
        char_byte[ch] = (unsigned char)ch;
    for (int byte = 0, next = 256; byte < 256; byte++) {
        int ch = printable(byte) ? byte : next++;
        char utf8[2] = { (char)ch, 0 };
        if (ch >= 128) {                /* two bytes: 110xxxxx 10xxxxxx */
            utf8[0] = (char)(0xc0 | ch >> 6);
            utf8[1] = (char)(0x80 | (ch & 63));
        }
        char_byte[ch] = (unsigned char)byte;
        byte_token[byte] = lookup(model->vocab_index, utf8, ch < 128 ? 1 : 2);
        if (byte_token[byte] < 0)
            die("the vocabulary has no token for the byte %d", byte);
    }
}

/* ---------------- text to tokens ---------------- */

static int is_digit(const struct model *model, int tok)
{
    const struct str *text = &model->vocab[tok];
    return text->len == 1 && text->chars[0] >= '0' && text->chars[0] <= '9';
}

/* Where the pair (left, right) stands in the list of merges, or -1; *joined
   is the token the pair becomes. */
static int merge_rank(const struct model *model, int left, int right,
                      int *joined)
{
    const struct str *left_text = &model->vocab[left],
                     *right_text = &model->vocab[right];
    char buf[256];
    if (is_digit(model, left) || is_digit(model, right) ||
        left_text->len + right_text->len + 1 > sizeof buf)
        return -1;
    memcpy(buf, left_text->chars, left_text->len);
    buf[left_text->len] = ' ';
    memcpy(buf + left_text->len + 1, right_text->chars, right_text->len);
    int rank = lookup(model->merge_index, buf,
                      left_text->len + 1 + right_text->len);
    if (rank < 0)
        return -1;
    memmove(buf + left_text->len, buf + left_text->len + 1, right_text->len);
    *joined = lookup(model->vocab_index, buf, left_text->len + right_text->len);
    return *joined < 0 ? -1 : rank;
}

/* text[0..len) as tokens; tok has room for len of them. */
static int bpe(const struct model *model, const char *text, size_t len,
               int *tok)
{
    int count = (int)len;
    for (int pos = 0; pos < count; pos++)
        tok[pos] = byte_token[(unsigned char)text[pos]];

    for (;;) {
        int at = -1, best = 0, joined = 0;
        for (int pos = 0; pos + 1 < count; pos++) {
            int pair_token,
                rank = merge_rank(model, tok[pos], tok[pos + 1], &pair_token);
            if (rank >= 0 && (at < 0 || rank < best)) {
                at = pos;
                best = rank;
                joined = pair_token;
            }
        }
        if (at < 0)
            return count;
        tok[at] = joined;
        memmove(tok + at + 1, tok + at + 2,
                (size_t)(count - at - 2) * sizeof *tok);
        count--;
    }
}

/* The special token written at text, such as <|im_end|>, or -1. */
static int special(const struct model *model, const char *text, size_t *len)
{
    const char *close =
        text[0] == '<' && text[1] == '|' ? strstr(text + 2, "|>") : NULL;
    if (!close)
        return -1;
    *len = (size_t)(close + 2 - text);
    return lookup(model->vocab_index, text, *len);
}

/* The prompt, put into the chat format the model was trained on, as tokens.
   tok has room for MAX_CTX of them. */
int tokenize(const struct model *model, const char *prompt, int *tok)
{
    static const char template[] =
        "<|im_start|>user\n%s<|im_end|>\n<|im_start|>assistant\n";
    size_t size = strlen(prompt) + sizeof template, len = 0;
    char *text = malloc(size);
    int *piece = malloc(size * sizeof *piece);
    int count = 0;

    if (!text || !piece)
        die("out of memory");
    snprintf(text, size, template, prompt);

    /* the text between two special tokens is one piece for bpe() */
    for (const char *at = text, *from = text; ; at++) {
        int id = *at ? special(model, at, &len) : -1;
        if (*at && id < 0)
            continue;
        int piece_len = bpe(model, from, (size_t)(at - from), piece);
        if (count + piece_len + 1 >= MAX_CTX)
            die("the prompt is longer than %d tokens", MAX_CTX);
        memcpy(tok + count, piece, (size_t)piece_len * sizeof *tok);
        count += piece_len;
        if (!*at)
            break;
        tok[count++] = id;
        at += len - 1;
        from = at + 1;
    }
    free(text);
    free(piece);
    return count;
}

/* ---------------- tokens to text ---------------- */

/* Write the bytes the token stands for. A token may end in the middle of a
   UTF-8 character; the next token then brings the rest. */
void print_token(const struct model *model, int tok)
{
    const struct str *text = &model->vocab[tok];
    for (size_t pos = 0; pos < text->len; pos++) {
        int ch = (unsigned char)text->chars[pos];
        if (ch >= 0xc0 && ch < 0xe0 && pos + 1 < text->len)
            ch = (ch & 31) << 6 | ((unsigned char)text->chars[++pos] & 63);
        putchar(ch < N_CHARS ? char_byte[ch] : '?');
    }
    flush();
}
