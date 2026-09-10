# 短时调度通知工具目录

请在这里创建本场景所需的工具模块。每个 `.py` 文件只要暴露 `make_tool(...) -> Tool`，就会被评测器自动发现。

工具设计要求：

- 成功交付后写出 `dispatch_receipt.txt`。
- 写文件时使用 `workspace.resolve("dispatch_receipt.txt")`。
- 返回值建议统一使用 `json_result(ok=..., ...)`。
- 不要修改 `service.py`，也不要硬编码通知文本。
