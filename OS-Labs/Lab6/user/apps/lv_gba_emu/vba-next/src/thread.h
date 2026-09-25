#ifndef __THREAD_H__
#define __THREAD_H__

#define THREAD_PRIORITY_HIGHEST 	1
#define THREAD_PRIORITY_HIGH 		2
#define THREAD_PRIORITY_NORMAL 		3
#define THREAD_PRIORITY_LOW 		4
#define THREAD_PRIORITY_LOWEST 		5

#include <stdint.h>

#if VITA
#include <psp2/types.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if VITA
typedef SceUID thread_t;
#else
typedef void* thread_t;
#endif

#ifdef THREADED_RENDERER
typedef void(*threadfunc_t)(void*);

thread_t thread_get();
thread_t thread_run(threadfunc_t func, void* p, int priority);
void thread_sleep(int ms);
void thread_set_priority(thread_t id, int priority);
#endif

#ifdef __cplusplus
}
#endif

#endif

