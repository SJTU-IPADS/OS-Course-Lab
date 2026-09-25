
<h1 align="center"> lv_100ask_nes</h1>

<p align="center">
<img src="./lv_100ask_nes_demo.gif">
</p>
<p align="center">
lv_100ask_nes  realizes the display and control of NES simulator based on lvgl.
</p>


**English** | [中文](./README_zh.md) |


# Introduction

You can't run lv_100ask_nes on bare metal machines, because lv_100ask_nes involves multithreading (multitasking), so you need to run lv_100ask_nes on platforms such as FreeRTOS, RT thread, or Linux.

**lv_100ask_nes** features：

- Simple and convenient control interface
- Cross platform and portability
- Custom front end
- Sound output (todo)
- more todo...

`lv_100ask_nes` is very simple to use, and the subsequent custom expansion functions are also very convenient, so stay tuned for more functions.

![](/./lv_100ask_nes_demo.gif)


# Demo video

- [https://www.youtube.com/watch?v=q98tbV9r7bc](https://www.youtube.com/watch?v=q98tbV9r7bc)
- [https://www.bilibili.com/video/BV1gt4y1L7Aw](https://www.bilibili.com/video/BV1gt4y1L7Aw)
- 
# Usage

Be careful:
1. Prepare ROM files and tell lv_100ask_nes_test the directory where ROM is stored;
2. If LV_MEM_CUSTOM is enabled, please ensure that you have available memory to run ROM;
3. For more details, please refer to **lv_lib_100ask/test/lv_100ask_nes_test**.


# Platform Project

- Code:blocks project: [https://forums.100ask.net/t/topic/397](https://forums.100ask.net/t/topic/397)
- Linux frame buffer project: [https://forums.100ask.net/t/topic/397](https://forums.100ask.net/t/topic/397)
- Eclipse with SDL driver: [https://forums.100ask.net/t/topic/397](https://forums.100ask.net/t/topic/397) 


# About
This is an open project and contribution is very welcome!

Forum: [https://forums.100ask.net](https://forums.100ask.net)
