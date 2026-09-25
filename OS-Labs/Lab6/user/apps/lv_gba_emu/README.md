# LVGL Game Boy Advance Emulator

![image](https://github.com/FASTSHIFT/lv_gba_emu/blob/main/images/lv_gba_emu_2.png)

* GUI: https://github.com/lvgl/lvgl
* GBA Emulator: https://github.com/libretro/vba-next
* Test ROM: https://github.com/XProger/OpenLara

## Feature
* The emulator kernel is based on [vba-next](https://github.com/libretro/vba-next) and does not depend on any third-party libraries.
* Decoupled from the OS, only relying on lvgl's memory allocation and file access interface.
* Support to use GBA framebuffer directly as `lv_canvas` buffer, zero copy overhead.
* Audio output.
* Frame rate control.
* Multiple input device.
* Virtual key.
* Memory usage optimization(~800KB + ROM size).

## To Do

- [ ] Game saves.
- [ ] Game Launcher.

## Clone
```bash
git clone https://github.com/FASTSHIFT/lv_gba_emu.git --recurse-submodules
```

## Build & Run
```bash
mkdir build
cd build
cmake ..
make -j
./gba_emu -f ../rom/OpenLara.gba
```
## Key Mapping
### SDL2
|KeyBoard|GBA|
|-|-|
|Backspace|Select|
|Tab|Start|
|Up|Up|
|Down|Down|
|Left|Left|
|Right|Right|
|X|B|
|Z|A|
|L|L|
|R|R|
