This folder contains GUI applications using LVGL. Except for the specially mentioned run commands, the rest of the programs do not require additional parameters. This folder requires `CHCORE_SERVER_GUI` and `CHCORE_LIB_GRAPHIC` support.

## demo
A basic lvgl demo, using the official Demo Widget, combined with the framework related to the Wayland Driver. All LVGL applications on ChCore should follow this framework.

## demos examples
A folder in the LVGL official repository, mainly providing some examples of LVGL.

## lv_gba_emu
A GBA emulator project using the LVGL framework and the open-source GBA emulator VBANext. Based on https://github.com/FASTSHIFT/lv_gba_emu.
### How to run
Note, because LVGL defaults to using file paths with prefixes, your file path should carry an "A:" when this application is running.
If your ROM file is in /user/gba_rom/Pokemon_Emerald.gba, then your run command should be  
`gba_emu.bin -f A:user/gba_rom/Pokemon_Emerald.gba`

## lv_100_ask
Open source project https://github.com/100askTeam/lv_lib_100ask, where all the code is compiled into a library. In addition, using this library, lv_file and other executable files were implemented based on the framework mentioned above.

## lv_text_editor
A text editor. The interface is written using the third-party tool gui_guider.
### How to run
Because this file does not use LVGL's file API, but directly uses the API in libC, the file path of this program does not need to add a prefix. For example, to open /user/hello.txt, the run command should be  
`lv_text_editor.bin /user/hello.txt`
