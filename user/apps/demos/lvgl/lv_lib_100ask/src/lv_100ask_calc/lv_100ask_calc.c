/**
 * @file lv_100ask_calc.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_100ask_calc.h"

#if LV_USE_100ASK_CALC != 0

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_100ask_calc_class
/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_100ask_calc_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_100ask_calc_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);

static void calc_btnm_changed_event_cb(lv_event_t *e);
static void lv_100ask_calc_tokenizer_init(lv_obj_t *obj, char *expr);
static lv_100ask_calc_token_t lv_100ask_calc_get_next_token(lv_obj_t *obj);
static lv_100ask_calc_token_t lv_100ask_calc_siglechar(char *curr_char);
static int lv_100ask_calc_expr(lv_obj_t *obj);
static int lv_100ask_calc_term(lv_obj_t *obj);
static int lv_100ask_calc_factor(lv_obj_t *obj);
static int lv_100ask_calc_tokenizer_num(char *curr_char);
static void lv_100ask_calc_accept(lv_obj_t *obj, lv_100ask_calc_token_t token);
static void lv_100ask_calc_error(lv_100ask_calc_error_t error_code ,lv_100ask_calc_error_t err);
static void lv_100ask_calc_tokenizer_next(lv_obj_t *obj);
static bool lv_100ask_calc_tokenizer_finished(lv_100ask_calc_token_t current_token, char *curr_char);
static int lv_100ask_calc_expr(lv_obj_t *obj);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_100ask_calc_class = {
    .constructor_cb = lv_100ask_calc_constructor,
    .destructor_cb  = lv_100ask_calc_destructor,
    .width_def      = LV_DPI_DEF * 2,
    .height_def     = LV_DPI_DEF * 3,
    .instance_size  = sizeof(lv_100ask_calc_t),
    .base_class     = &lv_obj_class
};

// Key layout
static const char * btnm_map[] = {  "(", ")", "C", "<-", "\n",
									"7", "8", "9", "/",  "\n",
									"4", "5", "6", "*",  "\n",
									"1", "2", "3", "-",  "\n",
									"0", ".", "=", "+",  ""};

// error list
static const lv_100ask_calc_error_table_t error_table[] = {
    {.error_code = no_error,            .message = "no error"},
    {.error_code = syntax_error,        .message = "syntax error!"}
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * lv_100ask_calc_create(lv_obj_t * parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

/*=====================
 * Setter functions
 *====================*/


/*=====================
 * Getter functions
 *====================*/

lv_obj_t * lv_100ask_calc_get_btnm(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_100ask_calc_t * calc = (lv_100ask_calc_t *)obj;

    return calc->btnm;
}


lv_obj_t * lv_100ask_calc_get_ta_hist(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_100ask_calc_t * calc = (lv_100ask_calc_t *)obj;

    return calc->ta_hist;
}


lv_obj_t * lv_100ask_calc_get_ta_input(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_100ask_calc_t * calc = (lv_100ask_calc_t *)obj;

    return calc->ta_input;
}

/*=====================
 * Other functions
 *====================*/

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_100ask_calc_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_100ask_calc_t * calc = (lv_100ask_calc_t *)obj;

    calc->curr_char = NULL;
    calc->next_char = NULL;
    calc->current_token = TOKENIZER_ERROR;
    calc->error_code = no_error;
    calc->count = 0;

    /*set layout*/
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_COLUMN);

    /*Display calculation history and results*/
    calc->ta_hist = lv_textarea_create(obj);
    lv_obj_set_style_bg_color(calc->ta_hist, LV_100ASK_COLOR_BLACK, 0);
    lv_obj_set_style_text_color(calc->ta_hist, LV_100ASK_COLOR_GREEN, 0);
    lv_obj_set_style_radius(calc->ta_hist, 0, 0);
    
    lv_obj_set_size(calc->ta_hist, LV_PCT(100), LV_PCT(20));
    lv_textarea_set_cursor_click_pos(calc->ta_hist, false);
    lv_textarea_set_max_length(calc->ta_hist, LV_100ASK_CALC_HISTORY_MAX_LINE);
    lv_textarea_set_align(calc->ta_hist, LV_TEXT_ALIGN_RIGHT);
    lv_textarea_set_text(calc->ta_hist, "");
    lv_textarea_set_placeholder_text(calc->ta_hist, "CALC HISTORY\t\t");
    lv_obj_set_style_border_width(calc->ta_hist, 0, 0);

    /*Input textarea*/
    calc->ta_input = lv_textarea_create(obj);
    lv_obj_set_style_bg_color(calc->ta_input, LV_100ASK_COLOR_BLACK, 0);
    lv_obj_set_style_text_color(calc->ta_input, LV_100ASK_COLOR_GREEN, 0);
    lv_obj_set_style_radius(calc->ta_input, 0, 0);
    lv_obj_set_style_border_width(calc->ta_input, 0, 0);
    
    lv_obj_set_size(calc->ta_input, LV_PCT(100), LV_PCT(5));
    lv_textarea_set_one_line(calc->ta_input, true);
    lv_textarea_set_cursor_click_pos(calc->ta_input, false);
    lv_textarea_set_max_length(calc->ta_input, LV_100ASK_CALC_HISTORY_MAX_LINE);
    lv_textarea_set_align(calc->ta_input, LV_TEXT_ALIGN_RIGHT);
    lv_textarea_set_text(calc->ta_input, "");

    /*Calculator input panel*/
    calc->btnm = lv_btnmatrix_create(obj);
    lv_obj_set_style_radius(calc->btnm, 0, 0);
    lv_obj_set_style_border_width(calc->btnm, 0, 0);

    lv_obj_set_size(calc->btnm, LV_PCT(100), LV_PCT(75));
    lv_btnmatrix_set_map(calc->btnm, btnm_map);
    lv_obj_add_event_cb(calc->btnm, calc_btnm_changed_event_cb, LV_EVENT_VALUE_CHANGED, obj);

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_100ask_calc_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);

}




static void calc_btnm_changed_event_cb(lv_event_t *e)
{
    lv_obj_t * obj = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * user_data = lv_event_get_user_data(e);
    
    uint32_t id = lv_btnmatrix_get_selected_btn(obj);
    const char * txt = lv_btnmatrix_get_btn_text(obj, id);

    lv_100ask_calc_t * calc = (lv_100ask_calc_t *)user_data;

    if(code == LV_EVENT_VALUE_CHANGED)
    {
        // Perform operations
        if (strcmp(txt, "=") == 0)
        {
            char tmp_buff[32];
            int calc_results;

            // Lexical analyzer
            lv_100ask_calc_tokenizer_init(user_data, calc->calc_exp);

            // Calculates the value of the first level priority expression
            calc_results = lv_100ask_calc_expr(user_data);  

            if (calc->error_code != no_error)
            {
                // Find the error code and display the corresponding message
                for (int i = 0; i < sizeof(error_table); i++)
                {
                    if (error_table[i].error_code == calc->error_code)
                    {
                        lv_textarea_add_text(calc->ta_hist, "\n");
                        lv_textarea_add_text(calc->ta_hist, error_table[i].message);
                        lv_textarea_add_text(calc->ta_hist, "\n");
                    }
                }
                calc->error_code = no_error;
            }
            else
            {
                lv_snprintf(tmp_buff, sizeof(tmp_buff), "%s=%d\n", lv_textarea_get_text(calc->ta_input), calc_results);
                lv_textarea_add_text(calc->ta_hist, tmp_buff);
                lv_textarea_set_text(calc->ta_input, tmp_buff);
                // Empty expression
                lv_memset_00(calc->calc_exp, sizeof(calc->calc_exp));
                calc->count = 0;

            }

        }
        // clear
        else if (strcmp(txt, "C") == 0)
        {
            lv_textarea_set_text(calc->ta_input, "");
            // Empty expression
            lv_memset_00(calc->calc_exp, sizeof(calc->calc_exp));
            calc->count = 0;
        }
        // del char
        else if (strcmp(txt, "<-") == 0)             
        {
            lv_textarea_del_char(calc->ta_input);
            calc->calc_exp[calc->count-1] = '\0';
            calc->count--;
        }
        // Add char
        else
        {
            if((calc->count == 0) && (strcmp(lv_textarea_get_text(calc->ta_input), "") == 0))
                lv_textarea_set_text(calc->ta_input, "");

            lv_textarea_add_text(calc->ta_input, txt);
            strcat(&calc->calc_exp[0], txt);
            calc->count++;
        }
    }
}



/**
 * Lexical analyzer initialization.
 * @param obj       pointer to a calc object
 * @param expr      pointer to expression
 */
static void lv_100ask_calc_tokenizer_init(lv_obj_t *obj, char *expr)
{
    lv_100ask_calc_t * calc = (lv_100ask_calc_t *)obj;

    calc->curr_char = calc->next_char = expr;
    calc->current_token = lv_100ask_calc_get_next_token(obj);

    return;
}



/**
 * Get a token.
 * @param obj       pointer to a calc object
 * @return          Token type
 */
static lv_100ask_calc_token_t lv_100ask_calc_get_next_token(lv_obj_t *obj)
{
    int i;
    lv_100ask_calc_t * calc = (lv_100ask_calc_t *)obj;

    // End of expression
    if (calc->curr_char == '\0')
        return TOKENIZER_ENDOFINPUT;

    if (isdigit(*calc->curr_char))
    {
        // The length of the allowed number cannot be exceeded
        for (i = 0; i <= LV_100ASK_CALC_MAX_NUM_LEN; i++)
        {
            if (!isdigit(*(calc->curr_char + i)))
            {
                calc->next_char = calc->curr_char + i;
                return TOKENIZER_NUMBER;
            }
        }
    }

    // Delimiter
    else if (lv_100ask_calc_siglechar(calc->curr_char))
    {
        calc->next_char++;
        return lv_100ask_calc_siglechar(calc->curr_char);
    }

    return TOKENIZER_ERROR;
}



/**
 * Get single character token type.
 * @param curr_char       Pointer to character
 * @return                Token type
 */
static lv_100ask_calc_token_t lv_100ask_calc_siglechar(char *curr_char)
{
    switch (*curr_char)
    {
        case '+':
            return TOKENIZER_PLUS;
        case '-':
            return TOKENIZER_MINUS;
        case '*':
            return TOKENIZER_ASTR;
        case '/':
            return TOKENIZER_SLASH;
        case '(':
            return TOKENIZER_LPAREN;
        case ')':
            return TOKENIZER_RPAREN;
        default:
            break;
    }

    return TOKENIZER_ERROR;
}


/**
 * Calculates the value of the first level priority expression.
 * @param curr_char       pointer to a calc object
 * @return                Calculation results
 */
static int lv_100ask_calc_expr(lv_obj_t *obj)
{
    lv_100ask_calc_t * calc = (lv_100ask_calc_t *)obj;
    int t1, t2 = 0;
    lv_100ask_calc_token_t op;

    // First operand
    t1 = lv_100ask_calc_term(obj);
    // Get operator
    op = calc->current_token;

    // Operators can only be plus or minus (same priority)
    while (op == TOKENIZER_PLUS || op == TOKENIZER_MINUS)
    {
        // Next token
        lv_100ask_calc_tokenizer_next(obj);

        // Second operand
        t2 = lv_100ask_calc_term(obj);
        switch ((int)op)
        {
            case TOKENIZER_PLUS:
                t1 = t1 + t2;
                break;
            case TOKENIZER_MINUS:
                t1 = t1 - t2;
                break;
        }
        op = calc->current_token;
    }

    return t1;
}


/**
 * Calculates the value of the second level priority (multiplication and division) expression.
 * @param curr_char       pointer to a calc object
 * @return                Calculation results
 */
static int lv_100ask_calc_term(lv_obj_t *obj)
{
    lv_100ask_calc_t * calc = (lv_100ask_calc_t *)obj;
    int f1, f2;
    lv_100ask_calc_token_t op;

    // Get left operand (factor)
    f1 = lv_100ask_calc_factor(obj);

    // Get operator
    op = calc->current_token;

    // Operators can only be multiply or divide (same priority)
    while (op == TOKENIZER_ASTR || op == TOKENIZER_SLASH)
    {
        // Next token
        lv_100ask_calc_tokenizer_next(obj);
        
        // Get right operand (factor)
        f2 = lv_100ask_calc_factor(obj);
        switch ((int)op)
        {
            case TOKENIZER_ASTR:
                f1 = f1 * f2;
                break;
            case TOKENIZER_SLASH:
                f1 = f1 / f2;
                break;
        }
        // The value calculated above will be used as the left operand
        op = calc->current_token;
    }

    return f1;
}


/**
 * Get the value of the current factor. 
 * If the current factor (similar to m in the above formula) is an expression, 
 * perform recursive evaluation
 * @param curr_char       pointer to a calc object
 * @return                Value of factor
 */
static int lv_100ask_calc_factor(lv_obj_t *obj)
{
    int r = 0;
    lv_100ask_calc_t * calc = (lv_100ask_calc_t *)obj;

    // Type of current token
    switch (calc->current_token)
    {
        // Number (Terminator)
        case TOKENIZER_NUMBER:
            // Convert it from ASCII to numeric value
            r = lv_100ask_calc_tokenizer_num(calc->curr_char);
            // Match the current token according to syntax rules
            lv_100ask_calc_accept(obj, TOKENIZER_NUMBER);
            break;
        // left bracket
        case TOKENIZER_LPAREN:
            lv_100ask_calc_accept(obj, TOKENIZER_LPAREN);
            // Treat the value in parentheses as a new expression and calculate recursively (recursion starts with function expr())
            r = lv_100ask_calc_expr(obj);
            // When the expression in the bracket is processed, the next token must be the right bracket
            lv_100ask_calc_accept(obj, TOKENIZER_RPAREN);
            break;
            // Tokens other than the left parenthesis and numbers have been disposed of by the upper level
            // If there is a token, it must be an expression syntax error
        default:
            lv_100ask_calc_error(calc->error_code, syntax_error);
    }

    // Returns the value of the factor
    return r;
}


/**
 * Convert it from ASCII to numeric value.
 * @param curr_char       Pointer to character
 * @return                Result of conversion to integer
 */
static int lv_100ask_calc_tokenizer_num(char *curr_char)
{
    return atoi(curr_char);
}


/**
 * Match the current token according to syntax rules.
 * @param curr_char       pointer to a calc object
 * @param token           token
 */
static void lv_100ask_calc_accept(lv_obj_t *obj, lv_100ask_calc_token_t token)
{
    lv_100ask_calc_t * calc = (lv_100ask_calc_t *)obj;

    if (token != calc->current_token)
        lv_100ask_calc_error(calc->error_code, syntax_error);

    lv_100ask_calc_tokenizer_next(obj);
}



/**
 * Set error code.
 * @param error_code       Current error_code
 * @param err              Error code to be set
 */
static void lv_100ask_calc_error(lv_100ask_calc_error_t error_code, lv_100ask_calc_error_t err)
{
    error_code = err;

    LV_UNUSED(error_code);
}


/**
 * Parse next token.
 * @param error_code       pointer to a calc object
 */
static void lv_100ask_calc_tokenizer_next(lv_obj_t *obj)
{
    lv_100ask_calc_t * calc = (lv_100ask_calc_t *)obj;

    if (lv_100ask_calc_tokenizer_finished(calc->current_token, calc->curr_char))
        return;

    calc->curr_char = calc->next_char;
    calc->current_token = lv_100ask_calc_get_next_token(obj);

    return;
}


/**
 * Judge whether the token has reached the end.
 * @param current_token   curren token
 * @param curr_char       pointer to a calc object
 * @return                true:  There are no tokens to parse
 *                        false: There are tokens that need to be resolved
 */
static bool lv_100ask_calc_tokenizer_finished(lv_100ask_calc_token_t current_token, char *curr_char)
{
    return *curr_char == '\0' || current_token == TOKENIZER_ENDOFINPUT;
}


#endif  /*LV_USE_100ASK_CALC*/
