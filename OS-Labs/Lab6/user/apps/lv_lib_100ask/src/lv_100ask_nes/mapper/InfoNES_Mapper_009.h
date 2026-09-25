
#ifndef InfoNES_MAPPER_009_H_INCLUDED
#define InfoNES_MAPPER_009_H_INCLUDED

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


void Map9_Init();
void Map9_Write( WORD wAddr, BYTE byData );
void Map9_PPU( WORD wAddr );

#endif  /*LV_USE_100ASK_NES*/

#ifdef __cplusplus
} /*extern "C"*/
#endif


#endif
