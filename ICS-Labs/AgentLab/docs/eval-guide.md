# 评测器使用指南

`lab_eval/` 是本实验提供的简化评测器。它不会替你判断代码风格，但会检查 agent 是否通过手写 JSON 协议完成任务、是否真的调用了工具、是否写出了预期文件，以及是否满足基本效率约束。

## 如何运行

运行单个通用工具评测：

```bash
uv run python -m lab_eval evals/read_file_efficiency.json
```

运行单个 assignment 评测：

```bash
uv run python -m lab_eval assignments/data_redaction/eval.json
```

也可以使用 Makefile：

```bash
make grade
make eval-read
make eval-list-files
make eval-memory-recall
make eval-memory-update
make eval-redaction
make eval-ephemeral
make eval-review
```

`make grade` 会运行全部公开评测并输出总分。`make eval-all` 是同义别名。

## 评测场景字段

每个 `eval.json` 都会被解析成一个 `EvalScenario`。常见字段如下：

| 字段                         | 作用                                                                           |
| ---------------------------- | ------------------------------------------------------------------------------ |
| `name`                       | 场景名称。                                                                     |
| `prompt`                     | 发给 agent 的用户请求。                                                        |
| `turns`                      | 可选的后续多轮用户请求；评测器会复用同一个 session。                           |
| `sessions`                   | 可选的多 session 用户请求列表；每个 session 会重新调用 `Agent.new_session()`。 |
| `workspace_files`            | 评测开始前写入临时 workspace 的文件。                                          |
| `skill_files`                | 评测开始前写入临时 skills 目录的 skill 文件。                                  |
| `skill_source_dirs`          | 从 assignment 目录复制 skill 的来源目录。                                      |
| `extra_tool_dirs`            | 额外加载的工具目录，通常是 assignment 的 `tools/`。                            |
| `expect_tool_calls`          | trace 中必须出现的工具名。                                                     |
| `expect_trace_events`        | trace 中某类事件至少应出现几次，例如 `memory_write`。                          |
| `expect_output_contains`     | 最终回答必须包含的文本。                                                       |
| `expect_output_not_contains` | 最终回答不能包含的文本，用于检查旧 memory 是否被错误当作当前值。               |
| `expect_files_contains`      | workspace 中某个文件必须包含的文本。                                           |
| `agent_config`               | 可选的 `AgentConfig` 覆盖项，公开 memory 测试会用它强制触发压缩。              |
| `limits`                     | LLM 请求、工具调用、token 和延迟限制。                                         |

## 评测报告字段

评测输出是 JSON：

| 字段                                | 如何理解                        |
| ----------------------------------- | ------------------------------- |
| `passed`                            | `true` 表示本场景通过。         |
| `answer`                            | agent 最终返回给用户的文本。    |
| `failures`                          | 未通过原因；先读这里。          |
| `metrics.request_count`             | 调用 LLM 的次数。               |
| `metrics.estimated_total_tokens`    | 基于文本长度估算的 token 数。   |
| `trace_summary.llm_responses`       | trace 中记录到的 LLM 响应次数。 |
| `trace_summary.tool_calls`          | trace 中记录到的工具调用次数。  |
| `trace_summary.parse_errors`        | JSON 协议解析失败次数。         |
| `trace_summary.context_compactions` | 上下文压缩次数。                |
| `trace_summary.finals`              | 记录到的最终回答次数。          |
| `workspace`                         | 本次评测的临时 workspace 路径。 |

默认评测 CLI 会保留 workspace。失败时优先打开报告里的 `workspace` 路径，看工具实际写出了什么文件。

## 统一评分报告

`make grade` 输出文本报告，形状如下：

```text
ICS Agent Lab grade report
Score: 68/83
Passed: 7/9

CASES
[PASS] read_file_efficiency    8/8   evals/read_file_efficiency.json
[FAIL] memory_persistent_update 0/7  evals/memory_persistent_update.json

FAILED CASES
- memory_persistent_update (0/7)
  - Expected at least 1 trace event(s) named 'memory_retrieve'; found 0.
  - workspace: ...
```

需要机器可读输出时可以运行：

```bash
uv run python -m lab_eval.suite --json
```

评分规则是按场景给分：通过某个场景获得该场景全部分值，未通过则该场景为 0 分。`make grade` 覆盖 83 分公开正确性；另外 17 分来自 token 效率榜单。

| 场景                       | 分值 |
| -------------------------- | ---: |
| `read_file_efficiency`     |    8 |
| `list_files_nested`        |    8 |
| `edit_file_replace`        |    8 |
| `bash_workspace`           |    8 |
| `data_redaction`           |   12 |
| `ephemeral_dispatch`       |   12 |
| `patch_review`             |   12 |
| `memory_persistent_recall` |    8 |
| `memory_persistent_update` |    7 |
| token 效率榜单             |   17 |

## Trace 要求

评测器通过 trace 判断 agent 是否真的执行了关键行为。建议记录：

```python
self.trace.add(step, "llm_response", agent=self.name, raw=raw)
self.trace.add(step, "tool_call", agent=self.name, name=name, arguments=arguments, result=result)
self.trace.add(step, "parse_error", agent=self.name, reason=reason)
self.trace.add(step, "context_compacted", agent=self.name, kept_recent=..., summarized=...)
self.trace.add(step, "memory_write", agent=self.name, id=..., kind=...)
self.trace.add(step, "memory_retrieve", agent=self.name, ids=[...], count=...)
self.trace.add(step, "final", agent=self.name, content=content)
```

如果最终答案正确但没有记录 `tool_call`，需要工具调用的评测仍会失败。memory 评测还会检查 `context_compacted`、`memory_write` 或 `memory_retrieve` 等事件，确保不是靠无限保留原始上下文通过。

## 效率限制

`limits` 不是隐藏性能竞赛，而是为了防止 agent 反复闲聊、重复读取文件或把所有内容塞进 prompt。

常见限制含义：

- `max_requests`：最多允许几次 LLM 请求。
- `max_tool_calls`：最多允许几次工具调用。
- `max_estimated_total_tokens`：粗略限制 prompt 和 completion 总长度。
- `max_latency_ms`：限制总延迟，当前 handout 场景通常不使用。

优化方向：

- system prompt 保持清晰但不要过长。
- 工具结果限制长度，避免把大文件完整写回上下文。
- 对 time-critical 场景设计合适的工具边界。
- 让 skill 摘要进入 system prompt，完整 skill 按需加载。
- 对长对话使用 memory 检索，只注入少量相关记录，不要把所有 memory 每轮都塞回 prompt。

## 建议调试流程

1. 先跑最小场景：`make eval-read`。
2. 如果失败，读 `failures`。
3. 查看 `trace_summary`，确认是否有 LLM 响应、工具调用和 final。
4. 查看 `workspace`，确认文件是否写到正确位置。
5. 打开 `traces/latest.jsonl` 或 CLI 指定 trace，逐步查看模型输出和工具结果。
6. 修复一个问题后只重跑对应场景，不要一次跑所有场景。
