/*
 *	InfoNES
 *		SDL ports by mata      03/04/19
 *              Modified by Jay        06/02/25
 *
 * 	Start Date: 2003.04.19
 */

#include "InfoNES_System.h"
#if LV_USE_100ASK_NES != 0

#include "lvgl/lvgl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "InfoNES.h"
#include "InfoNES_pAPU.h"

/*-------------------------------------------------------------------*/
/*  ROM image file information                                       */
/*-------------------------------------------------------------------*/

char szRomName[ 256 ];
char szSaveName[ 256 ];
int nSRAM_SaveFlag;

/*-------------------------------------------------------------------*/
/*  Constants ( Linux specific )                                     */
/*-------------------------------------------------------------------*/

#define VBOX_SIZE 0
static const char * VERSION = "InfoNES v0.97J RC1";

/*-------------------------------------------------------------------*/
/*  Global Variables ( SDL specific )                                */
/*-------------------------------------------------------------------*/

bool quit = false;

/* for video */

/* For Sound Emulation */
BYTE final_wave[2048];
int waveptr;
int wavflag;
int wavdone;

/* Pad state */
DWORD dwPad1;
DWORD dwPad2;
DWORD dwSystem;

/*-------------------------------------------------------------------*/
/*  Function prototypes ( SDL specific )                             */
/*-------------------------------------------------------------------*/

int start_application(char *filename);
void poll_event(void);
int LoadSRAM();
int SaveSRAM();


/* Palette data */
#if LV_COLOR_DEPTH == 16
WORD NesPalette[ 64 ] =
{
	0x39ce, 0x1071, 0x0015, 0x2013, 0x440e, 0x5402, 0x5000, 0x3c20,
	0x20a0, 0x0100, 0x0140, 0x00e2, 0x0ceb, 0x0000, 0x0000, 0x0000,
	0x5ef7, 0x01dd, 0x10fd, 0x401e, 0x5c17, 0x700b, 0x6ca0, 0x6521,
	0x45c0, 0x0240, 0x02a0, 0x0247, 0x0211, 0x0000, 0x0000, 0x0000,
	0x7fff, 0x1eff, 0x2e5f, 0x223f, 0x79ff, 0x7dd6, 0x7dcc, 0x7e67,
	0x7ae7, 0x4342, 0x2769, 0x2ff3, 0x03bb, 0x0000, 0x0000, 0x0000,
	0x7fff, 0x579f, 0x635f, 0x6b3f, 0x7f1f, 0x7f1b, 0x7ef6, 0x7f75,
	0x7f94, 0x73f4, 0x57d7, 0x5bf9, 0x4ffe, 0x0000, 0x0000, 0x0000
};
#else if LV_COLOR_DEPTH == 32
WORD NesPalette[ 64 ]=
{
#if 0
	0x616161, 0x000088, 0x1F0D99, 0x371379, 0x561260, 0x5D0010, 0x520E00, 0x3A2308,
	0x21350C, 0x0D410E, 0x174417, 0x003A1F, 0x002F57, 0x000000, 0x000000, 0x000000,
	0xAAAAAA, 0x0D4DC4, 0x4B24DE, 0x6912CF, 0x9014AD, 0x9D1C48, 0x923404, 0x735005,
	0x5D6913, 0x167A11, 0x138008, 0x127649, 0x1C6691, 0x000000, 0x000000, 0x000000,
	0xFCFCFC, 0x639AFC, 0x8A7EFC, 0xB06AFC, 0xDD6FF2, 0xE771AB, 0xE38658, 0xCC9E22,
	0xA8B100, 0x72C100, 0x5ACD4E, 0x34C28E, 0x4FBECE, 0x424242, 0x000000, 0x000000,
	0xFCFCFC, 0xBED4FC, 0xCACAFC, 0xD9C4FC, 0xECC1FC, 0xFAC3E7, 0xF7CEC3, 0xE2CDA7,
	0xDADB9C, 0xC8E39E, 0xBFE5B8, 0xB2EBC8, 0xB7E5EB, 0xACACAC, 0x000000, 0x000000
#else
	0x6A6D6A, 0x001380, 0x1E008A, 0x39007A, 0x550056, 0x5A0018, 0x4F1000, 0x3D1C00,
	0x253200, 0x003D00, 0x004000, 0x003924, 0x002E55, 0x000000, 0x000000, 0x000000,
	0xB9BCB9, 0x1850C7, 0x4B30E3, 0x7322D6, 0x951FA9, 0x9D285C, 0x983700, 0x7F4C00,
	0x5E6400, 0x227700, 0x027E02, 0x007645, 0x006E8A, 0x000000, 0x000000, 0x000000,
	0xFFFFFF, 0x68A6FF, 0x8C9CFF, 0xB586FF, 0xD975FD, 0xE377B9, 0xE58D68, 0xD49D29,
	0xB3AF0C, 0x7BC211, 0x55CA47, 0x46CB81, 0x47C1C5, 0x4A4D4A, 0x000000, 0x000000,
	0xFFFFFF, 0xCCEAFF, 0xDDDEFF, 0xECDAFF, 0xF8D7FE, 0xFCD6F5, 0xFDDBCF, 0xF9E7B5,
	0xF1F0AA, 0xDAFAA9, 0xC9FFBC, 0xC3FBD7, 0xC4F6F6, 0xBEC1BE, 0x000000, 0x000000
#endif
};
#endif

/*===================================================================*/
/*                                                                   */
/*           LoadSRAM() : Load a SRAM                                */
/*                                                                   */
/*===================================================================*/
/* Start application */
int start_application(char *filename)
{
  /* Set a ROM image name */
  strcpy( szRomName, filename );

  /* Load cassette */
  if(InfoNES_Load(szRomName)==0) {
    /* Load SRAM */
    LoadSRAM();

    /* Success */
    return 1;
  }
  /* Failure */
  return 0;
}


/*===================================================================*/
/*                                                                   */
/*           LoadSRAM() : Load a SRAM                                */
/*                                                                   */
/*===================================================================*/
int LoadSRAM()
{
/*
 *  Load a SRAM
 *
 *  Return values
 *     0 : Normally
 *    -1 : SRAM data couldn't be read
 */

  //FILE *fp;
  lv_fs_file_t rom;
  unsigned char pSrcBuf[ SRAM_SIZE ];
  unsigned char chData;
  unsigned char chTag;
  int nRunLen;
  int nDecoded;
  int nDecLen;
  int nIdx;

  // It doesn't need to save it
  nSRAM_SaveFlag = 0;

  // It is finished if the ROM doesn't have SRAM
  if ( !ROM_SRAM )
    return 0;

  // There is necessity to save it
  nSRAM_SaveFlag = 1;

  // The preparation of the SRAM file name
  strcpy( szSaveName, szRomName );
  strcpy( strrchr( szSaveName, '.' ) + 1, "srm" );

  /*-------------------------------------------------------------------*/
  /*  Read a SRAM data                                                 */
  /*-------------------------------------------------------------------*/
  lv_fs_res_t res = lv_fs_open(&rom, szSaveName, LV_FS_MODE_RD);
  /* Error Detection */
  if (res != LV_FS_RES_OK) {
      //fprintf(stderr, "Error: couldn't open file.\n");
      printf("[Open Error]: couldn't open file!\n");
      return -1;
  }

  lv_fs_read(&rom, &pSrcBuf, SRAM_SIZE, NULL);

  lv_fs_close(&rom);

  /*-------------------------------------------------------------------*/
  /*  Extract a SRAM data                                              */
  /*-------------------------------------------------------------------*/

  nDecoded = 0;
  nDecLen = 0;

  chTag = pSrcBuf[ nDecoded++ ];

  while ( nDecLen < 8192 )
  {
    chData = pSrcBuf[ nDecoded++ ];

    if ( chData == chTag )
    {
      chData = pSrcBuf[ nDecoded++ ];
      nRunLen = pSrcBuf[ nDecoded++ ];
      for ( nIdx = 0; nIdx < nRunLen + 1; ++nIdx )
      {
        SRAM[ nDecLen++ ] = chData;
      }
    }
    else
    {
      SRAM[ nDecLen++ ] = chData;
    }
  }

  // Successful
  return 0;
}

/*===================================================================*/
/*                                                                   */
/*           SaveSRAM() : Save a SRAM                                */
/*                                                                   */
/*===================================================================*/
int SaveSRAM()
{
/*
 *  Save a SRAM
 *
 *  Return values
 *     0 : Normally
 *    -1 : SRAM data couldn't be written
 */

  //FILE *fp;
    lv_fs_file_t rom;

  int nUsedTable[ 256 ];
  unsigned char chData;
  unsigned char chPrevData;
  unsigned char chTag;
  int nIdx;
  int nEncoded;
  int nEncLen;
  int nRunLen;
  unsigned char pDstBuf[ SRAM_SIZE ];

  if ( !nSRAM_SaveFlag )
    return 0;  // It doesn't need to save it

  /*-------------------------------------------------------------------*/
  /*  Compress a SRAM data                                             */
  /*-------------------------------------------------------------------*/

  memset( nUsedTable, 0, sizeof nUsedTable );

  for ( nIdx = 0; nIdx < SRAM_SIZE; ++nIdx )
  {
    ++nUsedTable[ SRAM[ nIdx++ ] ];
  }
  for ( nIdx = 1, chTag = 0; nIdx < 256; ++nIdx )
  {
    if ( nUsedTable[ nIdx ] < nUsedTable[ chTag ] )
      chTag = nIdx;
  }

  nEncoded = 0;
  nEncLen = 0;
  nRunLen = 1;

  pDstBuf[ nEncLen++ ] = chTag;

  chPrevData = SRAM[ nEncoded++ ];

  while ( nEncoded < SRAM_SIZE && nEncLen < SRAM_SIZE - 133 )
  {
    chData = SRAM[ nEncoded++ ];

    if ( chPrevData == chData && nRunLen < 256 )
      ++nRunLen;
    else
    {
      if ( nRunLen >= 4 || chPrevData == chTag )
      {
        pDstBuf[ nEncLen++ ] = chTag;
        pDstBuf[ nEncLen++ ] = chPrevData;
        pDstBuf[ nEncLen++ ] = nRunLen - 1;
      }
      else
      {
        for ( nIdx = 0; nIdx < nRunLen; ++nIdx )
          pDstBuf[ nEncLen++ ] = chPrevData;
      }

      chPrevData = chData;
      nRunLen = 1;
    }

  }
  if ( nRunLen >= 4 || chPrevData == chTag )
  {
    pDstBuf[ nEncLen++ ] = chTag;
    pDstBuf[ nEncLen++ ] = chPrevData;
    pDstBuf[ nEncLen++ ] = nRunLen - 1;
  }
  else
  {
    for ( nIdx = 0; nIdx < nRunLen; ++nIdx )
      pDstBuf[ nEncLen++ ] = chPrevData;
  }

  /*-------------------------------------------------------------------*/
  /*  Write a SRAM data                                                */
  /*-------------------------------------------------------------------*/
  // Open SRAM file
  lv_fs_res_t res = lv_fs_open(&rom, szSaveName, LV_FS_MODE_WR);

  /* Error Detection */
  if (res != LV_FS_RES_OK) {
      //fprintf(stderr, "Save Error: couldn't open file.\n");
      printf("[Save Error]: couldn't open file!\n");
      return -1;
  }

  // Write SRAM data
  lv_fs_write(&rom, pDstBuf, nEncLen, NULL);

  // Close SRAM file
  lv_fs_close(&rom);

  // Successful
  return 0;
}

/*===================================================================*/
/*                                                                   */
/*                  InfoNES_Menu() : Menu screen                     */
/*                                                                   */
/*===================================================================*/
int InfoNES_Menu() {
  lv_100ask_nes_menu();
  //if(quit) return -1;
  return 0;
}

/* Read ROM image file */
int InfoNES_ReadRom( const char *pszFileName ) {
  //FILE *fp;
  lv_fs_file_t rom;
  uint32_t rn;
  long file_size;

  /* Open ROM file */
  lv_fs_res_t res = lv_fs_open(&rom, pszFileName, LV_FS_MODE_RD);
  /* Error Detection */
  if (res != LV_FS_RES_OK) {
      //fprintf(stderr, "Error: couldn't open file.\n");
      printf("[Open Error]: couldn't open file!\n");
      return 8;
  }
  /* Getting filesize */
	lv_fs_seek(&rom, 0L, LV_FS_SEEK_END);
	lv_fs_tell(&rom, &file_size);


	if (file_size < 0x4010) {
		//fprintf(stderr, "Error: input file is too small.\n");
    printf("[Check Error]: input file is too small!\n");

		lv_fs_close(&rom);
        return 8;
	}

  lv_fs_close(&rom);
  lv_fs_open(&rom, pszFileName, LV_FS_MODE_RD | LV_FS_MODE_WR);


  lv_fs_read(&rom, &NesHeader, sizeof(NesHeader), &rn);
  if ( memcmp( NesHeader.byID, "NES\x1a", 4 ) != 0 )
  {
      /* not .nes file */
      lv_fs_close(&rom);
      printf("[Check Error]: Not .nes file!\n");
      return -1;
  }

  /* Clear SRAM */
  memset( SRAM, 0, SRAM_SIZE );

  /* If trainer presents Read Triner at 0x7000-0x71ff */
  if(NesHeader.byInfo1 & 4){
    lv_fs_read(&rom, &SRAM[ 0x1000 ], 512, NULL);
  }


  /* Allocate Memory for ROM Image */
#if LV_MEM_CUSTOM == 0
  if(ROM == NULL)
    ROM = (BYTE *)lv_mem_alloc( NesHeader.byRomSize * 0x4000 );
  else
    ROM = (BYTE *)lv_mem_realloc(ROM, NesHeader.byRomSize * 0x4000 );
#else
  if(ROM == NULL)
    ROM = (BYTE *)malloc( NesHeader.byRomSize * 0x4000 );
  else
    ROM = (BYTE *)realloc(ROM, NesHeader.byRomSize * 0x4000 );
#endif

  printf("NesHeader.byRomSize * 0x4000:%d\n", NesHeader.byRomSize * 0x4000);

  /* Read ROM Image */
  lv_fs_read(&rom, ROM, 0x4000 * NesHeader.byRomSize, NULL);

  if ( NesHeader.byVRomSize > 0 )
  {
      /* Allocate Memory for VROM Image */
#if LV_MEM_CUSTOM == 0
      if(ROM == NULL)
        VROM = (BYTE *)lv_mem_alloc( NesHeader.byVRomSize * 0x2000 );
      else
        VROM = (BYTE *)lv_mem_realloc(VROM, NesHeader.byVRomSize * 0x2000 );
#else
      if(ROM == NULL)
        VROM = (BYTE *)malloc( NesHeader.byVRomSize * 0x2000 );
      else
        VROM = (BYTE *)realloc(VROM, NesHeader.byVRomSize * 0x2000 );
#endif

      printf("NesHeader.byVRomSize * 0x2000: %d\n", NesHeader.byVRomSize * 0x2000);

      /* Read VROM Image */
      lv_fs_read(&rom, VROM, 0x2000*NesHeader.byVRomSize, NULL);
  }

  /* File close */
  lv_fs_close(&rom);
  printf("Read file successful!!!\n");

  /* Successful */
  return 0;
}

/* Release a memory for ROM */
void InfoNES_ReleaseRom(){
  if(ROM) {
#if LV_MEM_CUSTOM == 0
    lv_mem_free(ROM);
#else
    free(ROM);
#endif

  ROM=NULL;
  }

  if(VROM){
#if LV_MEM_CUSTOM == 0
    lv_mem_free(VROM);
#else
    free(VROM);
#endif

    VROM=NULL;
  }
}

/* Transfer the contents of work frame on the screen */
void InfoNES_LoadFrame(){
  lv_100ask_nes_flush();
}

/* Get a joypad state */
void InfoNES_PadState( DWORD *pdwPad1, DWORD *pdwPad2, DWORD *pdwSystem )
{
  //poll_event();
  *pdwPad1 = dwPad1;
  *pdwPad2 = dwPad2;
  *pdwSystem = dwSystem;
}

static const int joy_commit_range = 3276;

#if 0
void poll_event(void)
{
  SDL_Event	e;
  static signed char	x_joy=0, y_joy=0;

  while(SDL_PollEvent(&e))
  {
    switch(e.type)
    {
    case SDL_QUIT:
      dwSystem|=PAD_SYS_QUIT;
      quit=1;
      break;

    case SDL_KEYDOWN:
      switch(e.key.keysym.sym)
      {
      case SDLK_RETURN:
	if( !(e.key.keysym.mod & KMOD_ALT)) break;
	SDL_WM_ToggleFullScreen(screen);
	break;

      case SDLK_ESCAPE:
	dwSystem|=PAD_SYS_QUIT;
	quit=1;
	break;

      case SDLK_RIGHT:   dwPad1 |= (1<<7);break;
      case SDLK_LEFT:    dwPad1 |= (1<<6);break;
      case SDLK_DOWN:    dwPad1 |= (1<<5);break;
      case SDLK_UP:      dwPad1 |= (1<<4);break;
      case SDLK_s:	 dwPad1 |= (1<<3);break;    /* Start */
      case SDLK_a:	 dwPad1 |= (1<<2);break;    /* Select */
      case SDLK_z:	 dwPad1 |= (1<<1);break;    /* 'B' */
      case SDLK_x:	 dwPad1 |= (1<<0);break;    /* 'A' */
      case SDLK_m:
	/* Toggle of sound mute */
	APU_Mute = ( APU_Mute ? 0 : 1 );break;
      case SDLK_c:
	/* Toggle up and down clipping */
	PPU_UpDown_Clip = ( PPU_UpDown_Clip ? 0 : 1 ); break;
      } /* keydown */
      break;

    case SDL_KEYUP:
      switch(e.key.keysym.sym)
      {
      case SDLK_RIGHT:   dwPad1 &=~(1<<7);break;
      case SDLK_LEFT:    dwPad1 &=~(1<<6);break;
      case SDLK_DOWN:	 dwPad1 &=~(1<<5);break;
      case SDLK_UP:	 dwPad1 &=~(1<<4);break;
      case SDLK_s:	 dwPad1 &=~(1<<3);break;   /* Start */
      case SDLK_a:	 dwPad1 &=~(1<<2);break;   /* Select */
      case SDLK_z:	 dwPad1 &=~(1<<1);break;   /* 'B' */
      case SDLK_x:	 dwPad1 &=~(1<<0);break;   /* 'A' */
      } /* keyup */
      break;

    case SDL_JOYAXISMOTION:
      switch(e.jaxis.axis){
      case 0:	/* X axis */
	if(e.jaxis.value >  joy_commit_range){
	  if(x_joy > 0) break;
	  if(x_joy < 0) dwPad1 &=~(1<<6);
	  dwPad1 |= (1<<7); x_joy=+1; break; }
	if(e.jaxis.value < -joy_commit_range){
	  if(x_joy < 0) break;
	  if(x_joy > 0) dwPad1 &=~(1<<7);
	  dwPad1 |= (1<<6); x_joy=-1; break; }
	if     (x_joy < 0) dwPad1 &=~(1<<6);
	else if(x_joy > 0) dwPad1 &=~(1<<7);
	x_joy= 0; break;
      case 1: /* Y asis */
	if(e.jaxis.value >  joy_commit_range){
	  if(y_joy > 0) break;
	  if(y_joy < 0) dwPad1 &=~(1<<4);
	  dwPad1 |= (1<<5); y_joy=+1; break; }
	if(e.jaxis.value < -joy_commit_range){
	  if(y_joy < 0) break;
	  if(y_joy > 0) dwPad1 &=~(1<<5);
	  dwPad1 |= (1<<4); y_joy=-1; break; }
	if      (y_joy < 0) dwPad1 &=~(1<<4);
	else if (y_joy > 0) dwPad1 &=~(1<<5);
	y_joy= 0; break;
      } /* joysxismotion */

    case SDL_JOYBUTTONUP:
      switch(e.jbutton.button){
      case 2: dwPad1 &=~(1<<0);break; /* A */
      case 1: dwPad1 &=~(1<<1);break; /* B */
      case 8: dwPad1 &=~(1<<2);break; /* select */
      case 9: dwPad1 &=~(1<<3);break; /* start */
      } break;
    case SDL_JOYBUTTONDOWN:
      switch(e.jbutton.button){
      case 2: dwPad1 |= (1<<0);break; /* A */
      case 1: dwPad1 |= (1<<1);break; /* B */
      case 8: dwPad1 |= (1<<2);break; /* select */
      case 9: dwPad1 |= (1<<3);break; /* start */
      } break;
    }
  }
}
#endif // 0


/* memcpy */
void *InfoNES_MemoryCopy( void *dest, const void *src, int count ){
  memcpy( dest, src, count );
  return dest;
}

/* memset */
void *InfoNES_MemorySet( void *dest, int c, int count ){
  memset( dest, c, count);
  return dest;
}

/* Print debug message */
void InfoNES_DebugPrint( char *pszMsg ) {
  //fprintf(stderr,"%s\n", pszMsg);
  printf("[Error]: %s\n", pszMsg);
}

/* Wait */
void InfoNES_Wait(){}

/* Sound Initialize */
void InfoNES_SoundInit( void ){}

void waveout(void *udat,BYTE *stream,int len){
  if ( !wavdone )
  {
    /* we always expect that len is 1024 */
    memcpy( stream, &final_wave[(wavflag - 1) << 10], len );
    wavflag = 0; wavdone = 1;
  }
}

/* Sound Open */
int InfoNES_SoundOpen( int samples_per_sync, int sample_rate ){
#if 0
  SDL_AudioSpec asp;

  asp.freq=sample_rate;
  asp.format=AUDIO_U8;
  asp.channels=1;
  asp.samples=1024;
  asp.callback=waveout;

  if(SDL_OpenAudio(&asp,&audio_spec)<0){
    //fprintf(stderr,"Can't Open SDL Audio\n");
    printf("Can't Open SDL Audio\n");
    return -1;
  }
  waveptr = wavflag = 0; wavdone = 1;
  SDL_PauseAudio(0);

  /* Successful */
  return 1;
#endif // 0

}

/* Sound Close */
void InfoNES_SoundClose( void ){
  //SDL_CloseAudio();
}

/* Sound Output 5 Waves - 2 Pulse, 1 Triangle, 1 Noise. 1 DPCM */
void InfoNES_SoundOutput(int samples, BYTE *wave1, BYTE *wave2, BYTE *wave3, BYTE *wave4, BYTE *wave5){
  int i;

  for (i = 0; i < samples; i++)
  {
    final_wave[waveptr] =
      ( wave1[i] + wave2[i] + wave3[i] + wave4[i] + wave5[i] ) / 5;

    waveptr++;
    if ( waveptr == 2048 )
    {
      waveptr = 0;
      wavflag = 2; wavdone=0;
    }
    else if ( waveptr == 1024 )
    {
      wavflag = 1; wavdone=0;
    }
    //while (!wavdone) SDL_Delay(0);
  }
}

/* Print system message */
void InfoNES_MessageBox(char *pszMsg, ...){}

#endif  /*LV_USE_100ASK_NES*/

