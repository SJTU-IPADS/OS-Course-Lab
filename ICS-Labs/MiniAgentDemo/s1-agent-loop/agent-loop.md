# Agent Loop - 最简单的 AI Agent 框架

## 一、项目简介

`agent-loop.py` 是一个最简化的 AI Agent 框架实现，展示了 AI 编程助手的核心工作原理。整个框架的核心是一个简单而强大的循环模式：

```
while stop_reason == "tool_use":
    response = LLM(messages, tools)
    execute tools
    append results
```

### 核心工作流程

```
+----------+      +-------+      +---------+
|   User   | ---> |  LLM  | ---> |  Tool   |
|  prompt  |      |       |      | execute |
+----------+      +---+---+      +----+----+
                      ^               |
                      |   tool_result |
                      +---------------+
                      (loop continues)
```

### 主要组件

1. **LLM 客户端**: 通过 OpenRouter API 连接大语言模型
2. **工具定义**: 定义了 `bash` 工具，允许模型执行 shell 命令
3. **Agent 循环**: 核心循环，不断调用 LLM、执行工具、返回结果，直到模型决定停止
4. **安全机制**: 包含危险命令拦截和超时保护

---

## 二、任务说明

本练习需要你在两处 `TODO` 位置进行代码补全，体验 IDE 中使用 Tab 自动补全的编程方式。

### TODO 1: 实现 `run_bash` 函数

**位置**: 第 75-77 行

**任务要求**:
- 使用 `subprocess.run` 执行 shell 命令
- 设置 120 秒超时
- 限制输出长度，防止输出过长

**提示**:
- 使用 `shell=True` 参数
- 使用 `capture_output=True` 和 `text=True` 捕获输出
- 合并 stdout 和 stderr
- 对输出进行长度限制

### TODO 2: 实现 `agent_loop` 函数中的 LLM 调用

**位置**: 第 114-116 行

**任务要求**:
- 调用 `get_completion(messages)` 获取 LLM 响应
- 从响应中提取 assistant message 和 tool_calls
- 配置 `assistant_record` 字典

**提示**:
- 响应结构: `response.choices[0].message`
- 需要提取 `content` 和 `tool_calls` 属性
- `assistant_record` 需要包含 `role` 和 `content` 字段

---

## 三、测试方法

完成代码补全后，按以下步骤进行测试：

### 1. 激活 Python 环境

```bash
# 根据你的环境配置激活对应的虚拟环境
# 例如:
source venv/bin/activate  # Linux/Mac
# 或
venv\Scripts\activate     # Windows
```

### 2. 运行程序

```bash
python agent-loop.py
```

### 3. 输入测试命令

程序启动后，会显示提示符 `s1 >>`，输入以下测试命令：

```
请你帮我执行一次命令,帮我查看当前日期和时间
```

### 4. 预期结果

如果代码补全正确，你应该看到类似以下的输出：

```
[LOG]: Tool call: date
[LOG]: Running bash command: date
Sat Apr 26 10:30:45 CST 2025
```

LLM 会调用 `bash` 工具执行 `date` 命令，并正确返回当前日期和时间。

### 5. 退出程序

输入 `q` 或 `exit` 或直接按回车键退出程序。

---

## 四、标准答案

以下是两处 TODO 的标准答案，如果 tab 自动补全出现错误，或者无法正确排除错误，可以参考如下答案。

### TODO 1 标准答案

**位置**: `run_bash` 函数中

```python
def run_bash(command: str) -> str:
    try:
        print(f"[LOG]: Running bash command: {command}")
        r = subprocess.run(command, shell=True, cwd=WORKDIR,
                           capture_output=True, text=True, timeout=120)
        out = (r.stdout + r.stderr).strip()
        return out[:50000] if out else "(no output)"
    except subprocess.TimeoutExpired:
        return "Error: Timeout (120s)"
```

**关键点**:
- 使用 `subprocess.run` 执行命令
- `shell=True`: 通过 shell 执行命令
- `cwd=WORKDIR`: 在工作目录中执行
- `capture_output=True`: 捕获标准输出和错误输出
- `text=True`: 以文本形式返回输出
- `timeout=120`: 设置 120 秒超时
- 合并 stdout 和 stderr，并限制最大长度为 50000 字符

### TODO 2 标准答案

**位置**: `agent_loop` 函数中

```python
def agent_loop(messages: list):
    while True:
        # 1) Ask model what to do next.
        response = get_completion(messages)
        assistant_message = response.choices[0].message
        tool_calls = assistant_message.tool_calls or []

        assistant_record = {"role": "assistant", "content": assistant_message.content or ""}
        # 2) Record tool calls in the conversation transcript.
        if tool_calls:
        # ... 后续代码
```

**关键点**:
- 调用 `get_completion(messages)` 获取 LLM 响应
- 从 `response.choices[0].message` 提取消息内容
- 提取 `tool_calls` 属性，默认为空列表 `[]`
- 构建 `assistant_record` 字典，包含 `role` 和 `content`
- 使用 `or ""` 确保 content 不为 None

---

## 五、学习要点

通过这个练习，你将学习到：

1. **AI Agent 的核心模式**: 理解 Agent 如何通过循环调用 LLM 和工具来完成任务
2. **工具调用机制**: 了解 LLM 如何通过 function calling 调用外部工具
3. **消息历史管理**: 理解对话历史在 Agent 系统中的重要性
4. **安全考虑**: 学习如何在 Agent 中实现基本的安全防护

## 六、扩展思考

完成基础练习后，可以思考以下问题：

1. 如何添加更多的工具（如文件读写、网络请求）？
2. 如何实现更复杂的错误处理和重试机制？
3. 如何添加对话历史的长度限制？
4. 如何实现多轮对话的上下文管理？