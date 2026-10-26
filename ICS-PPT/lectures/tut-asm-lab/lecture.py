"""辅导课「汇编实验准备」：docker 与 ubuntu 容器、objdump、GDB，为实验 solve-bomb 做准备（目录与讲义 id 是 tut-asm-lab）。

前两节由 lectures/ICS1/ICS-tutorial-1-env（refs/ppts/ICS1/ICS-tutorial-1-env.pdf 的逐页转换）裁剪而来，
保留 LINUX 一节，即原 PDF 的第 8–13 页。这几页的 id 沿用原页码：原第 N 页的 id 是 sNN。
第 8 页是分节页，第 9–13 页放进同名的一节；原文是英文，英文原文在 i18n/en.toml。
封面与课程其他各讲的写法相同，只有标题不同。
「安装 docker，运行 ubuntu 容器」一节不在原 PDF 中：一个分节页与 2026-10-09 新写的四页，docker-cp 一页是 2026-10-11 加的。

后四节（使用 objdump、炸弹、使用 GDB、任务）是 2026-10-11 按辅导课课件 ICS-tutorial-5-asm.pptx（37 页）
的顺序写的：例子是实验 solve-bomb 的 bomb-light，原课件中代码的截图改为 p.demo 与 p.code，页面 id 用内容命名。
逐页对应见 README.md。
"""

from lecturekit.dsl import Lecture

import pages


lecture = Lecture(
    id="tut-asm-lab",
    title="汇编实验准备",
    ratio="16:9",
)

lecture.cover(
    "汇编实验准备",
    author="古金宇 · 廖承凡",
    time="上海交通大学 IPADS",
)

lecture.bridge("LINUX", id="s08")
with lecture.section("LINUX", id="linux") as s:
    s.page("s09", body=pages.s09)
    s.page("s10", body=pages.s10)
    s.page("s11", body=pages.s11)
    s.page("s12", body=pages.s12)
    s.page("s13", body=pages.s13)

lecture.bridge("安装 docker，运行 ubuntu 容器", id="bridge-docker")
with lecture.section("安装 docker，运行 ubuntu 容器", id="docker") as s:
    s.page("docker-install", body=pages.docker_install)
    s.page("docker-pull", body=pages.docker_pull)
    s.page("docker-run", body=pages.docker_run)
    s.page("docker-exec", body=pages.docker_exec)
    s.page("docker-cp", body=pages.docker_cp)

lecture.bridge("使用 objdump", id="bridge-objdump")
with lecture.section("使用 objdump", id="using-objdump") as s:
    s.page("objdump", body=pages.objdump)
    s.page("objdump-output", body=pages.objdump_output)
    s.page("bomb-objdump", body=pages.bomb_objdump)
    s.page("bomb-main", body=pages.bomb_main)
    s.page("bomb-calls", body=pages.bomb_calls)
    s.page("bomb-args", body=pages.bomb_args)
    s.page("bomb-strings", body=pages.bomb_strings)
    s.page("bomb-branch", body=pages.bomb_branch)
    s.page("bomb-defused", body=pages.bomb_defused)

lecture.bridge("使用 GDB", id="bridge-gdb")
with lecture.section("使用 GDB", id="using-gdb") as s:
    s.page("gdb", body=pages.gdb)
    s.page("gdb-commands", body=pages.gdb_commands)
    s.page("gdb-break", body=pages.gdb_break)
    s.page("gdb-stop", body=pages.gdb_stop)

lecture.bridge("任务", id="bridge-tasks")
with lecture.section("任务", id="tasks") as s:
    s.page("lab-solve-bomb", body=pages.lab_solve_bomb)
