
/**
 * @file simple_test.c
 *
 */



/*********************
 *      INCLUDES
 *********************/
#include "simple_test.h"
#include <stdio.h>
#include "string.h"
#include <chcore/launcher.h>

#if LV_100ASK_FILE_EXPLORER_SIMPLE_TEST != 0

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/



/**********************
 *  STATIC PROTOTYPES
 **********************/
static void file_explorer_simple_test_event_handler(lv_event_t * e);

/**********************
 *  STATIC VARIABLES
 **********************/


/**********************
 *      MACROS
 **********************/


/**********************
 *   GLOBAL FUNCTIONS
 **********************/
lv_obj_t * file_list = NULL;

void lv_100ask_file_explorer_simple_test(void)
{
    lv_obj_t * file_explorer = lv_100ask_file_explorer_create(lv_scr_act());
    file_list = lv_100ask_file_explorer_get_file_list(file_explorer);

#if LV_USE_FS_WIN32
    //跨磁盘访问要先设置 lv_conf.h 中的 LV_FS_WIN32_LETTER
    // [windows]打开某个磁盘的根目录
    //lv_100ask_file_explorer_open_dir(file_explorer, "D:");
    //lv_100ask_file_explorer_open_dir(file_explorer, "D:/100ask");
    lv_100ask_file_explorer_open_dir(file_explorer, "D:/100ask/tmp");

#if LV_100ASK_FILE_EXPLORER_QUICK_ACCESS
    // [windows]设置快速访问路径，跨磁盘访问要先设置 lv_conf.h 中的 LV_FS_WIN32_LETTER
    // 但是由于windows系统的权限问题，我们不能访问系统盘，这里只是给出示例，模拟windows资源管理器中的快速访问栏
    lv_100ask_file_explorer_set_quick_access_path(file_explorer, LV_100ASK_EXPLORER_HOME_DIR, "C:/Users/Public/Desktop");
    lv_100ask_file_explorer_set_quick_access_path(file_explorer, LV_100ASK_EXPLORER_VIDEO_DIR, "C:/Users/Public/Videos");
    lv_100ask_file_explorer_set_quick_access_path(file_explorer, LV_100ASK_EXPLORER_PICTURES_DIR, "C:/Users/Public/Pictures");
    lv_100ask_file_explorer_set_quick_access_path(file_explorer, LV_100ASK_EXPLORER_MUSIC_DIR, "C:/Users/Public/Music");
    lv_100ask_file_explorer_set_quick_access_path(file_explorer, LV_100ASK_EXPLORER_DOCS_DIR, "C:/Users/Public/Documents");
#endif

#else

#if defined(ESP_PLATFORM)
    lv_100ask_file_explorer_open_dir(file_explorer, "S:/");
#else
    // [linux]打开根目录
    lv_100ask_file_explorer_open_dir(file_explorer, "A:");
#endif

#if LV_100ASK_FILE_EXPLORER_QUICK_ACCESS
    // [linux]设置快速访问路径
    // lv_100ask_file_explorer_set_quick_access_path(file_explorer, LV_100ASK_EXPLORER_HOME_DIR, "/root");
    // lv_100ask_file_explorer_set_quick_access_path(file_explorer, LV_100ASK_EXPLORER_VIDEO_DIR, "/root/Videos");
    // lv_100ask_file_explorer_set_quick_access_path(file_explorer, LV_100ASK_EXPLORER_PICTURES_DIR, "/root/Pictures");
    // lv_100ask_file_explorer_set_quick_access_path(file_explorer, LV_100ASK_EXPLORER_MUSIC_DIR, "/root/Music");
    // lv_100ask_file_explorer_set_quick_access_path(file_explorer, LV_100ASK_EXPLORER_DOCS_DIR, "/root/Documents");
#endif

#endif

    lv_obj_add_event_cb(file_explorer, file_explorer_simple_test_event_handler, LV_EVENT_VALUE_CHANGED, NULL);

}


/*=====================
 * Other functions
 *====================*/

void print_file_content(const char *file_path) {
    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        printf("Failed to open file: %s\n", file_path);
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        printf("%s", line);
    }

    fclose(file);
}

static const char* get_extension(const char* file_path) {
    const char *dot = strrchr(file_path, '.');
    if(!dot || dot == file_path) return "";
    return dot + 1;
}

static int open_text_file(const char *file_path) {
    int argc = 2;
    char* argv[3];
    argv[0] = "lv_text_editor.bin";
    argv[1] = (char*)file_path;
    argv[2] = NULL;
    return create_process(argc, argv,NULL);
}

static int open_binary_file(const char *file_path) {
    int argc = 1;
    char* argv[2];
    argv[0] = file_path;
    argv[1] = NULL;
    return create_process(argc, argv,NULL);
}


/**********************
 *   STATIC FUNCTIONS
 **********************/

static void file_explorer_simple_test_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_VALUE_CHANGED) {
        char * cur_path =  lv_100ask_file_explorer_get_cur_path(obj);
        char * sel_fn = lv_100ask_file_explorer_get_sel_fn(obj);
        LV_LOG_USER("%s%s", cur_path, sel_fn);
        // Save the content to a string
        char file_path[LV_100ASK_FILE_EXPLORER_PATH_MAX_LEN];
        snprintf(file_path, LV_100ASK_FILE_EXPLORER_PATH_MAX_LEN, "%s%s", cur_path, sel_fn);

        // Remove the prefix "A:"
        char* file_path_without_prefix = file_path + 2;
        if (strcmp(get_extension(file_path_without_prefix), "txt") == 0) {
            open_text_file(file_path_without_prefix);
        } else if (strcmp(get_extension(file_path_without_prefix), "bin") == 0){
            open_binary_file(file_path_without_prefix);
        }
        else {
            print_file_content(file_path_without_prefix);
        }
    }
    
}

#endif  /*SIMPLE_TEST*/
