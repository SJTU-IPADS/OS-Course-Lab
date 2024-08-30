# 贡献指南

## 代码规范

> [!IMPORTANT] Commit
> 我们参照Conventional Commit构建了Pull Request的Blocker，并且关闭了对主分支的直接Push，请确保你的Commit符合Conventional Commit规范

> [!IMPORTANT] Github
> 我们使用Github Issues跟踪所有的问题，如果你在实验过程中产生了任何预期以外的错误，欢迎提交Issues.

> [!NOTE] MDBook
> 我们使用基于MdBook构建Markdown文档体系，你可以参照`.github/github-pages.yaml`中的下载指示，将所有的Mdbook及其所需要的所有**预处理器**，都安装到你的系统环境路径中。如果你对文档方面有任何的更正，你可以在`Pages/SUMMARY.md`中找到实验手册的文档结构以及对应的所有文件。更改后在仓库根目录你可以运行`mdbook serve`，并访问`localhost:3000`查看编译后的文档。我们也使用`markdownlint`对所有文档开启的CI检查，请确保提交后能够通过CI.

## 工具链使用

由于工具链版本问题，可能会导致在不同版本工具链编译的情况下导致在不同Release版本所链接的系统镜像无法正常工作的情况，请确保开发过程中使用与`.devcontainer/Dockerfile`即ubuntu 20.04.6的相同版本的交叉编译链进行预编译，本仓库对所有Lab的正确答案的构建同样开启了CI检查，如果发现在不同Release版本下无法通过，请检查你所使用的工具链是否符合预期。
