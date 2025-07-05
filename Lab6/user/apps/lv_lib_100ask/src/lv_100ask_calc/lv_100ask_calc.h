/**
 * @file lv_100ask_calc.h
 *
 */

#ifndef LV_100ASK_CALC_H
#define LV_100ASK_CALC_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../lv_lib_100ask.h"

#if LV_USE_100ASK_CALC != 0

/*********************
 *      DEFINES
 *********************/
// padding
#define LV_100ASK_CALC_HISTORY_MAX_LINE     (128)
#define LV_100ASK_CALC_HISTORY_MAX_H        ((LV_VER_RES/4) * 1)
#define LV_100ASK_PANEL_CALC_MAX_H          ((LV_VER_RES/4) * 3)
#define LV_100ASK_COLOR_BLACK               lv_color_hex(0x000000)
#define LV_100ASK_COLOR_GREEN               lv_color_hex(0xf5fffa)

/**********************
 *      TYPEDEFS
 **********************/
// token
typedef enum {
    TOKENIZER_ERROR = 0,    // Error
    TOKENIZER_ENDOFINPUT,   // End of input
    TOKENIZER_NUMBER,       // number
    TOKENIZER_PLUS,         // +
    TOKENIZER_MINUS,        // -
    TOKENIZER_ASTR,         // *
    TOKENIZER_SLASH,        // /
    TOKENIZER_LPAREN,       // (
    TOKENIZER_RPAREN,       // )
} lv_100ask_calc_token_t;

/* Error code */
typedef enum {
    no_error = 0,           // no error
    syntax_error,           // syntax error
} lv_100ask_calc_error_t;

/* Error code and corresponding message */
typedef struct {
    lv_100ask_calc_error_t error_code;
    char   *message;
} lv_100ask_calc_error_table_t;


/*Data of canvas*/
typedef struct {
    lv_obj_t obj;
    lv_obj_t * ta_hist;                         // display calc history
    lv_obj_t * ta_input;                        // input
    lv_obj_t * btnm;                            // btnmatrix
    char     * curr_char;                       // The character currently parsed by the expression
    char     * next_char;                       // The next character of the expression
    lv_100ask_calc_token_t  current_token;                     // Current token
    lv_100ask_calc_error_t  error_code;                        // Error code
    uint16_t count;                             // Record expression location
    char     calc_exp[LV_100ASK_CALC_EXPR_LEN]; // Expression
} lv_100ask_calc_t;

/***********************
 * GLOBAL VARIABLES
 ***********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
lv_obj_t * lv_100ask_calc_create(lv_obj_t * parent);

/*=====================
 * Setter functions
 *====================*/

/*=====================
 * Getter functions
 *====================*/

/**
 * Get the calc btnmatrix to allow styling or other modifications
 * @param obj   pointer to a calc object
 * @return      pointer to the btnmatrix of the calc
 */
lv_obj_t * lv_100ask_calc_get_btnm(lv_obj_t * obj);

/**
 * Get the calc textarea(calc history) to allow styling or other modifications
 * @param obj   pointer to a calc object
 * @return      pointer to the textarea(calc history) of the calc
 */
lv_obj_t * lv_100ask_calc_get_ta_hist(lv_obj_t * obj);

/**
 * Get the calc textarea(calc input) to allow styling or other modifications
 * @param obj   pointer to a calc object
 * @return      pointer to the textarea(calc input) of the calc
 */
lv_obj_t * lv_100ask_calc_get_ta_input(lv_obj_t * obj);


/*=====================
 * Other functions
 *====================*/

/**********************
 *      MACROS
 **********************/

#endif  /*LV_USE_100ASK_CALC*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_100ASK_CALC_H*/
