/*
 * Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS),
 * Shanghai Jiao Tong University (SJTU) Licensed under the Mulan PSL v2. You can
 * use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 * NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE. See the
 * Mulan PSL v2 for more details.
 */
#include <arch/trustzone/tlogger.h>
#include <arch/mm/page_table.h>
#include <arch/mmu.h>
#include <arch/sync.h>
#include <mm/mm.h>
#include <mm/uaccess.h>
#include <common/util.h>
#include <object/thread.h>
#include <common/lock.h>

#define LOGGER_BASE     0xFFFFFD0000000000
#define TMP_LOGGER_SIZE (PAGE_SIZE << 6)

#define LOG_ITEM_MAGIC     0x5A5A
#define LOG_ITEM_LEN_ALIGN 64

#define LOG_ITEM_SIZE 384

const char *user_prefix = "                                 [ChCore-User]";
const char *kernel_prefix = "                                 [ChCore-Kernel]";

struct lock tlogger_lock;
volatile bool using_tlogger = false;
static struct log_buffer *logger = NULL;
extern unsigned long boot_ttbr1_l0[];

void enable_tlogger(void)
{
#ifdef CHCORE_OPENTRUSTEE_TLOGGER
        using_tlogger = true;
#else /* CHCORE_OPENTRUSTEE_TLOGGER */
        return;
#endif /* CHCORE_OPENTRUSTEE_TLOGGER */
}

bool is_tlogger_on(void)
{
#ifdef CHCORE_OPENTRUSTEE_TLOGGER
        return using_tlogger;
#else /* CHCORE_OPENTRUSTEE_TLOGGER */
        return 0;
#endif /* CHCORE_OPENTRUSTEE_TLOGGER */
}

int append_rdr_log(const void *buf, size_t len)
{
#ifdef CHCORE_OPENTRUSTEE_TLOGGER
        int ret = 0;

        lock(&tlogger_lock);
        if (!using_tlogger || !logger) {
                goto out_unlock;
        }

        if (logger->flag.last_pos + len >= logger->flag.max_len) {
                logger->flag.last_pos = 0;
                logger->flag.write_loops++;
        }
        memcpy(logger->buffer_start + logger->flag.last_pos, buf, len);
        logger->flag.last_pos += len;

out_unlock:
        unlock(&tlogger_lock);
        return ret;
#else /* CHCORE_OPENTRUSTEE_TLOGGER */
        return 0;
#endif /* CHCORE_OPENTRUSTEE_TLOGGER */
}

#define LARGE_MSG_PREFIX_SIZE 50
const char ellipsis[] = "...\n";

static void __append_large_log(const char *str, size_t len, bool is_kernel)
{
        struct {
                char log[LARGE_MSG_PREFIX_SIZE];
                char log_ellipsis[sizeof(ellipsis)];
        } buf;

        if (len <= LARGE_MSG_PREFIX_SIZE) {
                (void)append_chcore_log(str, len, is_kernel);
                return;
        }

        memcpy(buf.log, str, LARGE_MSG_PREFIX_SIZE);
        memcpy(buf.log_ellipsis, ellipsis, sizeof(buf.log_ellipsis));
        (void)append_chcore_log(&buf, sizeof(buf), is_kernel);
}

static inline bool __check_empty_char(char c)
{
        return c == '\r' || c == '\0' || c == '\n';
}

int append_chcore_log(const char *str, size_t len, bool is_kernel)
{
#ifdef CHCORE_OPENTRUSTEE_TLOGGER
        int ret, count, i;
        struct log_item *item;
        size_t item_size;
        char *ptr;
        char log_item_buf[LOG_ITEM_SIZE];
        const char *prefix;
        bool empty = true;

        for (i = 0; i < len; i++) {
                if (!__check_empty_char(str[i])) {
                        empty = false;
                }
        }
        if (empty)
                return 0;

        if (len >= LOG_ITEM_SIZE) {
                __append_large_log(str, len, is_kernel);
                return 0;
        }

        if (is_kernel) {
                prefix = kernel_prefix;
        } else {
                prefix = user_prefix;
        }

        /* len < LOG_ITEM_SIZE, no integer overflow here. */
        /* log_item: `log_item_header`"[`prefix`]`str`\n" */
        item_size = sizeof(struct log_item) + strlen(prefix) + len + 2;
        if (current_thread && current_cap_group) {
                /* prefix: "[`user_prefix`][`cap_group_name`]" */
                item_size += 1 + strlen(current_cap_group->cap_group_name) + 1;
        }

        if (item_size >= LOG_ITEM_SIZE) {
                __append_large_log(str, len, is_kernel);
                return 0;
        }
        memset(log_item_buf, 0, sizeof(log_item_buf));
        item = (struct log_item *)log_item_buf;

        ptr = (char *)item->log_buffer;
        if (current_thread && current_cap_group) {
                count = simple_sprintf(ptr,
                                       "%s[%s]",
                                       prefix,
                                       current_cap_group->cap_group_name);
        } else {
                count = simple_sprintf(ptr, "%s", prefix);
        }
        ptr += count;
        memcpy(ptr, str, len);
        while (__check_empty_char(ptr[len - 1])) {
                ptr[--len] = 0;
        }
        ptr[len++] = '\n';

        item->magic = LOG_ITEM_MAGIC;
        item->real_len = count + len;
        item->buffer_len = (item->real_len + LOG_ITEM_LEN_ALIGN - 1)
                           / LOG_ITEM_LEN_ALIGN * LOG_ITEM_LEN_ALIGN;
        item->log_level = 2;

        ret = append_rdr_log(item, sizeof(struct log_item) + item->buffer_len);

        return ret;
#else /* CHCORE_OPENTRUSTEE_TLOGGER */
        return 0;
#endif /* CHCORE_OPENTRUSTEE_TLOGGER */
}

int tmp_tlogger_init(void)
{
#ifdef CHCORE_OPENTRUSTEE_TLOGGER
        logger = kmalloc(TMP_LOGGER_SIZE);
        if (logger == NULL) {
                return -ENOMEM;
        }
        logger->flag.last_pos = 0;
        logger->flag.write_loops = 0;
        logger->flag.max_len = TMP_LOGGER_SIZE - sizeof(struct log_buffer);
        lock_init(&tlogger_lock);
        return 0;
#else /* CHCORE_OPENTRUSTEE_TLOGGER */
        return 0;
#endif /* CHCORE_OPENTRUSTEE_TLOGGER */
}

int ot_sys_tee_push_rdr_update_addr(paddr_t addr, size_t size, bool is_cache_mem,
                                 char *chip_type_buff, size_t buff_len)
{
#ifdef CHCORE_OPENTRUSTEE_TLOGGER
        int ret;
        struct log_buffer *tmp_logger;
        size_t copy_len;

        size = ROUND_UP(size, PAGE_SIZE);
        ret = map_range_in_pgtbl_kernel(
                (void *)((unsigned long)boot_ttbr1_l0 + KBASE),
                LOGGER_BASE,
                addr,
                size,
                VMR_READ | VMR_WRITE | VMR_TZ_NS);
        flush_tlb_all();
        if (ret) {
                goto out;
        }
        tmp_logger = logger;
        logger = (void *)LOGGER_BASE;
        logger->flag.last_pos = tmp_logger->flag.last_pos;
        logger->flag.write_loops = tmp_logger->flag.write_loops;
        logger->flag.max_len = size - sizeof(struct log_buffer);
        copy_len = MIN(tmp_logger->flag.max_len, logger->flag.max_len);
        memcpy(logger->buffer_start, tmp_logger->buffer_start, copy_len);
        kfree(tmp_logger);

out:
        return ret;
#else /* CHCORE_OPENTRUSTEE_TLOGGER */
        return 0;
#endif /* CHCORE_OPENTRUSTEE_TLOGGER */
}

int ot_sys_debug_rdr_logitem(char *str, size_t str_len)
{
#ifdef CHCORE_OPENTRUSTEE_TLOGGER
        int ret = 0;
        char *kbuf;
        if (check_user_addr_range((vaddr_t)str, str_len)) {
                return -EINVAL;
        }
        kbuf = kmalloc(str_len);
        if (kbuf == NULL) {
                return -ENOMEM;
        }
        ret = copy_from_user(kbuf, str, str_len);
        if (ret) {
                goto out;
        }

        ret = append_rdr_log(kbuf, str_len);

out:
        kfree(kbuf);
        return ret;
#else /* CHCORE_OPENTRUSTEE_TLOGGER */
        return 0;
#endif /* CHCORE_OPENTRUSTEE_TLOGGER */
}
