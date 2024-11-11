#pragma once

#include <stdint.h>
#include <stdbool.h>

#define STATUS int
#define FAST
#define IMPORT extern
#define LOCAL static
#define FOREVER for (;;)
#define EOS '\n'
#define ERROR (-1)
#define OK 0
typedef int (*FUNCPTR)();
typedef bool BOOL;
