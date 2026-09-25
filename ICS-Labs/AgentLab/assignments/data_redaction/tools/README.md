# 数据脱敏工具目录

请在这里创建本场景所需的工具模块。每个 `.py` 文件只要暴露 `make_tool(...) -> Tool`，就会被评测器自动发现。

工具设计要求：

- 工具保持原子化，脱敏规则和流程写在 skill 中。
- `handler` 接收 `dict[str, Any]`，返回 JSON 字符串。
- 返回值建议统一使用 `json_result(ok=..., ...)`。
- 写出 `redacted_ticket.txt` 时使用 `workspace.resolve("redacted_ticket.txt")`，确保写入评测 workspace。
- 不要修改 `service.py`，也不要把当前工单的完整答案硬编码进工具。
