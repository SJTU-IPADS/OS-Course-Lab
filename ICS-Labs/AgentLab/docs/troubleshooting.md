# 常见问题排查

这份文档按症状列出常见问题。先读评测报告里的 `failures`，再对照下面的条目排查。

## 模型输出不是 JSON

症状：

- `trace_summary.parse_errors` 增加。
- 最终回答类似 “Agent stopped: model did not follow JSON protocol.”

检查：

- system prompt 是否明确写了“Output only JSON”。
- system prompt 是否给了 `tool_call` 和 `final` 两个完整示例。
- agent 是否把非法输出和修复提示重新放回 messages。
- 修复提示是否要求“exactly one JSON object and no Markdown”。

## 工具没有被调用

症状：

- `Expected tool call 'read_file' not found`。
- `trace_summary.tool_calls` 为 0。

检查：

- 工具模块是否在 `ics_agent_lab/tools/` 或 assignment 的 `tools/` 目录。
- 工具模块是否提供了 `make_tool(...)`。
- `Tool.name` 是否和评测期望一致。
- `Agent.new_session()` 是否把 `self.tools.docs()` 写入 system prompt。
- agent 执行工具时是否记录了 `tool_call` trace。

## 工具参数校验失败

症状：

- 工具结果中出现 `Missing required argument`、`Unexpected argument` 或类型错误。

检查：

- schema 的 `required` 是否和 handler 使用的参数一致。
- schema 的 `properties` 是否列出了所有允许参数。
- 参数类型是否只有当前校验器支持的 `string` 和 `integer`。
- system prompt 中的工具文档是否足够清楚。

## 文件写错位置

症状：

- 评测说 `Expected file does not exist`。
- 本地项目根目录出现了本应在 workspace 中的文件。

检查：

- 工具是否使用 `workspace.resolve("filename.txt")`。
- 是否直接用了 `Path("filename.txt")` 或当前进程目录。
- 是否把 workspace 路径和 assignment 源码路径混淆。

## 路径逃逸风险

症状：

- 评测或 review 场景指出 path traversal。
- 工具直接使用用户传入路径读取文件。

检查：

- 文件类工具必须先调用 `workspace.resolve(arguments["path"])`。
- 不要直接 `Path(arguments["path"]).read_text(...)`。
- 不要允许绝对路径或 `..` 逃逸出 workspace。

## skill 没有效果

症状：

- 模型不知道有可用 skill。
- assignment 场景中模型没有按场景流程行动。

检查：

- `SkillLoader.descriptions()` 是否扫描了 `skills/<name>/SKILL.md`。
- front matter 是否包含 `name` 和 `description`。
- `Agent.new_session()` 是否把 `skill_docs` 放入 system prompt。
- 是否实现了 `load_skill` 工具。
- system prompt 是否要求模型在使用领域规则前调用 `load_skill`。
- skill 本身是否过长、过散；可以对照 `docs/skill-design.md` 精简成 checklist 风格。

## assignment 工具没被加载

症状：

- 业务场景中工具名未知。
- 只有通用工具可用。

检查：

- assignment 的工具是否放在该场景的 `tools/` 目录。
- 文件名是否以 `_` 开头；以下划线开头的模块会被跳过。
- 模块是否提供 `make_tool(...)`。
- `eval.json` 中是否有 `extra_tool_dirs`。

## data_redaction 失败

常见原因：

- 忘记脱敏邮箱、手机号、学号、token 或内网 IP。
- 保留了敏感值的一部分。
- 删除了 `password reset`、`MFA enrollment` 等故障上下文。
- 提交前没有调用验证工具。
- 提交成功后没有写 `redacted_ticket.txt`。

## ephemeral_dispatch 失败

常见原因：

- token 获取和通知读取分成多轮 LLM 工具调用，超过 200ms。
- 工具没有处理 `read_dispatch_notice(token)` 返回 `None` 的情况。
- 通知成功后没有写 `dispatch_receipt.txt`。
- skill 没有告诉模型优先使用 dispatch 工具。

## patch_review 失败

常见原因：

- verdict 不明确。
- comments 没有说明具体风险。
- 没有识别 workspace 路径边界或 path traversal 风险。
- 没有给出修复建议。
- 没有提到需要补回归测试。
- 提交成功后没有写 `review.txt`。

## memory 评测失败

常见原因：

- memory 只存在前一个 session 的 `messages` 里，新 session 无法检索。
- 只把 memory 存在局部变量里，但评测创建新 session 后没有重新注入相关 memory。
- 写入了 memory，但后续 LLM 请求前没有检索并注入相关 memory。
- 每轮都注入全部 memory，导致 token 超限或噪声过大。
- 更新 memory 时只追加新事实，没有让旧值失效，导致当前值和旧值混在一起。
- trace 中没有 `memory_write` 或 `memory_retrieve`，评测器无法确认 memory 管理行为。

检查：

- memory store 是否有稳定的记录结构，例如 `id`、`key`、`content`、状态、时间顺序或版本信息。
- `Agent.new_session()` 创建的新消息列表是否仍会检索已有 memory。
- 检索 query 是否来自当前用户问题和最近上下文，而不是固定返回第一条 memory。
- 检索结果是否有数量和长度上限。
- 如果 memory 中没有证据，final answer 是否说明 unknown，而不是猜测。

## OpenRouter 或网络错误

检查：

- `.env` 是否存在。
- `OPENROUTER_API_KEY` 是否填写。
- `OPENROUTER_BASE_URL` 是否是 OpenAI-compatible endpoint。
- `MODEL_ID` 是否可用；不可用时会尝试 `OPENROUTER_FALLBACK_MODEL`。
- 如果使用本地代理，确认 `HTTP_PROXY`、`HTTPS_PROXY` 或 `ALL_PROXY` 格式可被 httpx 识别。
