# 工具使用 - 扩展 Agent 能力

## 一、项目简介

在 s1 的基础上，本阶段引入多个专用工具，通过调度映射机制扩展 Agent 的能力边界。

核心思路：**添加工具不需要改动循环逻辑**，只需注册到调度映射即可。

### 核心工作流程

```
+--------+      +-------+      +------------------+
|  用户   | ---> |  LLM  | ---> | 工具调度          |
| 提示词  |      |       |      | {                |
+--------+      +---+---+      |   bash: run_bash |
                    ^           |   read: run_read |
                    |           |   write: run_wr  |
                    +-----------+   edit: run_edit |
                    tool_result | }                |
                                +------------------+

调度映射是一个字典：{工具名: 处理函数}。
一次查找即可替代冗长的 if/elif 链。
```

### 主要组件

1. **工具调度映射**：`TOOL_HANDLERS` 字典，将工具名映射到处理函数
2. **路径沙箱**：`safe_path()` 函数，防止工作区逃逸
3. **专用工具**：`read_file`、`write_file`、`edit_file` 等，替代不安全的 bash 命令
4. **OpenAI 函数工具**：使用 OpenAI 的 function calling 格式定义工具

---

## 二、任务说明

本练习需要理解以下核心概念，并完成工具的注册和使用。

### 任务 1: 理解工具调度映射

**核心代码**:
```python
TOOL_HANDLERS = {
    "bash":       lambda **kw: run_bash(kw["command"]),
    "read_file":  lambda **kw: run_read(kw["path"], kw.get("limit")),
    "write_file": lambda **kw: run_write(kw["path"], kw["content"]),
    "edit_file":  lambda **kw: run_edit(kw["path"], kw["old_text"],
                                        kw["new_text"]),
}
```

**关键点**:
- 调度映射是 `{工具名: 处理函数}` 的字典
- 添加新工具只需在字典中新增一项
- 循环代码无需修改

### 任务 2: 理解路径沙箱机制

**核心代码**:
```python
def safe_path(p: str) -> Path:
    path = (WORKDIR / p).resolve()
    if not path.is_relative_to(WORKDIR):
        raise ValueError(f"路径逃逸工作区: {p}")
    return path
```

**关键点**:
- 使用 `resolve()` 解析绝对路径
- 使用 `is_relative_to()` 检查是否在工作区内
- 防止通过 `../` 等方式逃逸工作区

### 任务 3: 理解工具执行循环

**核心代码**:
```python
response = get_completion(messages)
assistant_message = response.choices[0].message
tool_calls = assistant_message.tool_calls or []

if not tool_calls:
    return

for tool_call in tool_calls:
    name = tool_call.function.name
    handler = TOOL_HANDLERS.get(name)
    arguments = json.loads(tool_call.function.arguments)
    output = handler(**arguments) if handler else f"未知工具: {name}"
    messages.append({
        "role": "tool",
        "tool_call_id": tool_call.id,
        "content": str(output),
    })
```

**关键点**:
- 从响应中提取 `tool_calls`
- 通过工具名查找处理函数
- 解析参数并调用处理函数
- 将结果追加为 `role="tool"` 消息

---

## 三、测试方法

> **测试方式**：通过 IDE Chat Agent 与代码进行交互式对话完成测试。

完成代码理解后，按以下步骤进行测试：

### 1. 配置环境变量

```sh
export OPENROUTER_API_KEY="你的 API 密钥"
export OPENROUTER_BASE_URL="https://openrouter.ai/api/v1"
export MODEL_ID="tencent/hy3-preview:free"  # 或你喜欢的模型
```

### 2. 运行程序

```sh
python s2-tool-use/tool-use.py
```

### 3. 输入测试命令

程序启动后，会显示提示符 `s2 >>`，尝试以下操作：

1. `读取文件 s1-agent-loop/agent-loop.py`
2. `创建一个名为 test.py 的文件，包含一个简单的 hello() 函数`
3. `修改 test.py，为函数添加文档字符串`
4. `读取 test.py 验证修改是否成功`

### 4. 预期结果

如果一切正常，你应该看到类似以下的输出：

```
s2 >> 创建一个名为 test.py 的文件，包含一个简单的 hello() 函数
[LOG]: Writing file: test.py
[LOG]: Tool call: write_file
Wrote 33 bytes to test.py
[LOG]: Reading file: test.py
[LOG]: Tool call: read_file
完成！文件 `test.py` 已创建，其中包含简单的 `hello()` 函数，调用时会打印 "Hello!"。
```

### 5. 退出程序

输入 `q` 或 `exit` 或直接按回车键退出程序。

---

## 四、标准答案

以下是完整的核心代码实现，用于参考和验证。

### 工具调度映射

```python
TOOL_HANDLERS = {
    "bash":       lambda **kw: run_bash(kw["command"]),
    "read_file":  lambda **kw: run_read(kw["path"], kw.get("limit")),
    "write_file": lambda **kw: run_write(kw["path"], kw["content"]),
    "edit_file":  lambda **kw: run_edit(kw["path"], kw["old_text"],
                                        kw["new_text"]),
}
```

### 路径沙箱函数

```python
def safe_path(p: str) -> Path:
    path = (WORKDIR / p).resolve()
    if not path.is_relative_to(WORKDIR):
        raise ValueError(f"路径逃逸工作区: {p}")
    return path

def run_read(path: str, limit: int | None = None) -> str:
    text = safe_path(path).read_text()
    lines = text.splitlines()
    if limit and limit < len(lines):
        lines = lines[:limit]
    return "\n".join(lines)[:50000]
```

### OpenAI 函数工具定义

```python
TOOLS = [
    {
        "type": "function",
        "function": {
            "name": "bash",
            "description": "执行 Shell 命令。",
            "parameters": {
                "type": "object",
                "properties": {"command": {"type": "string"}},
                "required": ["command"],
            },
        },
    },
    # ...更多工具...
]
```

---

## 五、学习要点

通过这个练习，你将学习到：

1. **工具调度模式**: 理解如何通过调度映射扩展 Agent 能力，而不修改核心循环
2. **路径沙箱**: 学习如何在工具层面实施安全限制，防止工作区逃逸
3. **专用工具设计**: 理解为什么需要 `read_file` 等专用工具，而不是什么都用 bash
4. **OpenAI 函数工具**: 了解如何使用 OpenAI 的 function calling 格式定义工具

---

## 六、扩展思考

完成基础练习后，可以思考以下问题：

1. 如何为工具添加更复杂的参数验证？
2. 如何实现工具执行的前置钩子（hook）和后置钩子？
3. 如何支持异步工具执行？
4. 如何实现工具的组合调用（一个工具调用另一个工具）？
