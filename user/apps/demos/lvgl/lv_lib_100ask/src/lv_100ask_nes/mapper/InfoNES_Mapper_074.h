
#ifndef InfoNES_MAPPER_074_H_INCLUDED
#define InfoNES_MAPPER_074_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/*-------------------------------------------------------------------*/
/*  Include files                                                    */
/*-------------------------------------------------------------------*/

#include "../InfoNES_Types.h"

#if LV_USE_100ASK_NES != 0

/*-------------------------------------------------------------------*/
/*  Function prototypes                                              */
/*-------------------------------------------------------------------*/


void Map74_Init();
void Map74_Write( WORD wAddr, BYTE byData );
void Map74_HSync();
void Map74_Set_CPU_Banks();
void Map74_Set_PPU_Banks();

#endif  /*LV_USE_100ASK_NES*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
