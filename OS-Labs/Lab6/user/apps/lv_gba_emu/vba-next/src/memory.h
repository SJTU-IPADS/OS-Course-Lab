#ifndef GBA_MEMORY_H
#define GBA_MEMORY_H

#define FLASH_128K_SZ 0x20000

#define EEPROM_IDLE           0
#define EEPROM_READADDRESS    1
#define EEPROM_READDATA       2
#define EEPROM_READDATA2      3
#define EEPROM_WRITEDATA      4

enum
{
   IMAGE_UNKNOWN,
   IMAGE_GBA
};

/* save game */
typedef struct
{
   void *address;
   int size;
} variable_desc;

extern int flashSize;

extern bool eepromInUse;
extern int eepromSize;

extern uint8_t *flashSaveMemory;
extern u8 *eepromData;

extern void eepromReadGameMem(const uint8_t *&data, int version);
extern void eepromSaveGameMem(uint8_t *&data);

extern int eepromRead(void);
extern void eepromWrite(u8 value);
extern void eepromInit(void);
extern void eepromReset(void);

extern u8 sramRead(u32 address);
extern void sramWrite(u32 address, u8 byte);
extern void sramDelayedWrite(u32 address, u8 byte);
extern void flashSaveGameMem(uint8_t *& data);
extern void flashReadGameMem(const uint8_t *& data, int version);

extern uint8_t flashRead(uint32_t address);
extern void flashWrite(uint32_t address, uint8_t byte);
extern void flashDelayedWrite(uint32_t address, uint8_t byte);
extern void flashSaveDecide(uint32_t address, uint8_t byte);
extern void flashReset(void);
extern void flashSetSize(int size);
extern void flashInit(void);

extern u16 rtcRead(u32 address);
extern bool rtcWrite(u32 address, u16 value);
extern void rtcEnable(bool);
extern bool rtcIsEnabled (void);
extern void rtcReset (void);
extern void rtcReadGameMem(const uint8_t *& data);
extern void rtcSaveGameMem(uint8_t *& data);

extern u16 gyroRead(u32 address);
extern bool gyroWrite(u32 address, u16 value);

void utilWriteIntMem(uint8_t *& data, int);
void utilWriteMem(uint8_t *& data, const void *in_data, unsigned size);
void utilWriteDataMem(uint8_t *& data, variable_desc *);

int utilReadIntMem(const uint8_t *& data);
void utilReadMem(void *buf, const uint8_t *& data, unsigned size);
void utilReadDataMem(const uint8_t *& data, variable_desc *);

#endif // GBA_MEMORY_H
