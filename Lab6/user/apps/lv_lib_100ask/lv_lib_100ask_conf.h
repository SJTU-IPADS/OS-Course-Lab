/**
 * @file lv_lib_100ask_conf.h
 * Configuration file for v8.2.0
 *
 */
/*
 * COPY THIS FILE AS lv_lib_100ask_conf.h
 */

/* clang-format off */
#if 1 /*Set it to "1" to enable the content*/

#ifndef LV_LIB_100ASK_CONF_H
#define LV_LIB_100ASK_CONF_H

#include "lv_conf.h"

/*******************
 * GENERAL SETTING
 *******************/

/*********************
 * USAGE
 *********************/

/* Simplified Pinyin input method */
#if LV_USE_KEYBOARD
    /* Requires LV_USE_KEYBOARD = 1 */
    #define LV_USE_100ASK_PINYIN_IME                    0
#endif

#if LV_USE_100ASK_PINYIN_IME
    #define LV_100ASK_PINYIN_IME_ZH_CN_PIN_YIN_DICT     1
    #define LV_100ASK_PINYIN_IME_CAND_TEXT_NUM          10

    /*test*/
    #define LV_100ASK_PINYIN_IME_SIMPLE_TEST            1
#endif


/* Page manager */
#define LV_USE_100ASK_PAGE_MANAGER                      0
#if LV_USE_100ASK_PAGE_MANAGER
    /*Switch pages and delete old pages at the same time. */
    /*1: DELETE , 0:DELETE NO*/
    #define LV_100ASK_PAGE_MANAGER_SW_DEL_PAGE          0

    /*1: use custom animation , 0:Use built-in animation*/
    #define LV_100ASK_PAGE_MANAGER_CUSTOM_ANIMARION     0
        
	/* Page switcher snapshot*/
	#if LV_USE_SNAPSHOT
        /* Requires LV_USE_SNAPSHOT = 1 */
        #define PAGE_SWITCHER_SNAPSHOT                  0 /*TODO*/
    #endif

    #define LV_100ASK_PAGE_MANAGER_BACK_BTN_SIZE        (LV_DPI_DEF/2)

    /*test*/
    #define LV_100ASK_PAGE_MANAGER_SIMPLE_TEST          1
#endif

/*screenshot*/
#if LV_USE_SNAPSHOT
    /* Requires LV_USE_SNAPSHOT = 1 */
    #define LV_USE_100ASK_SCREENSHOT                    0
#endif
#if LV_USE_100ASK_SCREENSHOT
    /*test*/
    #define LV_USE_100ASK_SCREENSHOT_TEST               1
#endif

/* sketchpad */
#if LV_USE_CANVAS
    /* Requires LV_USE_CANVAS = 1 */
    #define LV_USE_100ASK_SKETCHPAD                     0
#endif
#if LV_USE_100ASK_SKETCHPAD
    /* set sketchpad default size */
    #define SKETCHPAD_DEFAULT_WIDTH                     1024    /*LV_HOR_RES*/
    #define SKETCHPAD_DEFAULT_HEIGHT                    600     /*LV_VER_RES*/

    /*test*/
    #define LV_100ASK_SKETCHPAD_SIMPLE_TEST             1
#endif


/*Calculator*/
#define LV_USE_100ASK_CALC                              1
#if LV_USE_100ASK_CALC
    /*Calculation expression*/
    #define LV_100ASK_CALC_EXPR_LEN                      (128) /*Maximum allowed length of expression*/
    #define LV_100ASK_CALC_MAX_NUM_LEN                   (5)   /*Maximum length of operands allowed*/

    /*test*/
    #define LV_100ASK_CALC_SIMPLE_TEST                  1
#endif

/*GAME*/
/*Memory game*/
#define LV_USE_100ASK_MEMORY_GAME                       1
#if LV_USE_100ASK_MEMORY_GAME
    /*Initial values of rows and columns.*/
    /*Recommended row == column*/
    #define  LV_100ASK_MEMORY_GAME_DEFAULT_ROW          4
    #define  LV_100ASK_MEMORY_GAME_DEFAULT_COL          4

    /*test*/
    #define  LV_100ASK_MEMORY_GAME_SIMPLE_TEST          1
#endif   

/*2048 game*/
#define LV_USE_100ASK_2048                              1
#if LV_USE_100ASK_2048
    /* Matrix size*/
    /*Do not modify*/
    #define  LV_100ASK_2048_MATRIX_SIZE                 4

    /*test*/
    #define  LV_100ASK_2048_SIMPLE_TEST                 1
#endif

/*File explorer*/
#define LV_USE_100ASK_FILE_EXPLORER                     1
#if LV_USE_100ASK_FILE_EXPLORER
    /*Maximum length of path*/
    #define LV_100ASK_FILE_EXPLORER_PATH_MAX_LEN        (128)
    /*Quick access bar, 1:use, 0:not use*/
    #define LV_100ASK_FILE_EXPLORER_QUICK_ACCESS        1
    /*test*/
    #define  LV_100ASK_FILE_EXPLORER_SIMPLE_TEST        1
#endif  

/***Game simulator***/
/*NES*/
#if LV_USE_IMG
    #define LV_USE_100ASK_NES                           0
#endif
#if LV_USE_100ASK_NES
    /*platform*/
    #define LV_100ASK_NES_PLATFORM_POSIX                1
    #define LV_100ASK_NES_PLATFORM_FREERTOS             0 /*Not tested yet*/
    #define LV_100ASK_NES_PLATFORM_RTTHREAD             0 /*TODO*/
    
    /*test*/
#if LV_USE_100ASK_FILE_EXPLORER
    #define LV_100ASK_NES_SIMPLE_TEST                   1
#endif
#endif

#endif /*LV_LIB_100ASK_H*/

#endif /*End of "Content enable"*/

