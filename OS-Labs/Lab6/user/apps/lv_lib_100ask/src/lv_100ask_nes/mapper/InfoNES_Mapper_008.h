
#ifndef InfoNES_MAPPER_008_H_INCLUDED
#define InfoNES_MAPPER_008_H_INCLUDED

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


void Map8_Init();
void Map8_Write( WORD wAddr, BYTE byData );

#endif  /*LV_USE_100ASK_NES*/

#ifdef __cplusplus
} /*extern "C"*/
#endif


#endif
