
#ifndef InfoNES_MAPPER_005_H_INCLUDED
#define InfoNES_MAPPER_005_H_INCLUDED

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


void Map5_Init();
void Map5_Write( WORD wAddr, BYTE byData );
void Map5_Apu( WORD wAddr, BYTE byData );
BYTE Map5_ReadApu( WORD wAddr );
void Map5_HSync();
void Map5_RenderScreen( BYTE byMode );
void Map5_Sync_Prg_Banks( void );

#endif  /*LV_USE_100ASK_NES*/

#ifdef __cplusplus
} /*extern "C"*/
#endif


#endif
