#include "gui_guider.h"
#include "events_init.h"
#include "read_and_save.h"
#include "error_msg.h"
#include "input_device.h"
#include <time.h>
#include <stdlib.h>
#include <lv_drivers/wayland/wayland.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#define H_RES (640)
#define V_RES (480)

lv_ui guider_ui;
lv_disp_t* disp;
const char* filename = "";

int main(int argc, char** argv)
{
        int ret;
        filename = argv[1];
        lv_init();
        lv_wayland_init();
        disp = lv_wayland_create_window(H_RES, V_RES, "Window Title", NULL);
        lv_disp_set_default(disp);
        init_input_device();
        setup_ui(&guider_ui);
        events_init(&guider_ui);
        
        do {
                ret = open_file();
                if (ret < 0) {
                        create_modal_message_box("Error Opening File",
                                                 strerror(errno), true);
                        break;
                }
                char* buffer = read_file();
                if (buffer == NULL) {
                        create_modal_message_box("Error Reading File",
                                                 strerror(errno), true);
                        break;
                }
                lv_textarea_set_text(guider_ui.screen_ta_1, buffer);
        } while (0);
        while (lv_wayland_window_is_open(disp)) {
                usleep(1000 * lv_wayland_timer_handler()); //! run lv task at
                                                           //! the max speed
        }
        lv_wayland_deinit();
        return 0;
}