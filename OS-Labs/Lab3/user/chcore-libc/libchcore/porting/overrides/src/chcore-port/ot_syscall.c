#ifdef CHCORE_OPENTRUSTEE

#include <chcore/syscall.h>
#include <uapi/memory.h>
#include <uapi/opentrustee/syscall_num.h>

int usys_tee_push_rdr_update_addr(paddr_t addr, size_t size, bool is_cache_mem,
                                  char *chip_type_buff, size_t buff_len)
{
        return usys_opentrustee(OPENTRUSTEE_SYS_tee_push_rdr_update_addr,
                                addr,
                                size,
                                is_cache_mem,
                                (long)chip_type_buff,
                                buff_len,
                                0);
}

int usys_debug_rdr_logitem(char *str, size_t str_len)
{
        return usys_opentrustee(OPENTRUSTEE_SYS_debug_rdr_logitem,
                                (long)str,
                                str_len,
                                0,
                                0,
                                0,
                                0);
}

cap_t usys_create_ns_pmo(unsigned long paddr, unsigned long size)
{
        return usys_create_pmo_with_val(size, PMO_TZ_NS, paddr);
}

struct tz_shm_info {
        cap_t target_cap_group;
        struct tee_uuid *uuid;
        cap_t *self_cap;
};
cap_t usys_create_tee_shared_pmo(cap_t cap_group, void *uuid,
                                 unsigned long size, cap_t *self_cap)
{
        struct tz_shm_info info = {
                .target_cap_group = cap_group,
                .uuid = uuid,
                .self_cap = self_cap,
        };
        return usys_create_pmo_with_val(size, PMO_TZ_SHM, (unsigned long)&info);
}

cap_t usys_get_thread_id(cap_t thread_cap)
{
        return usys_opentrustee(
                OPENTRUSTEE_SYS_get_thread_id, thread_cap, 0, 0, 0, 0, 0);
}

int usys_terminate_thread(cap_t thread_cap)
{
        return usys_opentrustee(
                OPENTRUSTEE_SYS_terminate_thread, thread_cap, 0, 0, 0, 0, 0);
}

cap_t usys_tee_msg_create_msg_hdl(void)
{
        return usys_opentrustee(
                OPENTRUSTEE_SYS_tee_msg_create_msg_hdl, 0, 0, 0, 0, 0, 0);
}

cap_t usys_tee_msg_create_channel(void)
{
        return usys_opentrustee(
                OPENTRUSTEE_SYS_tee_msg_create_channel, 0, 0, 0, 0, 0, 0);
}

int usys_tee_msg_stop_channel(cap_t channel_cap)
{
        return usys_opentrustee(OPENTRUSTEE_SYS_tee_msg_stop_channel,
                                channel_cap,
                                0,
                                0,
                                0,
                                0,
                                0);
}

int usys_tee_msg_receive(cap_t channel_cap, void *recv_buf, size_t recv_len,
                         cap_t msg_hdl_cap, void *info, int timeout)
{
        return usys_opentrustee(OPENTRUSTEE_SYS_tee_msg_receive,
                                channel_cap,
                                (unsigned long)recv_buf,
                                recv_len,
                                msg_hdl_cap,
                                (unsigned long)info,
                                timeout);
}

int usys_tee_msg_call(cap_t channel_cap, void *send_buf, size_t send_len,
                      void *recv_buf, size_t recv_len, void *timeout)
{
        return usys_opentrustee(OPENTRUSTEE_SYS_tee_msg_call,
                                channel_cap,
                                (unsigned long)send_buf,
                                send_len,
                                (unsigned long)recv_buf,
                                recv_len,
                                (unsigned long)timeout);
}

int usys_tee_msg_reply(cap_t msg_hdl_cap, void *reply_buf, size_t reply_len)
{
        return usys_opentrustee(OPENTRUSTEE_SYS_tee_msg_reply,
                                msg_hdl_cap,
                                (unsigned long)reply_buf,
                                reply_len,
                                0,
                                0,
                                0);
}

int usys_tee_msg_notify(cap_t channel_cap, void *send_buf, size_t send_len)
{
        return usys_opentrustee(OPENTRUSTEE_SYS_tee_msg_notify,
                                channel_cap,
                                (unsigned long)send_buf,
                                send_len,
                                0,
                                0,
                                0);
}

int usys_tee_wait_switch_req(struct smc_registers *regs)
{
        return usys_opentrustee(
                OPENTRUSTEE_SYS_tee_wait_switch_req, (long)regs, 0, 0, 0, 0, 0);
}

int usys_tee_switch_req(struct smc_registers *regs)
{
        return usys_opentrustee(
                OPENTRUSTEE_SYS_tee_switch_req, (long)regs, 0, 0, 0, 0, 0);
}

int usys_tee_pull_kernel_var(unsigned long cmd_buf_addr_buf)
{
        return usys_opentrustee(OPENTRUSTEE_SYS_tee_pull_kernel_var,
                                cmd_buf_addr_buf,
                                0,
                                0,
                                0,
                                0,
                                0);
}

void usys_disable_local_irq(void)
{
        (void)usys_opentrustee(
                OPENTRUSTEE_SYS_disable_local_irq, 0, 0, 0, 0, 0, 0);
}

void usys_enable_local_irq(void)
{
        (void)usys_opentrustee(
                OPENTRUSTEE_SYS_enable_local_irq, 0, 0, 0, 0, 0, 0);
}

#endif /* CHCORE_OPENTRUSTEE */
