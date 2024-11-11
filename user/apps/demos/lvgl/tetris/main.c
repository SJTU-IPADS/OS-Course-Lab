#include <unistd.h>
#include <stdio.h>
#include "lv_conf.h"
#include "../demos/lv_demos.h"
#include "lv_drivers/wayland/wayland.h"
#include "tetris.h"
#include "display.h"

int main(void)
{    
    lv_init();
    lv_wayland_init();
    lv_disp_t * disp = lv_wayland_create_window(H_RES, V_RES, "Window Title", NULL);
    lv_disp_set_default(disp);
    LOG("init_field\n");
    init_field();
    LOG("lv_field\n");
    show_main_menu();
    //lv_example_grid_1();
    LOG("init done\n");
    //start_game();
    //uint32_t time_sleep;

    while(alive){
        lv_wayland_timer_handler();                 //! run lv task at the max speed
        usleep(MSPT * 1000);
        //lv_tick_inc(MSPT);
        tick();
        do_repaint();
    }

    //lv_deinit();
    lv_wayland_deinit();

    return 0;
}