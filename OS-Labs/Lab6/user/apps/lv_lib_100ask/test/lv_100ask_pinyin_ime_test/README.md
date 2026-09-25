# 文件说明

| 名称 | 说明 |
| ------ | ------- |
| lv_font_source_han_sans_bold_14(4E00-9FA5).c | 基本汉字(20902字)  |
|lv_font_source_han_sans_bold_14(常用3500汉字+所有拉丁字母).c | 常用3500汉字+所有拉丁字母 |
|zh_cn_pinyin_dict.json| 拼音词库json文件 |


# 使用方法
## [字库文件](http://lvgl.100ask.net/master/tools/fonts-zh-source.html)
解压：chinese-fonts.zip
请将 `lv_font_source_han_sans_bold_14(4E00-9FA5).c` 修改为 `lv_font_source_han_sans_bold_14.c`；

或者将 将 `lv_font_source_han_sans_bold_14(常用3500汉字+所有拉丁字母).c` 修改为 `lv_font_source_han_sans_bold_14.c`

> 选择用哪一个取决于你的板载资源或者项目需求，一般 `lv_font_source_han_sans_bold_14(常用3500汉字+所有拉丁字母).c`  已经够应对大部分需求。

使用示例：

- 将 `lv_font_source_han_sans_bold_14.c` 其复制到你的项目文件中，并包含到项目；
- 在你的项目应用程序的 C 文件中，将字体声明为：`extern lv_font_t my_font_name;` 或：`LV_FONT_DECLARE(my_font_name);`
- 我们还需要下面这两个步骤，应用字体文件：
  - lv_pinyin_ime_set_text_font(&lv_font_source_han_sans_bold_14, 0);   // 应用字体文件到 lv_pinyin_ime
  - lv_obj_set_style_text_font(ta, &lv_font_source_han_sans_bold_14, 0);   // 应用字体文件到 lv_textarea ，这里使用本地样式，你也可以使用普通样式


> 你也可以使用其他的字体文件，这里给出只是为了方便测试使用 lv_pinyin_ime，查看[这里](http://lvgl.100ask.net/master/tools/fonts-zh-source.html)获取更多[字库文件](http://lvgl.100ask.net/master/tools/fonts-zh-source.html)。



## 词库文件

词库文件可以自己制作，在 lv_pinyin_ime 的源码中已经内置了一个词库文件( /lv_lib_100ask/lv_100ask_pinyin_ime/src/zh_cn_pinyin_dict.c )，这里给出一个词库文件是为了后面的功能做准备：可以通过在运行时加载本地词库文件，而不是编译到代码中(节省空间)。
