/* Lab 2: a set-associative cache between the CPU and physical memory.
 *
 * Every access of the machine to memory arrives here: the instruction
 * fetches, loads and stores of the interpreter of lab 1, and the buffers
 * that system calls read and write. The cache keeps
 * copies of 64-byte lines of DRAM. An access whose line has a copy in the cache is a hit and is
 * served from the copy. An access whose line has none is a miss: the
 * whole line first moves from DRAM into the cache.
 *
 * 1. The path of an access, and where each level keeps its bytes
 *
 *        interp.c (lab 1)          through frame/cpu/x86.c: x86_begin fetches,
 *                                  load, store and the like read and write
 *        frame/os/sysbase.c        system calls: guest_read and guest_write
 *                                  (frame/os/copy.c) copy the buffers of the
 *                                  program one line at a time
 *            |
 *            |  fetch_bytes, mem_read, mem_write at a virtual address
 *            v
 *        frame/cpu/mem.c           the memory access unit of the CPU:
 *                                  mmu_translate, virtual -> physical, then
 *                                  the access cut at the line boundaries
 *            |
 *            |  cache_read(pa, dst, nbytes)  cache_write(pa, src, nbytes)
 *            |  every call within one line
 *            v
 *     +-------------------------------+
 *     |  this file: the cache         |   the array cache_lines, in
 *     |  hits are served here         |   memory of the host
 *     +-------------------------------+
 *            |  miss:         dram_line_read    one whole line in
 *            |  dirty victim: dram_line_write   one whole line out
 *            v
 *     +-------------------------------+
 *     |  frame/cpu/dram.c: DRAM       |   the file dram.img: every line
 *     |                               |   moved bypasses the host's caches
 *     +-------------------------------+
 *
 *   A hit costs a few loads and stores of the host, which the caches of
 *   the host serve. A line moved between the cache and DRAM is first
 *   removed from the caches of the host (clflush on x86-64), so it costs
 *   an access to the DRAM of the host, which takes far longer, and the
 *   fewer lines move, the sooner a program ends.
 *   ./mini-cpu --cache 0 runs the same program without a cache: then every
 *   access is read or written in DRAM.
 *
 * 2. The three fields of a physical address
 *
 *   With the default --cache 64,4, set_count is 64:
 *
 *      63                            12 11           6 5            0
 *     +--------------------------------+--------------+--------------+
 *     |              tag               |  set index   |    offset    |
 *     +--------------------------------+--------------+--------------+
 *                  52 bits                  6 bits         6 bits
 *
 *     offset       = pa % LINE_SIZE                byte within the line
 *     line number  = pa / LINE_SIZE                which line of DRAM
 *     set index    = line number % cache_set_count the one set that may hold it
 *     tag          = line number / cache_set_count tells apart the lines of a set
 *
 *   Example, pa = 0x12345: offset 0x05, line number 0x48d, set index 0x0d
 *   (13), tag 0x12. In the other direction, the first byte of a cached line
 *   comes from its tag and its set:
 *
 *     (tag * cache_set_count + set index) * LINE_SIZE
 *     (0x12 * 64 + 13) * 64 = 0x12340
 *
 *   When the number of sets is a power of two, the remainder and the
 *   quotient are the bit fields of the drawing: set index = (pa >> 6) & 63,
 *   tag = pa >> 12 for 64 sets. Compute them with % and /, which hold for
 *   any number of sets (--cache 3,2 has 3 sets).
 *
 * 3. The cache: S = cache_set_count sets of W = cache_way_count lines each
 *
 *   A line of the cache, a cache_line:
 *
 *     +-------+-------+-----+----------+-------------------------+
 *     | valid | dirty | tag | last_use | data[0] .. data[63]     |
 *     +-------+-------+-----+----------+-------------------------+
 *
 *   The cache, every box one such line; the W lines of a row are the ways
 *   of one set (S = 64 and W = 4 by default: 256 lines, 16 KiB of data):
 *
 *                 way 0        way 1        way 2        way W-1
 *              +-----------+-----------+-----------+-----------+
 *     set 0    |   line    |   line    |   line    |   line    |
 *              +-----------+-----------+-----------+-----------+
 *     set 1    |   line    |   line    |   line    |   line    |
 *              +-----------+-----------+-----------+-----------+
 *       ...                         ...
 *              +-----------+-----------+-----------+-----------+
 *     set S-1  |   line    |   line    |   line    |   line    |
 *              +-----------+-----------+-----------+-----------+
 *
 *     valid      1: data is a copy of a line of DRAM; 0: the way is empty
 *     dirty      1: data was written after it came from DRAM, so DRAM holds
 *                older bytes and the line must go back before it is dropped
 *     tag        the tag of the addresses whose bytes data holds
 *     last_use   the value of cache_use_clock at the last access of the
 *                line: the smallest last_use of a set is its least
 *                recently used way
 *     data       the 64 bytes of the line
 *
 *   The S * W lines are one array, cache_lines, the rows one after another:
 *
 *     cache_lines   | set 0: way 0 .. W-1 | set 1: way 0 .. W-1 | ... | set S-1 |
 *
 *     way `way` of set `set_index` is cache_lines[set_index * cache_way_count + way]
 *
 *   A line of DRAM may sit in any way of its set and in no other set: a
 *   lookup compares the tag with that of every valid way of one set.
 *
 * 4. The rules
 *
 *   write-back        a write changes the copy in the cache and sets dirty;
 *                     DRAM receives the line only when it leaves the cache
 *                     (eviction) or at cache_flush
 *   write allocation  a write that misses first reads the whole line, as a
 *                     read that misses does, then changes its bytes
 *   LRU               a miss in a set without an empty way evicts the way
 *                     with the smallest last_use
 *   whole lines       below the cache use only dram_line_read and
 *                     dram_line_write; dram_ptr is not allowed here
 *                     (tests/srccheck.sh checks the names you use)
 *
 *   The counters cache_hits, cache_misses and cache_writebacks (defined
 *   in frame/cpu/dram.c) count lines: every call of cache_read and
 *   cache_write is one lookup of one line and counts once.
 *
 * 5. What you write
 *
 *   The data structure is given below, and so is the first function,
 *   cache_init: it builds the cache of the drawing in section 3, and it is
 *   written out as an example of how the code uses cache_lines,
 *   cache_set_count and cache_way_count. Write the other ten functions in
 *   the order of this file: each TODO says what the function returns, its
 *   steps, which counter changes and when, and its edge cases, and later
 *   functions call earlier ones.
 *
 *     cache_init                     the lines and the counters (given)
 *     cache_set_index, cache_tag     the fields of an address
 *     cache_line_address             the first address of a line, from tag and set
 *     cache_lookup                   the way of a set that holds a tag
 *     cache_victim                   the way a miss replaces
 *     cache_fill                     write the victim back, read the new line
 *     cache_access                   one line: hit or miss, counters, LRU
 *     cache_read, cache_write        an access within one line
 *     cache_flush                    every dirty line back to DRAM
 *
 *   cache_init, cache_read, cache_write and cache_flush are the interface
 *   declared in mcpu.h under "Lab 2:"; the framework calls nothing else
 *   here. Every other function and variable is static: only this file
 *   uses them, and their names cannot clash with those of the other labs.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mcpu.h"

/* Only this file calls the static functions below. Until cache_read and
   cache_write call them, the compiler warns that they are unused, and this
   line turns that warning off. */
#pragma GCC diagnostic ignored "-Wunused-function"

/* One line of the cache: one box of the drawing in section 3. */
typedef struct {
    int      valid;                 /* 1: data is a copy of a line of DRAM */
    int      dirty;                 /* 1: data was written after it was read */
    uint64_t tag;                   /* the tag of the addresses in the line */
    uint64_t last_use;              /* cache_use_clock at the last access */
    uint8_t  data[LINE_SIZE];       /* the 64 bytes */
} cache_line;

static cache_line *cache_lines;     /* way of set_index: [set_index * cache_way_count + way] */
static unsigned    cache_set_count; /* number of sets, S of --cache S,W */
static unsigned    cache_way_count; /* lines in a set, W of --cache S,W */
static uint64_t    cache_use_clock; /* number of line accesses so far */

void cache_init(unsigned sets, unsigned ways)
{
    /* Written out as an example: build an empty cache of sets * ways
     * lines, the cache of the drawing in section 3.
     *
     * The framework calls cache_init twice: before it loads the program
     * and again after, so that loading is not counted. It calls
     * cache_flush before the second call, so nothing is written back
     * here.
     */

    /* 1. The lines of the earlier call are no longer used: free them.
     *    free(NULL) does nothing, so the first call needs no special case. */
    free(cache_lines);

    /* 2. 0 for sets or for ways means 1: cache_init(0, 0) builds one line. */
    cache_set_count = sets ? sets : 1;
    cache_way_count = ways ? ways : 1;

    /* 3. All cache_set_count * cache_way_count lines in one array, set
     *    after set: way `way` of set `set_index` is
     *    cache_lines[set_index * cache_way_count + way]. calloc fills the
     *    lines with zeros, so every line starts invalid (valid = 0) and
     *    clean (dirty = 0). */
    cache_lines = calloc((size_t)cache_set_count * cache_way_count, sizeof *cache_lines);
    if (!cache_lines) {
        fprintf(stderr, "cache_init: no memory for %u x %u lines\n",
                cache_set_count, cache_way_count);
        exit(1);
    }

    /* 4. The counters of --stats and the clock of LRU start from zero. */
    cache_hits = cache_misses = cache_writebacks = 0;
    cache_use_clock = 0;
}

static unsigned cache_set_index(uint64_t pa)
{
    /* TODO: return the set index of physical address pa.
     *
     * The line number is pa / LINE_SIZE; the set index is the line number
     * modulo cache_set_count (section 2). Every byte of a line has the
     * same set index: 0x12340 and 0x1237f both give 13 with 64 sets.
     *
     * cache_init has set cache_set_count to 1 or more before any access,
     * so the division is defined.
     */
    (void)pa;
    todo("cache_set_index");
}

static uint64_t cache_tag(uint64_t pa)
{
    /* TODO: return the tag of physical address pa: the line number
     * pa / LINE_SIZE divided by cache_set_count.
     *
     * Together, the set index and the tag name the line: two addresses
     * with the same set index and the same tag lie in the same 64-byte
     * line. With 64 sets, 0x12345 has tag 0x12 and 0x13345 tag 0x13
     * (both have set index 13: they compete for the ways of set 13).
     */
    (void)pa;
    todo("cache_tag");
}

static uint64_t cache_line_address(uint64_t tag, unsigned set_index)
{
    /* TODO: return the physical address of the first byte of the line
     * with this tag in set set_index:
     *
     *     (tag * cache_set_count + set_index) * LINE_SIZE
     *
     * This undoes cache_set_index and cache_tag: for every pa,
     * cache_line_address(cache_tag(pa), cache_set_index(pa)) is pa with
     * its offset cleared. A line keeps only its tag; cache_fill and
     * cache_flush need this address to write a dirty line back to the
     * place in DRAM it came from, and cache_fill to read a new line.
     */
    (void)tag; (void)set_index;
    todo("cache_line_address");
}

static cache_line *cache_lookup(unsigned set_index, uint64_t tag)
{
    /* TODO: return the way of set set_index that holds the line with this
     * tag, or NULL if no way of the set holds it.
     *
     * The ways of the set are the cache_way_count lines
     *
     *     cache_lines[set_index * cache_way_count + way],  way = 0 .. cache_way_count - 1
     *
     * A way holds the line when it is valid and its tag equals tag. An
     * invalid way never matches, whatever its tag field contains: after
     * cache_init every tag is 0, and tag 0 is a real tag (the lines at the
     * lowest addresses have it).
     *
     * This function only searches: it changes no counter and no last_use.
     * cache_access decides what a hit or a miss does.
     */
    (void)set_index; (void)tag;
    todo("cache_lookup");
}

static cache_line *cache_victim(unsigned set_index)
{
    /* TODO: return the way of set set_index that a miss in this set
     * replaces.
     *
     *   1. If a way of the set is invalid (empty), return it; with several,
     *      return the one with the lowest way number. An empty way costs
     *      nothing to reuse.
     *   2. Otherwise every way holds a line: return the least recently
     *      used one, the way with the smallest last_use. Every access sets
     *      last_use to a new, larger value of cache_use_clock, so two
     *      valid ways never have the same last_use.
     *
     * Example, one set of 2 ways: reads of lines A, B, A, then a miss on
     * C. B has the smaller last_use (A was used after it), so C replaces
     * B.
     *
     * This function only chooses: the victim may be dirty, and writing it
     * back is the work of cache_fill. No counter changes here.
     */
    (void)set_index;
    todo("cache_victim");
}

static void cache_fill(cache_line *victim, unsigned set_index, uint64_t tag)
{
    /* TODO: the work of a miss. victim is the way cache_victim chose in
     * set set_index; afterwards it holds the line with this tag.
     *
     *   1. If victim is valid and dirty, its bytes are newer than DRAM:
     *      write them back first, with
     *          dram_line_write(cache_line_address(victim->tag, set_index), victim->data)
     *      and add 1 to cache_writebacks. A clean or invalid victim is
     *      dropped without a write: DRAM already holds its bytes, or it
     *      holds nothing.
     *   2. Read the new line from DRAM into the way:
     *          dram_line_read(cache_line_address(tag, set_index), victim->data)
     *   3. The way now holds that line: valid = 1, dirty = 0, tag = tag.
     *
     * Each dram_line_read and dram_line_write moves 64 bytes from or to
     * the DRAM of the host. Write-back saves the second transfer for
     * every line that was only read.
     *
     * cache_misses and last_use are left to cache_access.
     */
    (void)victim; (void)set_index; (void)tag;
    todo("cache_fill");
}

static cache_line *cache_access(uint64_t pa)
{
    /* TODO: find the line that holds physical address pa, bringing it in
     * on a miss, and return it. This is the one lookup that cache_read
     * and cache_write share; pa may be any byte of the line.
     *
     *   1. set_index = cache_set_index(pa), tag = cache_tag(pa).
     *   2. line = cache_lookup(set_index, tag).
     *   3. A hit (line is not NULL): add 1 to cache_hits.
     *      A miss: add 1 to cache_misses, take line = cache_victim(set_index)
     *      and cache_fill(line, set_index, tag).
     *   4. Either way the line is now the most recently used of its set:
     *      add 1 to cache_use_clock and store the new value in
     *      line->last_use.
     *   5. Return line.
     *
     * The caller copies bytes from or into line->data; a write also sets
     * line->dirty. Every call is one line access: a hit moves nothing, a
     * miss one or two lines.
     */
    (void)pa;
    todo("cache_access");
}

void cache_read(uint64_t pa, void *dst, unsigned nbytes)
{
    /* TODO: copy nbytes bytes at physical address pa to dst, through the
     * cache.
     *
     * The bytes lie within one line: pa % LINE_SIZE + nbytes is at most
     * LINE_SIZE, and nbytes is at least 1. The CPU never reads across a
     * line in one access: frame/cpu/mem.c cuts an access of an
     * instruction at the line boundaries, and 8 bytes at line offset 60
     * arrive here as two calls, 4 bytes at offset 60 of one line and 4
     * bytes at offset 0 of the next.
     *
     *   1. line = cache_access(pa).
     *   2. Copy nbytes bytes from line->data + pa % LINE_SIZE to dst.
     *
     * A call is one line access: one hit or one miss. Reading changes no
     * dirty bit.
     */
    (void)pa; (void)dst; (void)nbytes;
    todo("cache_read");
}

void cache_write(uint64_t pa, const void *src, unsigned nbytes)
{
    /* TODO: copy nbytes bytes from src to physical address pa, through the
     * cache: cache_read with the copy in the other direction. The bytes
     * lie within one line, as for cache_read.
     *
     *   1. line = cache_access(pa). On a miss this reads the whole line
     *      from DRAM first (write allocation): the write changes only
     *      nbytes of its 64 bytes, and the others must keep the values
     *      DRAM holds.
     *   2. Copy nbytes bytes from src to line->data + pa % LINE_SIZE and
     *      set line->dirty = 1. Nothing goes to DRAM now (write-back).
     */
    (void)pa; (void)src; (void)nbytes;
    todo("cache_write");
}

void cache_flush(void)
{
    /* TODO: write every dirty line back to DRAM.
     *
     * For every set_index and every way: if the line is valid and dirty,
     * dram_line_write(cache_line_address(line->tag, set_index), line->data),
     * add 1 to cache_writebacks, and clear dirty. The lines stay valid:
     * a later access still hits them. A clean line is not written.
     *
     * The framework calls cache_flush after loading and before the
     * program exits. Without it, what the program wrote last stays in the
     * cache and never reaches dram.img.
     */
    todo("cache_flush");
}
