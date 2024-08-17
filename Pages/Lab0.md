# Lab0：拆炸弹

<!-- toc -->

## 简介

在实验 0 中，你需要通过阅读汇编代码以及使用调试工具来拆除一个
二进制炸弹程序。本实验分为两个部分：第一部分介绍拆弹实验的基本知
识，包括 ARM 汇编语言、QEMU 模拟器、GDB 调试器的使用；第二部分
需要分析炸弹程序，推测正确的输入来使得炸弹程序能正常退出。

> [!WARNING]
> 在完成本实验之前，请务必将你的学号填写在`student-number.txt`当中，否则本lab实验的成绩将计为0分

## Makefile 讲解

- `make bomb`: 使用student-number.txt提供的学号，生成炸弹，如果您不是上海交通大学的学生可以自行随意填写。
- `make qemu`: 使用qemu-aarch64二进制模拟运行炸弹
- `make qemu-gdb`: 使用qemu-aarch64提供的gdb server进行调试
- `make gdb`: 使用仓库目录自动生成的$(GDB)定义连接到qemu-aarch64的gdb-server进行调试

## 评分与提交规则

本实验你只需要提交`ans.txt`以及`student-number.txt`即可

> [!IMPORTANT]
> 运行 `make grade` 来得到本实验的分数  
> 运行 `make submit` 会在检查student-number.txt内容之后打包必要的提交文件

