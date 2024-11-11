#pragma once

#include "vxWorks.h"

/* XXX: seems that syslib is not a part of vxworks, but defined by app */

#define IRQ_CAN1        (4)
#define IRQ_CAN2        (5)
#define IRQ_FPGA        (6)

typedef struct exc_record {
	unsigned short 	valid;                       /* 有效标识,为0x1982表示有效 */   
	unsigned char 	trap_int_cnt;               /* Trap中断计数 */
	unsigned char 	trap_source;               /* Trap中断源 */
	unsigned char 	single_bit_error_cnt;	/* 单比特错次数,20140319, zpp, 备用for 695*/
	unsigned char 	double_bit_error_cnt;		/* 双比特错次数 寄存器双bit错误计数 */
	unsigned char 	trap_type;                  /* Trap类型 */
	unsigned char 	error_type;	        /* 错误类型，从System Fault Status Register中读出,20140319, zpp, 备用for 695*/
	unsigned int 		fail_addr;			/*20140319, zpp, 备用for 695*/
	/* 下面的参数为暂时的，以后可能改动 */
	unsigned int     pc;                        /* 导致错误的pc */
	unsigned int     psr;                       /* psr */
	unsigned int     fsr;                       /* fsr *//*added by zpp, 20120731*/
	unsigned int     task_id;                   /* 导致错误的地址 */
}exc_record_t;

struct irq_bind_arg {
        unsigned irqno;
        void (* user_irq_handler)(void);
};

exc_record_t *excRecordGet(void);
int sysClkRateGet(void);

void serial_puts(const char *s);
void serial_putc(const char c);
void Clear_Interrupt(unsigned char interrupt);
void Close_Interrupt(unsigned char interrupt);
void Init_Isr(void);
void Bind_Interrupt(unsigned irqno, void (* handler)(void));

unsigned int Get_Gpt_Clock_Counter(void);
void Config_Cache(bool enbale);

int map_device_pmo(unsigned long virt_addr, unsigned long phys_addr,
		unsigned long len);
