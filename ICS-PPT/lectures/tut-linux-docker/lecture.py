"""辅导课：Docker Intro。

由 lectures/ICS1/ICS-tutorial-1-env（refs/ppts/ICS1/ICS-tutorial-1-env.pdf 的逐页转换）裁剪而来，
保留 LINUX 一节，即原 PDF 的第 8–13 页。这几页的 id 沿用原页码：原第 N 页的 id 是 sNN。
第 8 页是分节页，第 9–13 页放进同名的一节；原文是英文，英文原文在 i18n/en.toml。
封面与课程其他各讲的写法相同，只有标题不同。
最后一节「安装 docker，运行 ubuntu 容器」不在原 PDF 中，是 2026-10-09 新写的四页与一个分节页。
"""

from lecturekit.dsl import Lecture

import pages


lecture = Lecture(
    id="tut-linux-docker",
    title="Docker Intro",
    ratio="16:9",
)

lecture.cover(
    "Docker Intro",
    author="古金宇 · 陈榕",
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
