#include "lv_lib_100ask.h"
#include "test/lv_100ask_calc_test/simple_test.h"
#include <unistd.h>
#include <stdio.h>
#include "lv_conf.h"
#include "../demos/lv_demos.h"
#include "lv_drivers/wayland/wayland.h"
#include <time.h>
#include <stdlib.h>

#define H_RES (600)
#define V_RES (650)
lv_disp_t * disp;


int main(void)
{    
    lv_init();
    lv_wayland_init();
    printf("[DEMO]: wayland initialized\n");

    disp = lv_wayland_create_window(H_RES, V_RES, "Window Title", NULL);
    printf("[DEMO]: window created\n");
    lv_disp_set_default(disp);
    printf("[DEMO]: set default\n");
    lv_100ask_calc_simple_test();
    //lv_demo_music();
    printf("[DEMO]: demo started\n");
    while(lv_wayland_window_is_open(disp)){
        usleep(1000 * lv_wayland_timer_handler());                 //! run lv task at the max speed
    }
    lv_wayland_deinit();
    return 0;
}