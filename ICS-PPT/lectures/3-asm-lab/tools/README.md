# tools：终端截图的生成

`assets/bomb_*.png` 是 bomb 在终端中的画面：`termshot.py` 在一个固定大小的伪终端上运行程序，按步骤输入，把程序写出的字节回放给终端仿真库 pyte，再用系统的 DejaVu Sans Mono 字体逐格画成 PNG（深色背景，程序自己的颜色）。`shots.sh` 列出四张图各自的步骤。

## 重新生成

1. 一个装有 `pyte` 与 `Pillow` 的 Python：`python3 -m venv build/scratch-bomb/venv && build/scratch-bomb/venv/bin/pip install pyte pillow`（在仓库根目录执行；`shots.sh` 默认用这个路径，`PYTHON=` 指定别的）。
2. `examples/bomb`：不连接服务器、8 个阶段的变体全为 0 的演示 bomb，在实验的源码树中生成：`make bomb ID=000000000001 VARIANTS=00000000 SERVER= O=<目录>`，复制为 `examples/bomb`。阶段的画面、爆炸、拆除后的画面三张图用它。
3. 计分板一张图需要一个在 `127.0.0.1` 上启动的课程服务器与它发出的 bomb：按 `solve-bomb-server/README.md` 启动 `server.py --bind 127.0.0.1 --port <端口> --public 127.0.0.1:<端口> --source <源码树> --data <数据目录>`（没有证书时它自己生成自签名证书），用学生的 `Makefile` 领取几个演示学号的 bomb（`CURL_CA_BUNDLE=<数据目录>/cert.pem make bomb ID=000000000001 SERVER=https://127.0.0.1:<端口>`），各运行几次填上计分板，把其中一个的路径作为 `shots.sh` 的参数。截图上只能出现演示学号。
4. 在讲义目录中执行 `sh tools/shots.sh [计分板用的 bomb]`。

每个被启动的程序最多使用 512 MB 内存，终端关闭后仍不结束的被杀死；结束后用 `pgrep -af bomb` 核对没有残留的进程，演示服务器用完后停止。

## termshot.py 的用法

```
python3 tools/termshot.py --out 图.png [--rows 24] [--cols 80] [--font-size 20]
    [--wait 文字] [--type 文字] [--key enter|ctrl-d|…] [--pause 秒] … -- 程序 [参数…]
```

`--wait`、`--type`、`--key`、`--pause` 按给出的顺序执行。程序离开备用屏幕（`ESC [?1049 l`）之后的字节不画，因此图是程序最后画满的那一屏；`--keep-raw 文件` 保存程序写出的全部字节，供核对。
