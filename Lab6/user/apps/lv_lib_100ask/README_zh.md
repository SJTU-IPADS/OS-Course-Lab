
# 前言
`lv_lib_100ask` 是基于lvgl库的各种开箱即用的方案参考或对lvgl库各种组件的增强接口。

# lib列表
|  路径   | 说明  |
|  ----  | ----  |
| [src/lv_100ask_pinyin_ime](src/lv_100ask_pinyin_ime/README_zh.md) | lvgl组件增强接口(拼音输入法) |
| [src/lv_100ask_sketchpad](src/lv_100ask_sketchpad/README_zh.md) | lvgl组件增强接口(画板) |
| [src/lv_100ask_page_manager](src/lv_100ask_page_manager/README_zh.md) | lvgl组件增强接口(页面管理器) |
| [src/lv_100ask_calc](src/lv_100ask_calc/README_zh.md) | lvgl组件增强接口(计算器) |
| [src/lv_100ask_2048](src/lv_100ask_2048/README_zh.md) | lvgl方案参考(2048小游戏) |
| [src/lv_100ask_memory_game](src/lv_100ask_memory_game/README_zh.md) | lvgl方案参考(数字对拼图小游戏) |
| [src/lv_100ask_file_explorer](src/lv_100ask_file_explorer/README_zh.md) | lvgl方案参考(文件浏览器) |
| [src/lv_100ask_nes](src/lv_100ask_nes/README_zh.md) | lvgl方案参考(nes模拟器) |
| [src/lv_100ask_screenshot](src/lv_100ask_screenshot/README_zh.md) | lvgl截屏功能 |
| more todo...  | more todo... |

# 使用方法

1. 克隆本仓库到你的项目目录下： `https://gitee.com/weidongshan/lv_lib_100ask.git`
2. `lv_lib_100ask` 目录应该和项目的lvgl目录同级(建议)。与 `lv_conf.h` 类似，本项目也有一个配置文件，名称为 `lv_lib_100ask_conf.h`
3. 复制 `lv_lib_100ask/lv_lib_100ask_conf_template.h` 到 `lv_lib_100ask` 同级的目录上
4. 将 `lv_lib_100ask_conf_template.h` 重命名为 `lv_lib_100ask_conf.h` 。(上面两步操作即： "cp lv_lib_100ask/lv_lib_100ask_conf_template.h ./lv_lib_100ask_conf.h")
5. 将第一个 #if 0 更改为 #if 1 以启用文件的内容
6. 启用或禁用功能

# 更多资源
这是一个开源的项目，非常欢迎大家一起参与改进！

- lvgl官网：[https://lvgl.io](https://lvgl.io)
- lvgl官方文档：[https://docs.lvgl.io](https://docs.lvgl.io)
- 百问网lvgl中文文档：[http://lvgl.100ask.net](http://lvgl.100ask.net)
- 百问网lvgl论坛：[https://forums.100ask.net/c/13-category/13](https://forums.100ask.net/c/13-category/13)
- 百问网lvgl学习交流群：[http://lvgl.100ask.net/master/contact_us/index.html](http://lvgl.100ask.net/master/contact_us/index.html)
- 百问网lvgl视频教程：
    - [https://www.bilibili.com/video/BV1Ya411r7K2](https://www.bilibili.com/video/BV1Ya411r7K2)
    - [https://www.100ask.net/detail/p_61c5a317e4b0cca4a4e8b6f1/6](https://www.100ask.net/detail/p_61c5a317e4b0cca4a4e8b6f1/6)
