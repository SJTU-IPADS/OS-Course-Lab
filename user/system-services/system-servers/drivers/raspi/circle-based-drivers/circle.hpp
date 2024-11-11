#pragma once

#define PREFIX "[Circle]"
#define error(fmt, ...) printf(PREFIX " " fmt, ##__VA_ARGS__)
#define info(fmt, ...) printf(PREFIX " " fmt, ##__VA_ARGS__)
#include <pthread.h>

/* Bind the thread on a CPU core by setting cpu affinity. */
void bind_to_cpu(int cpu);

#ifdef CIRCLE_AUDIO
void* audio(void* args);
#endif //CIRCLE_AUDIO

#ifdef CIRCLE_GPIO
void* gpio(void* args);
#endif //CIRCLE_GPIO

#ifdef CIRCLE_SD
void* sdcard(void* args);
#endif //CIRCLE_SD

#ifdef CIRCLE_SERIAL
void* serial(void* args);
#endif

#ifdef CIRCLE_USB
void* usb_mouse_and_keyboard(void* args);
#endif

#ifdef CIRCLE_ETHERNET
int bcm54213_start();
#endif 

#ifdef CIRCLE_WLAN
int wlan_start();
#endif

#ifdef CIRCLE_CPU
void* cpu(void* args);
#endif
