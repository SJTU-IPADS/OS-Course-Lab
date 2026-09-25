# ICS Agent 实验

本实验要求你实现一个可以使用工具、技能和子 agent 的简化编程 agent。你可以使用 OpenAI 兼容 SDK 发送和接收普通聊天消息，但不能使用 SDK 自带的 function calling、tool calling、agent SDK 或 `tool_choice`。

核心目标是：自己实现 agent 循环、手写 JSON 工具协议、工具调度、错误处理、上下文管理、skill 加载、持久化记忆模块实现和评测所需的 trace。

## 你最终需要交付什么

完成 lab 后，仓库应具备这些能力：

- `Agent.run_turn(...)` 能用手写 JSON 协议完成多轮 LLM/工具循环。
- 通用工具能读取、写入、编辑、列出 workspace 文件，并能在 workspace 内运行 shell 命令。
- agent 能发现 skill 摘要，并通过 `load_skill` 按需加载完整 skill。
- agent 能通过 `ask_subagent` 把边界清晰的小任务交给子 agent。
- 三个业务场景的工具和 skill 能通过对应评测。
- agent 能管理长期上下文，把旧信息压缩成 memory，并在后续任务中按需检索。
- trace 中能观察到 LLM 响应、工具调用、解析错误、上下文压缩、memory 写入/检索和最终回答。

本实验重点是 agent 机制本身。不要把评测场景硬编码进 agent，也不要绕过手写 JSON 协议。

## 实验限制

允许：

- 使用 OpenAI/OpenRouter SDK 发送普通 Chat Completions 请求。
- 自己设计 system prompt、JSON 协议提示和错误修复提示。
- 为工具编写普通 Python 代码。
- 为每个场景编写 `SKILL.md`，让模型按需读取流程说明。

禁止：

- 使用 SDK native function calling/tool calling。
- 使用 agent SDK 或内置 agent 框架。
- 向 API 传 `tools`、`tool_choice` 或类似参数。
- 修改评测器来放宽评测。
- 修改 assignment 的 `service.py` 来绕过业务要求。
- 在工具或 agent 中硬编码某个 eval 的 prompt、答案或文件名。

如果在后续检查中发现最终提交的源码里使用了任何现有的 Agent 框架（包括 `openai` 自有的工具调用模块），该实验将不得分。

## 建议完成顺序

### Milestone 0: 环境准备

1. 安装依赖：

   ```bash
   make sync
   ```

2. 复制 `.env.example` 为 `.env`，填写 `OPENROUTER_API_KEY`。可以按需修改 `MODEL_ID`。

3. 确认 CLI 可以启动：

   ```bash
   make run PROMPT='hello'
   ```

刚开始 agent 还不会真正回答，这是正常的。

### Milestone 1: 手写 JSON agent loop

实现 `ics_agent_lab/core/agent.py` 中的 agent loop。一次 agent step 的基本流程是：

1. 调用普通 LLM chat completion。
2. 记录 `llm_response` trace。
3. 解析 assistant 文本中的 JSON。
4. 如果是工具调用，执行工具并把结果写回上下文。
5. 如果是最终回答，记录 `final` trace 并返回内容。
6. 如果 JSON 不合法，记录 `parse_error`，给模型一次修复机会。

你需要在 `ics_agent_lab/core/protocol.py` 中自行设定提示词，要求 LLM 按照你要求的格式进行输出。比如，你可以在系统提示词中明确要求模型只输出下面两种 JSON 对象之一：

```json
{"type": "tool_call", "name": "read_file", "arguments": {"path": "a.txt"}}
```

```json
{"type": "final", "content": "answer for the user"}
```

注意，由于大模型存在幻觉的可能性，回复给你的内容可能不完全符合 JSON 格式，比如模型很有可能会把 Markdown 格式的代码引用范式（反引号）一起输出出来，你可以尝试进行一定的容错处理，或者要求 LLM 进行重试。

### Milestone 2: 通用工具

建议至少实现这些通用工具：

- `read_file`
- `write_file`
- `edit_file`
- `list_files`
- `bash`
- `load_skill`
- `save_memory`
- `read_memory`
- `ask_subagent`

文件类工具必须通过 `Workspace.resolve(path)` 解析路径，不能允许路径逃逸出 workspace。工具返回值建议统一使用 `json_result(...)`，这样 LLM 更容易读懂工具结果，评测 trace 也更稳定。

### Milestone 3: skill loader

实现 `SkillLoader`：

- 扫描 `skills/<name>/SKILL.md`。
- 读取 front matter 中的 `name` 和 `description`。
- `descriptions()` 只返回名称和摘要，用于 system prompt。
- 完整正文通过 `load_skill` 工具按需加载。

不要把所有 skill 正文一次性塞进 system prompt。这样会浪费 token，也不符合按需加载的设计目标。

Skill 写作建议：

`SKILL.md` 的目标不是把答案写死，而是给模型一个短而清晰的流程、checklist 和输出要求。

- 推荐先读 `docs/skill-design.md`。
- 对中等能力模型，短步骤和 checklist 往往比长篇说明更稳定。
- 如果场景失败先表现为 token 超限、parse error 或“空谈不行动”，先精简 skill，再检查工具实现。

### Milestone 4: assignment 工具和 skill

进入每个 assignment 子目录后，先读该目录的 `README.md`：

- `assignments/data_redaction/`
- `assignments/ephemeral_dispatch/`
- `assignments/patch_review/`

每个场景都要求你实现场景工具和场景 skill。工具应暴露原子业务能力，流程、判断标准和注意事项应写在 skill 中。

### Milestone 5: Agent Memory 管理

真实 agent 不能把所有历史消息无限塞进 prompt。上下文过长会带来两个问题：一是早期关键信息被模型忽略或被后续噪声淹没，二是 token 成本快速上升。你需要实现两个边界清楚的机制：context compact 只负责压缩当前 session 的旧消息；memory 只负责跨 session 的持久事实和偏好。

本实验不要求使用向量数据库、embedding、外部服务或复杂图数据库。推荐使用标准库实现一个轻量 Markdown memory store。仓库中的 `ics_agent_lab/memory/loader.py` 使用类似 skill 的格式：每条 memory 是一个 `.md` 文件，第一行标题保存 key，正文保存 content。完整 memory 由模型在需要时通过 `read_memory` 工具按 key 读入上下文，稳定事实通过 `save_memory` 工具保存或更新。

最低要求：

- memory 按需加载：system prompt 中只出现 memory key 列表；当问题依赖长期事实时，模型应调用 `read_memory` 读取 exact key，而不是每次注入全部 memory。
- memory 可保存：当用户明确要求 remember、update 或 persist 时，模型应调用 `save_memory`，用稳定 key 覆盖同一事实的旧值。
- 更新优先：如果同一事实后来被更正，回答当前状态时应优先使用新事实，同时在需要时可以保留旧事实的历史含义。
- 不编造：如果 memory 和当前上下文都没有证据，应说明未知，而不是猜测。

公开 memory 评测会用多个独立 session 写入、更新和查询 memory。查询 session 不会携带前面 session 的 `messages`，因此不能靠原始对话历史通过。隐藏测试可能会更长、更嘈杂，但仍然只考察这个 lab 框架内的 memory 管理能力。

不要把 context compact summary 写进长期 memory。它属于当前 session 的上下文维护，不应污染用户偏好、项目事实或跨 session 指令。

### Milestone 6: Token Efficiency

最后一个阶段是冲榜任务。你需要把自己的 `ics_agent_lab` 源码目录打包成 zip，并在网页上提交学号和压缩包。服务器会解压你的提交，只保留 `.py` 文件，然后在隔离的 Docker 判题环境中运行一个隐藏任务。这个任务不会复用公开 `evals/` 或 assignment 的题目，它只检查你的 agent 是否能在复杂、高噪声、长上下文的场景中完成一次真实调查。

冲榜任务的核心目标不仅仅是“能否调用某个固定工具”，而是让 agent 在大量干扰信息中高效定位证据、维护上下文、使用工具推进任务，并提交正确结论。判题会统计本次运行的总 token 消耗。只有评测通过的提交会进入总榜，总榜按 token 数从低到高排序；评测失败、评测出错和正在评测的提交只会显示在你自己的提交列表中。

你需要在[网站](https://ipads.se.sjtu.edu.cn/courses/ics/ics-agent-lab/)上提交源码压缩包并查看测评结果。

你需要重点优化这些能力：

- 写好明确且简洁的系统提示词：系统提示词应当准确地告知 LLM 已知的信息、通用的任务流程、清晰的工作协议。
- 做好上下文压缩：压缩结果应保留任务目标、关键 ID、路径、证据链、工具输出、失败尝试和下一步计划，不能只截断前几百个字符。
- 设计清晰的工具协议：模型输出必须稳定遵守你设定的协议，该任务涉及的工具及其描述均由我们测试时给出，为了能够正确加载工具，请你确保我们给出的工具加载方式未发生改变。
- 其他任一在这个 Agent 的框架范围内可能降低 Token 消耗的方式。

关于上下文压缩的部分，你可以不使用我们所提供的 `core.agent.AgentConfig` 中的配置，可以在其中添加自己的与上下文压缩相关的变量，比如设定为 token 数（而不是我们提供的 message 数）超出阈值时才进行压缩。但请保证测评可以正确运行。

提交和榜单注意事项：

- 提交代码前请确认不要误用他人的学号，否则请联系助教对此次提交进行 Dispute。
- 同一学号 5 分钟内最多进行一次有效评测提交。格式错误、不是 zip 或缺少目标目录的提交不计入这个限制。
- 哈希值相同的代码不能重复提交。不同同学提交同一份代码也会被拒绝；同一个同学也不能重复提交同一份代码进行多次测评，除非前一次测评出现了格式错误等。
- 学号记录在后台数据库中，前端只会显示所提交的代码哈希值。浏览器会缓存你自己的提交标识，并在列表中高亮显示，如使用无痕模式或更换浏览器可能导致你无法识别自己曾经的提交，建议关闭无痕模式后上传代码。
- 单次测评的 token 消耗的最高限制为 30w，如超出限制，评测会被提前中止并标记为失败。请不要尝试靠无限试错完成任务。
- 判题容器的网络访问受限，仅允许访问 openrouter 域名，我们测评时统一使用 DeepSeek-V4-Flash。如果你在实现 Lab 的过程中使用过其他 TransportLayer，提交前不要忘记改回 `OpenRouterChatTransport`。也因此请不要依赖外部网站、远程下载、私有服务或隐藏数据通道等获取题目内容并进行针对性回复。
- 提交包中非 Python 文件会被删除，另外，`llm/` 和 `runtime/` 两个文件夹会被删除替换为我们的原始实现，此后再进行评测，因此不要把必要逻辑放在数据文件、临时文件、非 `.py` 的配置文件或提到的这两个文件夹里。
- 榜单仅用于展示实时评测结果，方便同学们了解其他同学的进度，在提交最终代码后，我们将会以最终代码为准重新进行该部分的评测。

我们会在网站上给出两个 Baseline，分别为我们实现的无上下文压缩 Agent 和带上下文压缩的 Agent，它们的 token 消耗分别为 $c_1$ 和 $c_2$。对于该部分，你的最终得分将由下面的公式计算而来：

$$
\mathrm{score}=\begin{cases}
   0 & \mathrm{if}\text{ }c_{\mathrm{your}}\ge c_1 \\
   \frac{c_1-c_{\mathrm{your}}}{c_1-c_2}\times 10 & \mathrm{if}\text{ }c_2\le c_{\mathrm{your}}<c_1 \\
   10 + \sqrt{\frac{c_2-c_{\mathrm{your}}}{c_2-c_\mathrm{min}}}\times 7 & \mathrm{if}\text{ }c_{\mathrm{your}}<c_2
\end{cases}
$$

这里提供一个数据供参考，在无上下文压缩的 Agent 里，一共进行了 20 次工具调用。

## 项目结构

```text
.
├── ics_agent_lab/                 # 你主要实现的 agent 包
│   ├── cli.py                     # 命令行入口，只负责参数和交互
│   ├── core/                      # Agent 循环、协议解析、日志 trace
│   ├── llm/                       # LLMTransport 兼容传输层
│   ├── runtime/                   # RuntimeConfig、依赖组装、子 Agent 组装
│   ├── skills/                    # SKILL.md 扫描和按需加载
│   └── tools/                     # 工具基类、工具注册、具体工具
├── lab_eval/                      # 评测框架，不属于 agent 运行时
├── assignments/                   # 业务场景
│   ├── data_redaction/
│   ├── ephemeral_dispatch/
│   └── patch_review/
├── evals/                         # 通用内置工具评测场景
├── workspace/                     # 本地运行时工具工作区
├── main.py                        # 调用 ics_agent_lab.cli 运行 agent
├── Makefile
└── pyproject.toml
```

## 关键源码位置

### Agent

`ics_agent_lab/core/agent.py` 是本实验最核心的位置。

- `AgentConfig` 保存最大步数、JSON 修复次数、上下文压缩阈值、工具结果长度限制，以及你选择暴露的 memory 参数。
- `Agent.new_session()` 应创建包含协议、工具文档和 skill 摘要的 system message。
- `Agent.run_turn(messages, user_input)` 应追加用户输入并驱动 agent loop。
- `Agent.save_trace(path)` 会保存 JSONL trace，评测器通过 trace 统计行为。

### Trace

`ics_agent_lab/core/trace.py` 已提供 `TraceRecorder`。建议至少记录这些事件：

- `llm_response`
- `tool_call`
- `parse_error`
- `context_compacted`
- `memory_write`
- `memory_retrieve`
- `final`

评测器不只看最终答案，也会看 trace 中是否真的发生了工具调用。

### Tools

`ics_agent_lab/tools/base.py` 已提供：

- `Tool`
- `Workspace`
- `ToolRegistry`
- `json_result`
- `validate_arguments`

每个工具模块都应暴露：

```python
def make_tool(...) -> Tool:
    ...
```

`ics_agent_lab/tools/builder.py` 会自动发现 `ics_agent_lab/tools/*.py` 和 assignment 的额外工具目录。`make_tool(...)` 可以按参数名接收下面这些依赖：

- `workspace`
- `skill_loader`
- `memory_loader`
- `subagent_runner`

### Skills

skill 文件格式固定为：

```text
skills/<name>/SKILL.md
```

示例：

```markdown
---
name: test
description: 示例技能
---

技能正文...
```

system prompt 中只应出现 skill 名称和描述。完整正文应由模型在需要时调用 `load_skill` 读取。

### Runtime 和 CLI

`ics_agent_lab/runtime/builder.py` 负责组装 LLM、workspace、skill loader、工具注册表、主 agent 和子 agent runner。CLI 和评测器都会走这条路径，因此不要只让手动运行能工作，也要让评测器创建的 runtime 能工作。

`ics_agent_lab/cli.py` 支持单轮 prompt 和交互模式。交互模式中 `/new` 会调用 `agent.new_session()` 开始新的对话 session；每轮结束后会保存 trace。

## 运行命令

单轮运行：

```bash
make run PROMPT='please read a.txt'
```

交互模式：

```bash
make shell
```

格式化和编译检查：

```bash
make check
```

`make check` 当前会运行格式化和 Python 编译检查，不包含单独的单元测试。

## 评测矩阵

统一评分命令：

```bash
make grade
```

`make grade` 会运行全部公开评测，报告每个场景是否通过、失败原因和最终得分。`make eval-all` 是同义别名。未全部通过时命令退出码为 1。

通用工具评测：

| 命令                      | 场景                                  | 主要检查                                                  |
| ------------------------- | ------------------------------------- | --------------------------------------------------------- |
| `make eval-read`          | `evals/read_file_efficiency.json`     | `read_file` 能读取文件，并在 1 次工具调用内完成。         |
| `make eval-list-files`    | `evals/list_files_nested.json`        | `list_files` 能递归报告 workspace 内所有文件。            |
| `make eval-edit-file`     | `evals/edit_file_replace.json`        | `edit_file` 能替换局部文本，而不是重写整个文件。          |
| `make eval-bash`          | `evals/bash_workspace.json`           | `bash` 能在 workspace 内创建指定文件。                    |
| `make eval-memory-recall` | `evals/memory_persistent_recall.json` | 在新 session 中检索之前 session 写入的 memory。           |
| `make eval-memory-update` | `evals/memory_persistent_update.json` | 在新 session 中更新旧 memory，并在后续 session 读取新值。 |

业务场景评测：

| 命令                  | 场景                                       | 主要检查                                                         |
| --------------------- | ------------------------------------------ | ---------------------------------------------------------------- |
| `make eval-redaction` | `assignments/data_redaction/eval.json`     | 工单脱敏、保留故障上下文、提交成功并写出 `redacted_ticket.txt`。 |
| `make eval-ephemeral` | `assignments/ephemeral_dispatch/eval.json` | 在短时 token 失效前交付通知，并写出 `dispatch_receipt.txt`。     |
| `make eval-review`    | `assignments/patch_review/eval.json`       | 审查 patch 风险、提交 review，并写出 `review.txt`。              |

## 评分细则

总分 100 分，其中 `make grade` 可直接测试 83 分公开正确性，另有 17 分 token 效率榜单。公开评测按任务阶段难度分配如下：

| 阶段                       | 评测场景                   | 分值 | 主要能力                                            |
| -------------------------- | -------------------------- | ---: | --------------------------------------------------- |
| 基础 agent loop 和工具调用 | `read_file_efficiency`     |    8 | 单次工具调用、最终回答。                            |
| workspace 文件发现         | `list_files_nested`        |    8 | 递归列出 workspace 内文件，路径报告稳定。           |
| 局部文件编辑               | `edit_file_replace`        |    8 | 精确替换文本，避免整文件重写。                      |
| workspace shell 工具       | `bash_workspace`           |    8 | 在 workspace 内运行命令并生成文件。                 |
| 数据脱敏业务流程           | `data_redaction`           |   12 | 场景工具、skill 流程、验证后提交、保留故障上下文。  |
| 短时调度业务流程           | `ephemeral_dispatch`       |   12 | time-critical 工具边界、效率约束、通知交付。        |
| patch 风险审查流程         | `patch_review`             |   12 | skill checklist、风险判断、明确 review 和测试建议。 |
| memory 跨 session 召回     | `memory_persistent_recall` |    8 | 在新 session 中召回之前保存的用户和项目事实。       |
| memory 跨 session 更新     | `memory_persistent_update` |    7 | 更新旧 memory 后，只把新值作为当前答案。            |
| token 效率榜单             | `log_analyzing`            |   17 | 复杂场景、上下文压缩、工具连续调用和 token 控制。   |

分值合计 100 分：

- 通用工具和基础 agent 能力：32 分。
- 业务场景、skill 使用和效率设计：36 分。
- memory 管理公开评测：15 分。
- token 效率榜单：17 分。

一个场景通过则获得该场景全部分值；未通过则该场景为 0 分。评测报告会列出失败原因，便于逐项修复。

也可以直接运行某个场景：

```bash
uv run python -m lab_eval evals/read_file_efficiency.json
uv run python -m lab_eval evals/memory_persistent_recall.json
uv run python -m lab_eval assignments/data_redaction/eval.json
```

注意：提交上来的代码在我们进行测评时，`llm/` 和 `runtime/` 两个文件夹会被删除替换为我们的原始实现，此后再进行评测。进行本地评测时请确保这两个文件夹恢复回分发状态时也可以正常工作。

## 答辩安排

暂定为本学期最后一周的课上进行，主要考核你对你所提交的代码（无论是自己写的或是 Vibe 的）的熟悉程度，请尽量理解你提交的代码。

从代码角度看，你需要实现的部分分为四个模块（`core`、`memory`、`skills`、`tools`），如有 1 个模块不够熟悉则实验分数乘以 0.9，如 2 个模块不熟悉则乘以 0.8，依此类推。

## 如何读评测报告

评测输出是 JSON，常用字段包括：

- `passed`：是否通过。
- `answer`：agent 最终返回给用户的文本。
- `failures`：未通过原因。
- `metrics.request_count`：LLM 请求次数。
- `metrics.estimated_total_tokens`：估算 token 消耗。
- `trace_summary.tool_calls`：trace 中观察到的工具调用次数。
- `trace_summary.context_compactions`：trace 中观察到的上下文压缩次数。
- `workspace`：本次评测使用的临时 workspace，默认会保留，便于查看生成文件。

常见失败原因：

- 模型输出 Markdown 或自然语言，而不是纯 JSON。
- 工具名和注册名不一致。
- 工具 schema 要求的参数和模型实际传参不一致。
- 工具成功执行了，但没有记录 `tool_call` trace。
- 文件写到了项目根目录，而不是评测 workspace。
- skill 摘要没有进入 system prompt，模型不知道应该调用 `load_skill`。
- 业务场景工具太粗或太细，导致超过工具调用次数或 token 限制。
- memory 只保存在当前 `messages` 或 Python 临时变量里，新 session 无法检索。
- memory 更新后旧值仍被当作当前值，导致查询当前信息时返回过期内容。

## 模型选择建议

本 lab 不绑定某一个特定模型，但不同模型的稳定性差异会很明显。最终测试会以 `nvidia/nemotron-3-super-120b-a12b:free` 模型为基准。

- 能力较弱或指令遵循一般的模型，更容易输出非法 JSON、忽略冗长 skill，或在 assignment 场景里做多余解释。
- 如果你连续遇到 `parse_error`、token 超限或 patch_review 反复不给明确 verdict，先缩短 prompt 和 skill，再去改工具。
- 如果换模型，请优先保持 `temperature=0`，否则你会同时面对协议问题和随机性问题。

更多调试建议见 `docs/troubleshooting.md` 和 `docs/eval-guide.md`。

## assignment 目录约定

每个 assignment 都是一个独立业务场景，包含：

- `README.md`：场景背景、任务目标、实现位置和运行方式。
- `service.py`：场景提供的业务服务函数。学生工具可以导入这些函数，但不应修改服务实现。
- `tools/`：学生为该场景编写工具模块的位置。
- `skills/`：学生为该场景编写 `SKILL.md` 的位置。
- `eval.json`：评测配置，评测器会据此加载 workspace、额外工具目录和 skill 目录。

学生进入某个 assignment 后，应先阅读该子目录下的 `README.md`。顶层 README 只说明整体路径；具体服务 API、场景规则、工具建议和评测命令以 assignment 自己的 README 为准。

## 问题反馈

本实验是首次发布，可能存在诸多不足，如有任何问题，或对实验细节和评分有任何建议，欢迎与助教反馈讨论。
