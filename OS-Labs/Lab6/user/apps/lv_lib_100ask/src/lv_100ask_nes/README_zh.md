
<h1 align="center"> lv_100ask_nes</h1>

<p align="center">
<img src="lv_100ask_nes_demo.gif">
</p>
<p align="center">
lv_100ask_nes 基于lvgl实现了nes模拟器的显示和控制。
</p>

[English](README.md) | **中文** |


# 前言

不能在裸机上运行 lv_100ask_nes ，因为 lv_100ask_nes 涉及多线程(多任务)，所以需要比如在 freeRTOS、RT-Thread 或者 Linux等平台上才能运行 lv_100ask_nes。

**lv_100ask_nes** 特性：

- 简单方便的控制接口
- 跨平台、可移植性强
- 自定义前端
- 声音输出(TODO)
- more todo...

`lv_100ask_nes` 使用起来非常简单，后续自定义拓展功能也很方便，更多新功能敬请期待。

![](./lv_100ask_nes_demo.gif)

# 演示视频

- [https://www.youtube.com/watch?v=q98tbV9r7bc](https://www.youtube.com/watch?v=q98tbV9r7bc)
- [https://www.bilibili.com/video/BV1gt4y1L7Aw](https://www.bilibili.com/video/BV1gt4y1L7Aw)

# 使用方法

需要注意的是：
1. 准备好ROM文件并告诉 lv_100ask_nes_test 存放ROM的目录；
2. 如果使能了 LV_MEM_CUSTOM 请确保你有可用的内存运行ROM；
3. 其他更多细节请参考 **lv_lib_100ask/test/lv_100ask_nes_test** 的示例代码。

## 视频教程

- T113: [https://www.bilibili.com/video/BV1a94y1X7gP](https://www.bilibili.com/video/BV1a94y1X7gP)
- More TODO

# 获取现成项目

- Code:blocks project: [https://forums.100ask.net/t/topic/397](https://forums.100ask.net/t/topic/397)
- Linux frame buffer project: [https://forums.100ask.net/t/topic/397](https://forums.100ask.net/t/topic/397)
- Eclipse with SDL driver: [https://forums.100ask.net/t/topic/397](https://forums.100ask.net/t/topic/397) 

# 关于我们
这是一个开源的项目，非常欢迎大家参与改进lv_100ask_nes项目！

交流论坛：[https://forums.100ask.net](https://forums.100ask.net)
