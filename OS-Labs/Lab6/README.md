# 实验：GUI
[Lab Tutorial](docs/gui-lab-tutorial/main.pdf)

QEMU 使用 `make` 构建、`make qemu` 启动。`ramdisk-rpi4/chcore_shell.bin`
提供适配 raspi4 控制台的 shell，构建时会自动打包进镜像。

Lab3/Lab5 的 shell 固定等待 57 号串口中断，而 Lab6 的 raspi4 内核使用
mini UART，且未启用接收中断，直接复制旧 shell 会出现提示符可见但无法输入的问题。
这里的 shell 复用原有输入处理函数，每轮休眠 1 ms 后继续读取串口。
此兼容补丁已在 raspi4b QEMU 中验证，尚未验证树莓派实机。

可使用原始 Lab5 二进制重新生成（脚本会校验源文件 SHA-256）：

```bash
python3 tools/patch_rpi4_shell.py ../Lab5/ramdisk/chcore_shell.bin ramdisk-rpi4/chcore_shell.bin
```
