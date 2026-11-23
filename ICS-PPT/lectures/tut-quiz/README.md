# ICS 辅导课 · Quiz 题目

本讲是 Quiz 第 1 ~ 5 题的题面。

讲义 id 是 `tut-quiz`，与目录名相同。封面与讲义的标题是「Quiz 题目」，PDF 与 PPTX 的文件名由标题得到。2026-11-23 原来的第二部分（mini-cpu 的组相联缓存）与第三部分（mini-ollama 的 KV Cache）拆成讲义 [`../tut-cache-lab`](../tut-cache-lab/) 与 [`../tut-kvcache-lab`](../tut-kvcache-lab/)，拆分前的目录在 `../../build/scratch-split/before/tut-quiz/`。

## 构建

```bash
python3 -m lecturekit.cli inspect lectures/tut-quiz
python3 -m lecturekit.cli view    lectures/tut-quiz --watch
python3 -m lecturekit.cli render  lectures/tut-quiz --to pdf
python3 -m lecturekit.cli render  lectures/tut-quiz --to pptx
sh lectures/tut-quiz/diagrams/render.sh
```

## 两种语言

中文是基线（写在 Python 里），英文是 `i18n/en.toml` 翻译覆盖层。代码块的内容不翻译，其中的注释是英文。

## 结构

封面之后是一个部分「Quiz 题目」（`lecture.section(..., id="quiz")`），其中第 4、5 题各是一个小节。讲义只有这一个部分，部分的衔接页只会重复封面的标题，所以没有衔接页，封面之后直接是第 1 题。没有部分的讲义（比如 `ICS2/tutorial-agent-lab`）同样从封面直接进入内容。共 26 张幻灯片。页面函数在 `pages.py` 中，顺序与 `lecture.py` 相同，函数名把下划线换成连字符就是页面的 id。辅助函数与常量以 `_quiz_` 开头。`pages.py` 以一段注释开头，写明讲义的设计与材料的来源。每个页面函数的注释写明它承接上一页的方式。

| 题 | 幻灯片 | 页面 |
| --- | --- | --- |
| 封面 | 1 | `cover` |
| 第 1 题 | 2–5 | `q1-code` · `q1-push` · `q1-ret` · `q1-input` |
| 第 2 题 | 6–11 | `q2-gets` · `q2-frame` · `q2-pass` · `q2-nx` · `q2-rop` · `q2-canary` |
| 第 3 题 | 12–13 | `q3-convention` · `q3-callee` |
| 第 4 题（小节） | 14–20 | `q4-pipe-hazard` · `q4-pipe-cycles` · `q4-rob` · `q4-rob-rules` · `q4-rob-q1` · `q4-rob-answer`（第（2）题第 1 问的答案）· `q4-rob-branch` |
| 第 5 题（小节） | 21–26 | `q5-cpu-count` · `q5-cpu-bound` · `q5-cpu-simd` · `q5-gpu-pipeline` · `q5-gpu-batch` · `q5-disk` |

页码随修改变化，以上是 2026-11-23 的页码。

## 题面

题面取自 `../../exames/quiz.md`（`quiz.pdf` 的源文件），题号、分值、文字、代码、表与图照原卷，不加提示，不放答案。唯一的例外是第 4 题（2）第 1 问之后的一页（`q4-rob-answer`）：它把 `../../exames/quiz-answer.md` 中各指令进入、执行、完成与提交的周期，换成周期 1 ~ 9 结束时 ROB 中按程序顺序排列的条目与状态，周期 9 一行是答案；周期 7、8 只有 4 个条目的原因写在讲稿中。

一道题可以分成几页，一页放一个小问，或几个放得下的小问。小问用到的代码、寄存器与内存的值、表、规则、假设与图尽量放在同一页，各页重复它用到的材料，表只留小问用到的行，规则与假设只留小问用到的条目。主题的字号固定，有几页放不下小问用到的全部材料，缺的部分在同一题的前一页或后一页，`pages.py` 开头的注释逐页列出缺什么。

第 4、5 题各是一个小节，小节名是题目的标题，各页标题是题目中所在部分的标题。

## 插图

插图的源文件在 `diagrams/`，`diagrams/render.sh` 运行全部脚本，生成 `assets/*.svg`，再用 `check_bounds.py` 检查文字是否超出画布。`assets/` 中的图不手改。全部图共用 `diagrams/svgkit.py`（取自 `9-gpu`）。`save()` 同时按 `svgkit.py` 中 `SCHEMES` 的各张色表替换颜色与淡出的不透明度，为每张图生成画布相同、透明底的 `<图名>.<方案>.svg`（比如 `q-rob.dark.svg`），增加一个方案只需要增加一张色表。新增的颜色在 `svgkit.py` 中定义并写入每张色表，遇到表中没有的颜色时 `save()` 报错。

| 前缀 | 脚本 | 说明 |
| --- | --- | --- |
| `q_` | `q_rob.py`、`q_cpu_memory.py`、`q_gpu_memory.py`、`q_gpu_disk.py` | 由 `../../exames/diagrams/` 复制，只改了导入的 `svgkit.py` 与输出路径，生成的图与原卷的图相同 |
