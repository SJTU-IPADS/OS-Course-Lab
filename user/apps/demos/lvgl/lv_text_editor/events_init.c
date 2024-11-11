/*
 * Copyright 2024 NXP
 * SPDX-License-Identifier: MIT
 * The auto-generated can only be used on NXP devices
 */

#include "events_init.h"
#include <stdio.h>
#include "lvgl.h"
#include <stdlib.h>
#include <errno.h>
#include <lv_drivers/wayland/wayland.h>
#include "read_and_save.h"
#include "error_msg.h"

extern const char* filename;
extern lv_ui guider_ui;

void events_init(lv_ui *ui)
{
	
}

static void screen_btn_2_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		//Save the file
		if(save_file(lv_textarea_get_text(guider_ui.screen_ta_1)) < 0) {
			create_modal_message_box("Error Saving File", strerror(errno), false);
		}
		else {
			char text[100];
			sprintf(text, "File %s saved successfully!", filename);
			create_modal_message_box("File Saved", text, false);
		}
	}
		break;
	default:
		break;
	}
}

static void screen_btn_3_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
	{
		// Exit
		lv_wayland_close_window(lv_disp_get_default());
	}
		break;
	default:
		break;
	}
}

void events_init_screen(lv_ui *ui)
{
	lv_obj_add_event_cb(ui->screen_btn_2, screen_btn_2_event_handler, LV_EVENT_ALL, NULL);
	lv_obj_add_event_cb(ui->screen_btn_3, screen_btn_3_event_handler, LV_EVENT_ALL, NULL);
}
