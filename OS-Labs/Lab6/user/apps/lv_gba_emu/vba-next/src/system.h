#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>

extern void systemDrawScreen (void);
extern bool systemReadJoypads (void);
extern uint32_t systemGetClock (void);
extern void systemMessage(const char *, ...);
#ifdef USE_MOTION_SENSOR
extern void systemUpdateMotionSensor (void);
extern int  systemGetAccelX (void);
extern int  systemGetAccelY (void);
extern int  systemGetGyroZ (void);
extern void systemSetSensorState(bool);
#endif

// sound functions
extern void systemOnWriteDataToSoundBuffer(int16_t * finalWave, int length);
#endif // SYSTEM_H
