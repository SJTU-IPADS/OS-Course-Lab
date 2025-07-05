#ifndef GBA_CHEATS_H
#define GBA_CHEATS_H

struct CheatsData {
  int code;
  int size;
  int status;
  bool enabled;
  uint32_t rawaddress;
  uint32_t address;
  uint32_t value;
  uint32_t oldValue;
  char codestring[20];
  char desc[32];
};

void cheatsAdd(const char *codeStr, const char *desc, uint32_t rawaddress, uint32_t address, uint32_t value, int code, int size);
void cheatsAddCheatCode(const char *code, const char *desc);
void cheatsAddGSACode(const char *code, const char *desc, bool v3);
void cheatsAddCBACode(const char *code, const char *desc);
void cheatsDelete(int number, bool restore);
void cheatsDeleteAll(bool restore);
void cheatsEnable(int number);
void cheatsDisable(int number);
void cheatsWriteMemory(uint32_t address, uint32_t value);
void cheatsWriteHalfWord(uint32_t address, uint16_t value);
void cheatsWriteByte(uint32_t address, uint8_t value);
int cheatsCheckKeys(uint32_t keys, uint32_t extended);

extern int cheatsNumber;
extern CheatsData cheatsList[100];

#endif // CHEATS_H
