# 短时调度通知挑战

## Goal

实现一组场景工具和一个 skill，使 agent 能在短时访问凭证失效前读取紧急调度通知，并把通知交付给用户。

## Service API

工具可以导入下面的服务函数，但不应修改 `service.py`：

```python
from assignments.ephemeral_dispatch.service import (
    request_dispatch_token,
    read_dispatch_notice,
    notify_user,
)
```

| 函数                          | 返回值 | 说明                                             |
| ----------------------------- | ------ | ------------------------------------------------ |
| `request_dispatch_token()`    | `str`  | 返回一个临时 token，该 token 会在 200ms 后过期。 |
| `read_dispatch_notice(token)` | `str   | None`                                            | token 仍有效时返回紧急通知；过期或无效时返回 `None`。 |
| `notify_user(message)`        | `str`  | 将通知交付到面向用户的通知入口。                 |

## Student Deliverables

你需要补全：

```text
assignments/ephemeral_dispatch/tools/
assignments/ephemeral_dispatch/skills/ephemeral-dispatch/SKILL.md
```

提交成功后，工具应在评测 workspace 中写出：

```text
dispatch_receipt.txt
```

文件内容应为最终交付给用户的通知文本。

## Tool Design Expectations

每个工具模块都应遵循项目约定：

```python
def make_tool(workspace: Workspace) -> Tool:
    ...
```

## Skill Expectations

`ephemeral-dispatch` skill 应告诉模型：

- 使用专门的 dispatch 工具完成读取和通知。
- 不要先闲聊、总结或调用无关工具。
- 成功后最终回答应简短说明用户已收到通知。

skill 不应包含当前通知的固定文本；通知内容应来自服务函数。

## Evaluation

运行：

```bash
uv run python -m lab_eval assignments/ephemeral_dispatch/eval.json
```

或：

```bash
make eval-ephemeral
```

评测会检查：

- 通知是否成功交付。
- `dispatch_receipt.txt` 是否存在。
- 文件是否包含实际调度通知中的关键设备信息。
- LLM 请求次数、工具调用次数和 token 是否在限制内。

当前场景限制见 `eval.json`：

- `max_requests`: 3
- `max_tool_calls`: 2
- `max_estimated_total_tokens`: 4000
