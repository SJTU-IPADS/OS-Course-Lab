#ifndef GBA_H
#define GBA_H

#include <stdint.h>
#include "GBACheats.h"
#include "thread.h"

#define BITS_16 0
#define BITS_32 1

#define SAVE_GAME_VERSION_1 1
#define SAVE_GAME_VERSION_2 2
#define SAVE_GAME_VERSION_3 3
#define SAVE_GAME_VERSION_4 4
#define SAVE_GAME_VERSION_5 5
#define SAVE_GAME_VERSION_6 6
#define SAVE_GAME_VERSION_7 7
#define SAVE_GAME_VERSION_8 8
#define SAVE_GAME_VERSION_9 9
#define SAVE_GAME_VERSION_10 10
#define SAVE_GAME_VERSION  SAVE_GAME_VERSION_10

#define R13_IRQ  18
#define R14_IRQ  19
#define SPSR_IRQ 20
#define R13_USR  26
#define R14_USR  27
#define R13_SVC  28
#define R14_SVC  29
#define SPSR_SVC 30
#define R13_ABT  31
#define R14_ABT  32
#define SPSR_ABT 33
#define R13_UND  34
#define R14_UND  35
#define SPSR_UND 36
#define R8_FIQ   37
#define R9_FIQ   38
#define R10_FIQ  39
#define R11_FIQ  40
#define R12_FIQ  41
#define R13_FIQ  42
#define R14_FIQ  43
#define SPSR_FIQ 44

typedef struct {
	uint8_t *address;
	uint32_t mask;
} memoryMap;

typedef union {
	struct {
#ifdef MSB_FIRST
		uint8_t B3;
		uint8_t B2;
		uint8_t B1;
		uint8_t B0;
#else
		uint8_t B0;
		uint8_t B1;
		uint8_t B2;
		uint8_t B3;
#endif
	} B;
	struct {
#ifdef MSB_FIRST
		uint16_t W1;
		uint16_t W0;
#else
		uint16_t W0;
		uint16_t W1;
#endif
	} W;
#ifdef MSB_FIRST
	volatile uint32_t I;
#else
	uint32_t I;
#endif
} reg_pair;

typedef struct 
{
	reg_pair reg[45];
	bool busPrefetch;
	bool busPrefetchEnable;
	uint32_t busPrefetchCount;
	uint32_t armNextPC;
} bus_t;

typedef struct
{
	int layerEnable;
	int layerEnableDelay;
	int lcdTicks;
} graphics_t;

extern uint64_t joy;

extern void (*cpuSaveGameFunc)(uint32_t,uint8_t);

extern bool CPUReadState(const uint8_t * data, unsigned size);
extern unsigned CPUWriteState(uint8_t* data, unsigned size);
#ifdef LOAD_FROM_MEMORY
extern int CPULoadRomData(const char *data, int size);
#else
extern int CPULoadRom(const char *);
#endif
extern void doMirroring(bool);
extern void CPUUpdateRegister(uint32_t, uint16_t);
extern void CPUInit(const char *,bool);
extern void CPUCleanUp (void);
extern void CPUReset (void);
extern void CPULoop(void);
extern void UpdateJoypad(void);
extern void CPUCheckDMA(int,int);
#if USE_FRAME_SKIP
extern void SetFrameskip(int);
#endif

#if THREADED_RENDERER
extern void ThreadedRendererStart(void);
extern void ThreadedRendererStop(void);
#endif

#if USE_MOTION_SENSOR

#define HARDWARE_SENSOR_NONE 0
#define HARDWARE_SENSOR_TILT 0x1
#define HARDWARE_SENSOR_GYRO 0x2

typedef struct {
	int sensor;

	int tilt_x;
	int tilt_y;

	uint16_t pinState;
	uint16_t direction;
	uint16_t gyroSample;
	bool readWrite;
	bool gyroEdge;
} hardware_t;

extern hardware_t hardware;

#endif

#endif // GBA_H
