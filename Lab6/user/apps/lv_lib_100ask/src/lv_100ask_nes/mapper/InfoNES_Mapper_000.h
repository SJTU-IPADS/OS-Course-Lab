
#ifndef InfoNES_MAPPER_000_H_INCLUDED
#define InfoNES_MAPPER_000_H_INCLUDED

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

void Map0_Init();
void Map0_Write( WORD wAddr, BYTE byData );
void Map0_Sram( WORD wAddr, BYTE byData );
void Map0_Apu( WORD wAddr, BYTE byData );
BYTE Map0_ReadApu( WORD wAddr );
void Map0_VSync();
void Map0_HSync();
void Map0_PPU( WORD wAddr );
void Map0_RenderScreen( BYTE byMode );


#endif  /*LV_USE_100ASK_NES*/

#ifdef __cplusplus
} /*extern "C"*/
#endif


#endif
