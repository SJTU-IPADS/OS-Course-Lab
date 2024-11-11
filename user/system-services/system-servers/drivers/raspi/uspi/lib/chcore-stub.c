/*
 * Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
 * Licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#define _GNU_SOURCE
#include <sched.h>

#include <stdio.h>
#include <stdlib.h>
#include <chcore/type.h>
#include <chcore/syscall.h>
#include <chcore/pmu.h>
#include <pthread.h>

#include "uspi/stdarg.h"
#include "config.h"
#include "uspios.h"
#include "uspienv/bcmpropertytags.h"

#undef malloc
#undef free

#include <stdlib.h>

__thread int debug_tid;

#define kinfo printf

#define BUG_ON(expr)                                                          \
	do {                                                                  \
		if ((expr)) {                                                 \
			printf("BUG: %s:%d %s\n", __func__, __LINE__, #expr); \
			for (;;) {                                            \
			}                                                     \
		}                                                             \
	} while (0)

void assertion_failed(const char *pExpr, const char *pFile, unsigned nLine)
{
#if DISABLE_LOG_NOTICE == 0
	kinfo("[failed] expr: %s, file: %s, line: %d\n", pExpr, pFile, nLine);
#endif
}

void uspi_assertion_failed(const char *pExpr, const char *pFile, unsigned nLine)
{
#if DISABLE_LOG_NOTICE == 0
	kinfo("[failed] expr: %s, file: %s, line: %d\n", pExpr, pFile, nLine);
#endif
	printf("debug_tid %d\n", debug_tid);
	BUG_ON(1);
}

#if 0
void *malloc(unsigned size)
{
	return kmalloc(size);
}

void free(void *p)
{
	kfree(p);
}
#endif

void *tmalloc(unsigned size)
{
	return malloc(size);
}

void tfree(void *p)
{
	free(p);
}

/*
 * Disable most print during driver init,
 * because printf may involves futex which may still be buggy now.
 */
extern int simple_vsprintf(char **out, const char *format, va_list ap);
void LogWrite(const char *pSource, unsigned Severity, const char *pMessage, ...)
{
#if DISABLE_LOG_NOTICE == 0
	if (Severity > LOG_NOTICE)
		return;
#else
	if (Severity > LOG_WARNING)
		return;
#endif

#if 0
	kinfo("%s: ", pSource);

	va_list va;

	va_start(va, pMessage);
	extern int vfprintf(FILE *restrict f, const char *restrict fmt, va_list ap);
	vfprintf(stdout, pMessage, va);
	va_end(va);

	printf("\n");
#endif
}

void usDelay(unsigned nMicroSeconds)
{
	volatile u64 cnt;
	volatile u64 i;

	cnt = nMicroSeconds * 100UL;

	for (i = 0; i < cnt; ++i) {
	}
}

void MsDelay(unsigned nMilliSeconds)
{
	usDelay(nMilliSeconds * 1000);
}

int GetMACAddress(unsigned char Buffer[6])
{
	TBcmPropertyTags Tags;
	BcmPropertyTags(&Tags);
	TPropertyTagMACAddress MACAddress;
	if (!BcmPropertyTagsGetTag(&Tags,
				   PROPTAG_GET_MAC_ADDRESS,
				   &MACAddress,
				   sizeof MACAddress,
				   0)) {
		_BcmPropertyTags(&Tags);

		return 0;
	}

	memcpy(Buffer, MACAddress.Address, 6);
	kinfo("MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
	      Buffer[0],
	      Buffer[1],
	      Buffer[2],
	      Buffer[3],
	      Buffer[4],
	      Buffer[5]);

	_BcmPropertyTags(&Tags);
	return 1;
}

void DebugHexdump(const void *pBuffer, unsigned nBufLen, const char *pSource)
{
	printf("%s\n", pSource);
	for (int i = 0; i < nBufLen; i++) {
		printf("0x%x, ", *((unsigned char *)(pBuffer) + i));
	}
	printf("\n");
	return;
}

/* Ticking Time Bomb */

#define TICK_TIMER_NUM 20

// typedef void TKernelTimerHandler (unsigned hTimer, void *pParam, void *pContext);

typedef struct TKernelTimer {
	TKernelTimerHandler *m_pHandler;
	unsigned m_nElapsesAt;
	void *m_pParam;
	void *m_pContext;
} TKernelTimer;

TKernelTimer m_KernelTimer[TICK_TIMER_NUM];

u64 plat_get_current_tick(void)
{
	return usys_get_current_tick();
}

void fire_timer_callbacks(void)
{
	unsigned hTimer;
	TKernelTimer *pTimer;
	TKernelTimerHandler *pHandler;

	u64 current_tick = plat_get_current_tick();

	for (hTimer = 0; hTimer < TICK_TIMER_NUM; hTimer++) {
		pTimer = &m_KernelTimer[hTimer];
		pHandler = pTimer->m_pHandler;

		if (pHandler != 0) {
			if (current_tick > pTimer->m_nElapsesAt) {
				// printf("fires callback !!!!!!!!!!!!!!!\n");
				(*pHandler)(hTimer + 1,
					    pTimer->m_pParam,
					    pTimer->m_pContext);

				pTimer->m_pHandler = 0;
			}
		}
	}
}

void init_timer_callbacks(void)
{
	unsigned hTimer;
	TKernelTimer *pTimer;

	for (hTimer = 0; hTimer < TICK_TIMER_NUM; hTimer++) {
		pTimer = &m_KernelTimer[hTimer];
		pTimer->m_pHandler = 0;
	}
}

unsigned TimerStartKernelTimer(unsigned nDelay, TKernelTimerHandler *pHandler,
			       void *pParam, void *pContext)
{
	unsigned hTimer;

	for (hTimer = 0; hTimer < TICK_TIMER_NUM; hTimer++) {
		if (m_KernelTimer[hTimer].m_pHandler == 0) {
			break;
		}
	}

	if (hTimer >= TICK_TIMER_NUM) {
		kinfo("%s failed: no more free timers.\n", __func__);
		BUG_ON(1);
		return 0;
	}

	BUG_ON(pHandler == 0);

	m_KernelTimer[hTimer].m_nElapsesAt = nDelay + plat_get_current_tick();
	m_KernelTimer[hTimer].m_pParam = pParam;
	m_KernelTimer[hTimer].m_pContext = pContext;

	asm volatile("dmb ish\n\t" ::: "memory");

	// printf("TimerCallback: 0x%lx\n", (u64)pHandler);

	m_KernelTimer[hTimer].m_pHandler = pHandler;

	return hTimer + 1;
}

void TimerCancelKernelTimer(unsigned hTimer)
{
	BUG_ON(!(1 <= hTimer && hTimer <= TICK_TIMER_NUM));

	m_KernelTimer[hTimer - 1].m_pHandler = 0;
}

/* Hardware related */

unsigned StartKernelTimer(unsigned nDelay, TKernelTimerHandler *pHandler,
			  void *pParam, void *pContext)
{
	// kinfo("%s is invoked\n", __func__);
	return TimerStartKernelTimer(nDelay, pHandler, pParam, pContext);
}

void CancelKernelTimer(unsigned hTimer)
{
	// kinfo("%s is invoked\n", __func__);
	TimerCancelKernelTimer(hTimer);
}

static void bind_to_cpu(int cpu)
{
	int ret;
	cpu_set_t mask;

	CPU_ZERO(&mask);
	CPU_SET(cpu, &mask);
	ret = sched_setaffinity(0, sizeof(mask), &mask);
	BUG_ON(ret != 0);

	/*
	 * Invoke usys_yield to ensure current thread to run on the target CPU.
	 * If current thread is running on another CPU, it will be migrated to
	 * the specific CPU after setting affinity.
	 */
	usys_yield();
}

#include <sys/time.h>
volatile int usb_timer_thread_ready = 0;
/* The periodical thread. */
void *usb_timer_thread(void *arg)
{
	debug_tid = 2;

	bind_to_cpu(0);

	/* TODO: Can use notication later for avoiding periodical wake up. */
	//int notifc_cap;
	//notifc_cap = usys_create_notifc();

	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = 10000000;

	usb_timer_thread_ready = 1;

	while (1) {
		//ret = usys_wait(notifc_cap, true, &ts);

		/* Sleep for 10ms. */
		nanosleep(&ts, NULL);
		// printf("%s wakes up\n", __func__);
		fire_timer_callbacks();
	}
}

typedef void TInterruptHandler(void *pParam);

struct usb_irq_handler_args {
	TInterruptHandler *pHandler;
	void *pParam;
};

volatile int usb_irq_handler_thread_ready = 0;
/* The interrupt handler thread. */
void *usb_interrupt_handler_thread(void *arg)
{
	struct usb_irq_handler_args *irq_args;
	TInterruptHandler *pHandler;
	void *pParam;
	cap_t irq_cap;

	debug_tid = 1;

	bind_to_cpu(0);

	irq_args = (struct usb_irq_handler_args *)arg;
	pHandler = irq_args->pHandler;
	pParam = irq_args->pParam;

	/*
	 * Now, usys_irq_register will disable interrupt for current thread,
	 * which can be used to avoid the potential re-entry problem.
	 */
	irq_cap = usys_irq_register(0x9);
	// printf("irq_cap is %d\n", irq_cap);

	usb_irq_handler_thread_ready = 1;

	while (1) {
		/* Wait for a USB interrupt; it also acks previous interrupts */
		usys_irq_wait(irq_cap, true);

		/* Execute the interrupt handler */
		pHandler(pParam);

		// TODO: merge into usys_irq_wait
		// usys_irq_ack(irq_cap);
	}

	return NULL;
}

void ConnectInterrupt(unsigned nIRQ, TInterruptHandler *pHandler, void *pParam)
{
	pthread_t pid;
	struct usb_irq_handler_args *irq_args;

	debug_tid = 0;
	printf("%s: nIRQ %d, pHandler: 0x%lx\n", __func__, nIRQ, (u64)pHandler);

	irq_args = (struct usb_irq_handler_args *)malloc(sizeof(*irq_args));
	irq_args->pHandler = pHandler;
	irq_args->pParam = pParam;

	init_timer_callbacks();

	/* Create the interrupt handler thread. */
	pthread_create(&pid, NULL, usb_interrupt_handler_thread, irq_args);

	/*
	 * Create a periodical thread for handling installed callbacks
	 * (specific for USB on raspi3).
	 */
	pthread_create(&pid, NULL, usb_timer_thread, NULL);

	/*
	 * An important note:
	 * Currently, the interrupt handler thread, periodical thread,
	 * and the init thread (main thread) all execute on CPU 0 to avoid
	 * potential errors caused by concurrency.
	 */

	while (usb_irq_handler_thread_ready == 0) {
		usys_yield();
	}

	while (usb_timer_thread_ready == 0) {
		usys_yield();
	}

	printf("%s finished\n", __func__);
}

/* A more robust way is implementing them in an asm file. */
void put32(u64 addr, u32 val)
{
	asm volatile("str w1, [x0]\n\t");
}

void get32(u64 addr)
{
	asm volatile("ldr w0, [x0]\n\t");
}
