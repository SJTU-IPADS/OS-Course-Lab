# 变更风险审查工具目录

请在这里创建本场景所需的工具模块，每个 `.py` 文件只要暴露 `make_tool(...) -> Tool`，就会被评测器自动发现。

推荐工具：

- `read_diff.py`
- `read_patch_file.py`
- `submit_review.py`

工具设计要求：

- 工具保持原子化，只负责读取 diff、读取 patch fixture 文件、提交 review。
- 审查 checklist、verdict 标准和评论格式写在 skill 中。
- 提交成功后写出 `review.txt`。
- 写文件时使用 `workspace.resolve("review.txt")`。
- 返回值建议统一使用 `json_result(ok=..., ...)`。
- 不要修改 `service.py`，也不要把当前 review 的完整文本硬编码进工具。
