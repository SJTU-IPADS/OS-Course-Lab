# Lab0 拆炸弹

> [!WARNING]
> 在完成本实验之前，请务必将你的学号填写在student-number.txt当中，否则本lab实验的成绩将计为0分

## Makefile 讲解

- `make bomb`: 使用student-number.txt提供的学号，生成炸弹，如果您不是上海交通大学的学生可以自行随意填写。
- `make qemu`: 使用qemu-aarch64二进制模拟运行炸弹
- `make qemu-gdb`: 使用qemu-aarch64提供的gdb server进行调试
- `make gdb`: 使用仓库目录自动生成的$(GDB)定义连接到qemu-aarch64的gdb-server进行调试

## 教程

> [!TIP]
> 请阅读[tools-tutorial.pdf](docs/tools-tutorial.pdf)了解如何使用调试工具  
> 请仔细阅读[instruction.pdf](docs/instruction.pdf)了解如何完成本实验

## 评分与提交规则

> [!IMPORTANT]
> 运行 `make grade` 来得到本实验的分数  
> 运行 `make submit` 会在检查student-number.txt内容之后打包必要的提交文件

## 附录

-  线上教程[https://www.bilibili.com/video/BV1q94y1a7BF/?vd_source=63231f40c83c4d292b2a881fda478960]
