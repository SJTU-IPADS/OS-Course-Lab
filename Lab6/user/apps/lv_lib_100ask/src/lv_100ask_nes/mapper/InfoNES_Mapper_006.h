
#ifndef InfoNES_MAPPER_006_H_INCLUDED
#define InfoNES_MAPPER_006_H_INCLUDED

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


void Map6_Init();
void Map6_Write( WORD wAddr, BYTE byData );
void Map6_Apu( WORD wAddr, BYTE byData );
void Map6_HSync();

#endif  /*LV_USE_100ASK_NES*/

#ifdef __cplusplus
} /*extern "C"*/
#endif


#endif
