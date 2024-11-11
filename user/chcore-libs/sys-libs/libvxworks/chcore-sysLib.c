#include <chcore/syscall.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sysLib.h"

/* XXX: seems that syslib is not a part of vxworks, but defined by app */
exc_record_t excRecord;
exc_record_t *excRecordGet(void)
{
	return &excRecord;
}

/* 1us as a tick */
int sysClkRateGet(void)
{
	return 1000 * 1000;
}

void serial_puts(const char *s)
{
        usys_putstr((vaddr_t)s, strlen(s));
}

void serial_putc(const char c)
{
        usys_putstr((vaddr_t)(&c), 1);
}

#define MAX_IRQ_NUM 128
cap_t irq_cap_map[MAX_IRQ_NUM] = {[0 ... MAX_IRQ_NUM - 1] = -1};
void Clear_Interrupt(unsigned char interrupt)
{
        if (interrupt < 0 || interrupt >= MAX_IRQ_NUM || irq_cap_map[interrupt] == -1)
                return;
        usys_irq_ack(irq_cap_map[interrupt]);
}

void Close_Interrupt(unsigned char interrupt)
{
        usys_disable_irqno(interrupt);
}

void Init_Isr(void)
{
        printf("Init_Isr is not implemented\n");
}

void *user_irq_handler_wrapper(void *arg)
{
        unsigned irqno;
        int ret;
        cap_t irq_notific_cap;
        void (* user_irq_handler)(void);

        irqno = ((struct irq_bind_arg *)arg)->irqno;
        user_irq_handler = ((struct irq_bind_arg *)arg)->user_irq_handler;
        free(arg);

        if (irqno >= MAX_IRQ_NUM)
                return NULL;

        if ((irq_notific_cap = usys_irq_register(irqno)) < 0) {
                printf("irq register failed, return %d\n", irq_notific_cap);
                return (void *)irq_notific_cap;
        }
        irq_cap_map[irqno] = irq_notific_cap;

        while (true) {
                ret = usys_irq_wait(irq_notific_cap, true);
                if (ret < 0) {
                        printf("irq wait failed, ret = %d\n", ret);
                        return (void *)ret;
                }
                user_irq_handler();
        }
}

void Bind_Interrupt(unsigned irqno, void (* handler)(void))
{
        int ret;
        pthread_t thread_id;
        struct irq_bind_arg *arg;
        pthread_attr_t attr;
        struct sched_param sched_param;

        arg = malloc(sizeof(struct irq_bind_arg));
        arg->irqno = irqno;
        arg->user_irq_handler = handler;
        pthread_attr_init(&attr);
        sched_param.sched_priority = 199;
        pthread_attr_setschedparam(&attr, &sched_param);
        pthread_attr_setinheritsched(&attr, true);
        if ((ret = pthread_create(&thread_id, &attr,
                        user_irq_handler_wrapper, (void *)arg)) < 0)
                printf("create pthread failed, ret = %d\n", ret);

        return;
}

unsigned int Get_Gpt_Clock_Counter(void)
{
        /* XXX: assumes a counter represents 1ms, ChCore SPARC assumes 1us as a tick */
        return (usys_get_current_tick() / 1000) % 1000;
}

void Config_Cache(bool enbale)
{
        usys_cache_config(enbale);
}

int map_device_pmo(unsigned long virt_addr, unsigned long phys_addr,
		unsigned long len)
{
        int ret;
	cap_t pmo_cap;

	pmo_cap = usys_create_device_pmo(phys_addr, len);
	if (pmo_cap < 0) {
		printf("%s err:%d\n", __func__, pmo_cap);
		return pmo_cap;
	}

	ret = usys_map_pmo(0, pmo_cap, virt_addr, VM_READ | VM_WRITE);
	return ret;
}
