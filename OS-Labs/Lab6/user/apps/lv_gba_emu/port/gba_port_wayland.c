#include "port.h"


#include "../gba_emu/gba_emu.h"
#include <lvgl.h>


static uint32_t gba_input_update_cb(void* user_data)
{
    static const int key_map[] = {
        'x', /* GBA_JOYPAD_B */
        'y', /* GBA_JOYPAD_Y */
        KeyBackspace, /* GBA_JOYPAD_SELECT */
        KeyTabulator, /* GBA_JOYPAD_START */
        KeyUp, /* GBA_JOYPAD_UP */
        KeyDown, /* GBA_JOYPAD_DOWN */
        KeyLeft, /* GBA_JOYPAD_LEFT */
        KeyRight, /* GBA_JOYPAD_RIGHT */
        'z', /* GBA_JOYPAD_A */
        'b', /* GBA_JOYPAD_X */
        'l', /* GBA_JOYPAD_L */
        'r', /* GBA_JOYPAD_R */
        -1, /* GBA_JOYPAD_L2 */
        -1, /* GBA_JOYPAD_R2 */
        -1, /* GBA_JOYPAD_L3 */
        -1 /* GBA_JOYPAD_R3 */
    };

    uint32_t key_state = 0;

    for (int i = 0; i < sizeof(key_map) / sizeof(key_map[0]); i++) {
        if (lv_port_keystate[key_map[i]]) {
            key_state |= (1 << i);
        }
    }

    return key_state;
}

void gba_port_init(lv_obj_t* gba_emu)
{
    lv_gba_emu_add_input_read_cb(gba_emu, gba_input_update_cb, NULL);
}

