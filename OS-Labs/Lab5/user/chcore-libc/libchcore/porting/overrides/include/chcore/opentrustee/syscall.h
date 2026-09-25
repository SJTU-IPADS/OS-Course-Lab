#ifndef CHCORE_SYSCALL_OPENTRUSTEE_SYSCALL_H
#define CHCORE_SYSCALL_OPENTRUSTEE_SYSCALL_H

#include <chcore/type.h>
#ifdef CHCORE_OPENTRUSTEE
#include <uapi/opentrustee/smc.h>
#endif

int usys_tee_push_rdr_update_addr(paddr_t addr, size_t size, bool is_cache_mem,
                                  char *chip_type_buff, size_t buff_len);
int usys_debug_rdr_logitem(char *str, size_t str_len);
cap_t usys_create_ns_pmo(unsigned long paddr, unsigned long size);
cap_t usys_create_tee_shared_pmo(cap_t cap_group, void *uuid,
                                 unsigned long size, cap_t *self_cap);
cap_t usys_get_thread_id(cap_t thread_cap);
int usys_terminate_thread(cap_t thread_cap);
cap_t usys_tee_msg_create_msg_hdl(void);
cap_t usys_tee_msg_create_channel(void);
int usys_tee_msg_stop_channel(cap_t channel_cap);
int usys_tee_msg_receive(cap_t channel_cap, void *recv_buf, size_t recv_len,
                         cap_t msg_hdl_cap, void *info, int timeout);
int usys_tee_msg_call(cap_t channel_cap, void *send_buf, size_t send_len,
                      void *recv_buf, size_t recv_len, void *timeout);
int usys_tee_msg_reply(cap_t msg_hdl_cap, void *reply_buf, size_t reply_len);
int usys_tee_msg_notify(cap_t channel_cap, void *send_buf, size_t send_len);
int usys_tee_wait_switch_req(struct smc_registers *regs);
int usys_tee_switch_req(struct smc_registers *regs);
int usys_tee_pull_kernel_var(unsigned long cmd_buf_addr_buf);
void usys_disable_local_irq(void);
void usys_enable_local_irq(void);

#endif /* CHCORE_SYSCALL_OPENTRUSTEE_SYSCALL_H */
