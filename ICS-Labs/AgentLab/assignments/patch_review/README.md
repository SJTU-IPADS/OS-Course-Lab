# 变更风险审查挑战

## Goal

实现一组场景工具和一个 skill，使 agent 能像代码评审者一样读取待审查 patch，判断是否存在安全或行为风险，并提交结构化 review。

这个场景考察的是把代码审查 checklist 写进 skill，让模型按 checklist 使用工具读取证据、判断风险、给出明确结论。

## Service API

工具可以导入下面的服务函数，但不应修改 `service.py`：

```python
from assignments.patch_review.service import (
    read_diff,
    read_patch_file,
    submit_review,
)
```

| 函数 | 返回值 | 说明 |
| --- | --- | --- |
| `read_diff()` | `str` | 返回待审查的 diff 文本。 |
| `read_patch_file(path)` | `str | None` | 返回 patch fixture 中指定文件内容；未知路径返回 `None`。 |
| `submit_review(verdict, comments)` | `str` | 提交 review；成功时返回 `REVIEW SUBMITTED`。 |

## Student Deliverables

你需要补全：

```text
assignments/patch_review/tools/
assignments/patch_review/skills/patch-review/SKILL.md
```

建议至少提供这些工具：

- `read_diff`
- `read_patch_file`
- `submit_review`

提交成功后，工具应在评测 workspace 中写出：

```text
review.txt
```

文件内容应包含最终提交的 verdict 和 comments。

## Tool Design Expectations

每个工具模块都应遵循项目约定：

```python
def make_tool(workspace: Workspace) -> Tool:
    ...
```

工具只负责读取和提交：

- 读取 diff。
- 在 diff 不足以判断时读取相关 patch fixture 文件。
- 提交 review。

审查 checklist、风险判断和评论格式应写在 skill 中，而不是藏在工具里。

## Skill Expectations

`patch-review` skill 应告诉模型：

- 先读取 diff，再决定是否读取相关文件。
- 优先检查 workspace 边界、路径逃逸、输入校验、异常处理和测试缺口。
- 如果文件工具直接使用用户路径读取本地文件，而没有经过 workspace 安全解析，应认为存在 path traversal 风险。
- review 应包含明确 verdict，例如 `request_changes` 或可接受的结论。
- comments 应说明风险、建议修复方向，并要求补充相应回归测试。
- 最终回答和工具参数尽量使用英文，避免模型在英文评测 prompt 中混用中文。

skill 应写通用审查标准，不应只针对当前 diff 写固定句子。

## Evaluation

运行：

```bash
uv run python -m lab_eval assignments/patch_review/eval.json
```

或：

```bash
make eval-review
```

评测会检查：

- agent 是否提交 review。
- `review.txt` 是否存在。
- review 是否指出关键风险并给出可执行修复建议。
- LLM 请求次数、工具调用次数和 token 是否在限制内。

当前场景限制见 `eval.json`：

- `max_requests`: 6
- `max_tool_calls`: 8
- `max_estimated_total_tokens`: 6500

## Common Failures

- 只复述 diff，没有给明确 verdict。
- 只说“有安全问题”，没有说明风险如何发生。
- 漏掉 workspace 路径边界或路径逃逸问题。
- 提交 review 后没有写出 `review.txt`。
- comments 没有包含修复建议或测试建议。
- skill checklist 太泛，模型不知道优先审查哪些风险。
