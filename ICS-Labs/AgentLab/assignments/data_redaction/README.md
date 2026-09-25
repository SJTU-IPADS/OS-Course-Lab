# 数据脱敏提交挑战

## Goal

实现一组场景工具和一个 skill，使 agent 能读取指定工单、移除敏感信息、保留必要故障上下文、验证脱敏结果，并提交通过检查的版本。

## Service API

工具可以导入下面的服务函数，但不应修改 `service.py`：

```python
from assignments.data_redaction.service import (
    read_ticket,
    validate_redaction,
    submit_redacted_ticket,
)
```

| 函数                                         | 返回值      | 说明                                                |
| -------------------------------------------- | ----------- | --------------------------------------------------- |
| `read_ticket(ticket_id)`                     | `str        | None`                                               | 返回原始工单文本；未知工单返回 `None`。 |
| `validate_redaction(content)`                | `list[str]` | 返回脱敏检查问题列表；空列表表示通过。              |
| `submit_redacted_ticket(ticket_id, content)` | `str`       | 提交脱敏后的工单；成功时返回 `REDACTION ACCEPTED`。 |

## Student Deliverables

你需要补全：

```text
assignments/data_redaction/tools/
assignments/data_redaction/skills/data-redaction/SKILL.md
```

建议至少提供这些工具：

- `read_ticket`
- `validate_redaction`
- `submit_redacted_ticket`

提交成功后，工具应在评测 workspace 中写出：

```text
redacted_ticket.txt
```

文件内容应为最终提交的脱敏工单文本。

## Tool Design Expectations

每个工具模块都应遵循项目约定：

```python
def make_tool(workspace: Workspace) -> Tool:
    ...
```

工具只负责原子业务动作：

- 读取工单。
- 验证某段候选脱敏文本。
- 提交已经验证过的脱敏文本。

工具返回值建议使用 `json_result(...)`。如果提交成功并需要写文件，使用 `workspace.resolve("redacted_ticket.txt")` 写入评测 workspace。

## Skill Expectations

`data-redaction` skill 应告诉模型：

- 先读取原始工单，再处理内容。
- 邮箱、手机号、学号、访问 token 和内网 IP 都属于敏感信息，必须替换为占位符或泛化描述。
- 保留故障排查所需上下文，尤其是故障动作、系统状态和复现线索。
- 提交前必须调用验证工具；如有问题，修正后再次验证。
- 验证通过后再提交。
- 最终回答和工具参数尽量使用英文，避免模型在英文评测 prompt 中混用中文。

skill 中应写流程和判断标准，不应写死当前工单的完整答案。

## Evaluation

运行：

```bash
uv run python -m lab_eval assignments/data_redaction/eval.json
```

或：

```bash
make eval-redaction
```

评测会检查：

- agent 是否完成提交。
- `redacted_ticket.txt` 是否存在。
- 脱敏结果是否保留关键故障上下文。
- LLM 请求次数、工具调用次数和 token 是否在限制内。

当前场景限制见 `eval.json`：

- `max_requests`: 6
- `max_tool_calls`: 8
- `max_estimated_total_tokens`: 6500

## Common Failures

- 只删除敏感信息，导致故障上下文也丢失。
- 忘记调用验证工具就直接提交。
- 验证失败后没有根据 issues 修正内容。
- 提交成功但没有写出 `redacted_ticket.txt`。
- 工具把输出文件写到项目根目录，而不是评测 workspace。
- skill 只写“脱敏数据”，没有列出具体敏感类型和提交前检查流程。
