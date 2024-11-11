⚠️  lv_100ask_pinyin_ime已合入LVGL主线仓库，因此 lv_lib_100ask **不再更新** lv_100ask_pinyin_ime，请移步：

- 文档地址(有中文)：[https://docs.lvgl.io/master/others/ime_pinyin.html](https://docs.lvgl.io/master/others/ime_pinyin.html)
- 源码地址：[https://github.com/lvgl/lvgl/tree/master/src/extra/others/ime](https://github.com/lvgl/lvgl/tree/master/src/extra/others/ime)
- 使用示例：[https://docs.lvgl.io/master/others/ime_pinyin.html#example](https://docs.lvgl.io/master/others/ime_pinyin.html#example)


<h1 align="center"> lv_100ask_pinyin_ime</h1>

<p align="center">
<img src="./lv_100ask_pinyin_ime.gif">
</p>
<p align="center">
lv_100ask_pinyin_ime 是在 lv_keyboard 的基础上编写的一个自定义部件，它和 lv_keyboard 没有什么区别，只是增加了支持中文拼音输入法(拼音)的功能。
</p>


[English](README.md) | **中文** |


# 前言
`lv_100ask_pinyin_ime` 是在 lv_keyboard 的基础上编写的一个自定义部件（创建接口为：  `lv_100ask_pinyin_ime_create(lv_obj_t *parent));` ），它和 [lv_keyboard](https://docs.lvgl.io/master/widgets/extra/keyboard.html) 没有什么区别，只是增加了支持中文拼音输入法的功能。

所以我们将其称为：**支持中文拼音输入法的LVGL键盘(lv_keyboard)部件增强插件**。

正常来说，只要是lvgl能运行的环境 `lv_100ask_pinyin_ime` 也能够运行！影响因素主要有两点：使用的字库文件大小和使用的词库大小。

`lv_100ask_pinyin_ime` 使用起来非常简单，后续自定义拓展功能也很方便，更多功能敬请期待。

![](./lv_100ask_pinyin_ime.gif)


# 使用方法
参考 **lv_lib_100ask/test/lv_100ask_pinyin_ime_test** 的示例。

# 自定义词库
如果不想使用内置的拼音词库，可以使用自定义的词库。
或者觉得内置的拼音词库消耗的内存比较大，也可以使用自定义的词库。

自定义词库非常简单。
- 首先，在 `lv_lib_100ask_conf.h` 中将 `LV_100ASK_PINYIN_IME_ZH_CN_PIN_YIN_DICT` 设置为 `0`
- 然后，按照下面这个格式编写词库。

## 词库格式

必须按照下面的格式，编写自己的词库：

```c
lv_100ask_pinyin_dict_t your_pinyin_dict[] = {
            { "a", "啊阿呵吖" },
            { "ai", "埃挨哎唉哀皑蔼矮碍爱隘癌艾" },
            { "ba", "芭捌叭吧笆八疤巴拔跋靶把坝霸罢爸扒耙" },
            { "cai", "猜裁材才财睬踩采彩菜蔡" },
            /* ...... */
            { "zuo", "昨左佐做作坐座撮琢柞"},
            {NULL, NULL}

```

**最后一项**必须要以 '{NULL, NULL}' 结尾，否则将无法正常工作。

## 应用新词库

按照上面的词库格式编写好词库之后，只需要调用这个函数即可设置使用你的词库：

```c
    lv_obj_t * pinyin_ime = lv_100ask_pinyin_ime_create(lv_scr_act());
    lv_100ask_pinyin_ime_set_dict(pinyin_ime, your_pinyin_dict);
```

**注意**： 必须在设置好词库之后再使用 `lv_100ask_pinyin_ime`


# 关于我们
这是一个开源的项目，非常欢迎大家参与改进lv_100ask_pinyin_ime 项目！
作者联系邮箱: smilezyb@163.com