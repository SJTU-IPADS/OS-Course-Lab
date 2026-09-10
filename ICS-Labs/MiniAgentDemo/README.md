# Mini Agent Demo

## 项目结构

本 Demo 展示了 Agent 架构的三个渐进层级：

- **s1-agent-loop**：核心循环模式（OpenAI 聊天补全 + 工具调用）
- **s2-tool-use**：多工具调度映射（bash、read、write、edit）
- **s3-skill-load**：双层技能注入（系统提示 + 按需加载）

## s01：Agent 循环

Agent 循环是 Agent 的核心控制流模式，实现了：

- **循环**：`while True` 直到不再有 `tool_calls`
- **退出条件**：当 `response.tool_calls` 为空或 None 时
- **工具结果处理**：将 `role="tool"` 消息追加回历史记录
- **客户端**：连接 OpenRouter 的 OpenAI SDK

> **测试方式**：通过 IDE 的 TAB 自动补全功能完成代码补全。

详见 [s1-agent-loop/agent-loop.md](s1-agent-loop/agent-loop.md)。

运行：
```sh
python s1-agent-loop/agent-loop.py
```

## s02：工具使用

在 s01 基础上扩展了多个专用工具及调度映射：

- **工具调度映射**：`{工具名: 处理函数}` 字典
- **路径沙箱**：`safe_path()` 防止工作区逃逸
- **多工具支持**：bash、read_file、write_file、edit_file
- **处理函数抽象**：每个工具均为纯函数

> **测试方式**：通过 IDE Chat Agent 与代码进行交互式对话完成测试。

详见 [s2-tool-use/tool-use.md](s2-tool-use/tool-use.md)。

运行：
```sh
python s2-tool-use/tool-use.py
```

## s03：技能加载

实现双层技能注入，避免 token 浪费：

- **第一层（系统提示）**：技能名称和描述（开销低，约 100 tokens/技能）
- **第二层（工具结果）**：通过 `load_skill` 工具加载完整技能内容（按需加载，开销高）
- **SkillLoader 类**：扫描 `skills/*/SKILL.md` 并解析 YAML 前置元信息
- **技能结构**：包含 `name`、`description` 及正文内容（Markdown 格式）

> **测试方式**：通过 CLI Agent 在命令行中与 Agent 进行交互完成测试。

详见 [s3-skill-load/skill-load.md](s3-skill-load/skill-load.md)。

运行：
```sh
cd s3-skill-load
python skill-load.py
```

## 环境配置

1. 使用 uv 安装依赖：
```sh
uv sync
```

如果系统没有安装 uv，可以参考 [uv 安装说明](https://github.com/astral-sh/uv)。

2. 创建 `.env` 文件：
```sh
OPENROUTER_API_KEY="你的 OpenRouter API 密钥"
OPENROUTER_BASE_URL="https://openrouter.ai/api/v1"
MODEL_ID="tencent/hy3-preview:free"
OPENROUTER_FALLBACK_MODEL="openrouter/free"
```

3. 加载环境变量：
```sh
source .env
```

## 消息流示例

```
用户："读取 requirements.txt 文件并告诉我里面有什么"

1. [用户] "读取 requirements.txt..."
   |
   LLM → 决定调用 read_file 工具
   |
2. [助手] 工具调用：read_file("requirements.txt")
   |
   Agent 执行 read_file() → "numpy==1.2.3\npandas==..."
   |
3. [工具] 工具结果："numpy==1.2.3\npandas==..."
   |
   LLM → 处理结果并生成回复
   |
4. [助手] "该文件包含 numpy 1.2.3 和 pandas..."
   |
   无更多工具调用 → 返回
```

## 依赖项

- `openai`：OpenAI Python SDK（兼容 OpenRouter）
- `python-dotenv`：从 `.env` 文件加载环境变量
- `pyyaml`：解析技能 SKILL.md 的前置元信息（仅 s03 需要）

完整列表见 `pyproject.toml`。

## 注意事项

- 所有脚本均强制执行**路径沙箱**，防止逃逸工作区
- **危险命令**（如 `rm -rf /`、`sudo` 等）已被拦截
- **输出截断**：超过 50KB 的内容会被截断，防止 token 爆炸
- **工具执行超时**：每条命令最多执行 120 秒
- 本演示以 Linux 为先（bash 命令假定使用 POSIX shell）
