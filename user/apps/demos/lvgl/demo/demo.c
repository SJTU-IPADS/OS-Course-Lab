#include <unistd.h>
#include <stdio.h>
#include "lv_conf.h"
#include "../demos/lv_demos.h"
#include "lv_drivers/wayland/wayland.h"
#include <time.h>
#include <stdlib.h>

#define H_RES (800)
#define V_RES (480)
lv_disp_t * disp;
void set_full_screen(void){
    static bool fullscreen = false;
    lv_wayland_window_set_fullscreen(disp, !fullscreen);
    fullscreen = !fullscreen;
}

void load_wallpaper(){
    lv_obj_t* avatar = lv_img_create(lv_scr_act());
    lv_img_set_src(avatar, "A:/imgs/wallpaper.bin");
    lv_obj_set_size(avatar, 1920, 1080);
}

int main(void)
{    
    lv_init();
    lv_wayland_init();
    printf("[DEMO]: wayland initialized\n");

    disp = lv_wayland_create_window(H_RES, V_RES, "Window Title", NULL);
    printf("[DEMO]: window created\n");
    lv_disp_set_default(disp);
    printf("[DEMO]: set default\n");
    lv_demo_widgets();
    //lv_demo_music();
    printf("[DEMO]: demo started\n");
    while(lv_wayland_window_is_open(disp)){
        usleep(1000 * lv_wayland_timer_handler());                 //! run lv task at the max speed
    }
    lv_wayland_deinit();
}