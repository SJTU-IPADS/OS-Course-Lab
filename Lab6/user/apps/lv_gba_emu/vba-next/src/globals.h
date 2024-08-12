#ifndef GLOBALS_H
#define GLOBALS_H

#include "types.h"
#include <stdint.h>

//performance boost tweaks.
#if USE_TWEAKS
    #define USE_TWEAK_SPEEDHACK 1
	#define USE_TWEAK_MEMFUNC 1
#endif

#define PIX_BUFFER_SCREEN_WIDTH 256

extern int saveType;
extern bool useBios;
extern bool skipBios;
extern bool cpuIsMultiBoot;
extern int cpuSaveType;
extern bool mirroringEnable;
extern bool enableRtc;
extern bool skipSaveGameBattery; // skip battery data when reading save states

extern int cpuDmaCount;

extern uint8_t *rom;
extern uint8_t *bios;
extern uint8_t *vram;
extern uint16_t *pix;
extern uint8_t *oam;
extern uint8_t *ioMem;
extern uint8_t *internalRAM;
extern uint8_t *workRAM;
extern uint8_t *paletteRAM;

#endif // GLOBALS_H
