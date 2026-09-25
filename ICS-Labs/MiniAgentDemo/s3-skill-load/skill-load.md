# 技能系统 - 按需加载知识

## 一、项目简介

本阶段实现双层技能注入机制，解决将大量领域知识注入 Agent 时的 token 浪费问题。

核心思路：**按需加载知识，而非一次性全部注入** —— 通过 tool_result 注入，而不是塞进系统提示。

### 核心工作流程

```
系统提示（第一层 —— 始终存在）：
+--------------------------------------+
| 你是一个编程 Agent。                  |
| 可用技能：                            |
|   - git: Git 工作流助手               |  ~100 tokens/技能
|   - test: 测试最佳实践                |
+--------------------------------------+

当模型调用 load_skill("git")：
+--------------------------------------+
| 工具结果（第二层 —— 按需加载）：       |
| <skill name="git">                   |
|   完整的 Git 工作流说明...            |  ~2000 tokens
|   步骤 1: ...                         |
| </skill>                             |
+--------------------------------------+
```

第一层：系统提示中包含技能*名称*（开销低）。第二层：通过 tool_result 注入完整*内容*（按需加载）。

### 主要组件

1. **SkillLoader 类**：扫描 `skills/*/SKILL.md` 并解析 YAML 前置元信息
2. **双层注入**：第一层（系统提示）开销低，第二层（工具结果）按需加载
3. **技能结构**：每个技能是一个目录，包含 `SKILL.md` 文件
4. **load_skill 工具**：Agent 调用该工具加载完整技能内容

---

## 二、任务说明

本练习需要理解双层技能注入机制，并掌握技能的定义和使用方式。

### 任务 1: 理解 SkillLoader 实现

**核心代码**:
```python
class SkillLoader:
    def __init__(self, skills_dir: Path):
        self.skills = {}
        for f in sorted(skills_dir.rglob("SKILL.md")):
            text = f.read_text()
            meta, body = self._parse_frontmatter(text)
            name = meta.get("name", f.parent.name)
            self.skills[name] = {"meta": meta, "body": body}

    def get_descriptions(self) -> str:
        lines = []
        for name, skill in self.skills.items():
            desc = skill["meta"].get("description", "")
            lines.append(f"  - {name}: {desc}")
        return "\n".join(lines)

    def get_content(self, name: str) -> str:
        skill = self.skills.get(name)
        if not skill:
            return f"错误: 未知技能 '{name}'。"
        return f"<skill name=\"{name}\">\n{skill['body']}\n</skill>"
```

**关键点**:
- 扫描 `skills/` 目录下所有 `SKILL.md` 文件
- 使用目录名作为技能标识符
- `get_descriptions()` 返回第一层内容（轻量）
- `get_content()` 返回第二层内容（完整）

### 任务 2: 理解双层注入机制

**第一层（系统提示）**：
```python
def get_completion(messages: list):
    system = f"""你是一个位于 {WORKDIR} 的编程 Agent。
使用 load_skill 来访问专业知识。

可用技能：
{SKILL_LOADER.get_descriptions()}"""

    request = {
        "model": MODEL,
        "messages": [{"role": "system", "content": system}, *messages],
        "tools": TOOLS,
    }
    return client.chat.completions.create(**request)
```

**第二层（工具结果）**：
```python
TOOL_HANDLERS = {
    # ...其他工具...
    "load_skill": lambda **kw: SKILL_LOADER.get_content(kw["name"]),
}
```

**关键点**:
- 第一层在系统提示中列出技能名称和描述（~100 tokens/技能）
- 第二层通过 `load_skill` 工具按需加载完整内容（~2000 tokens/技能）
- 只有被请求的技能才会消耗大量 token

### 任务 3: 理解技能文件结构

每个技能是一个目录，包含 `SKILL.md` 文件：
```
skills/
  pdf/
    SKILL.md       # ---\n name: pdf\n description: 处理 PDF 文件\n ---\n ...
  code-review/
    SKILL.md       # ---\n name: code-review\n description: 代码审查\n ---\n ...
```

`SKILL.md` 文件结构：
```markdown
---
name: pdf
description: 处理 PDF 文件
---

# PDF 处理技能

## 步骤 1: ...
## 步骤 2: ...
```

---

## 三、测试方法

> **测试方式**：通过 CLI Agent 在命令行中与 Agent 进行交互完成测试。

完成代码理解后，按以下步骤进行测试：

### 1. 配置环境变量

```sh
export OPENROUTER_API_KEY="你的 API 密钥"
export OPENROUTER_BASE_URL="https://openrouter.ai/api/v1"
export MODEL_ID="tencent/hy3-preview:free"  # 或你喜欢的模型
```

### 2. 创建测试技能

```sh
cd s3-skill-load
mkdir -p skills/test
cat > skills/test/SKILL.md << 'EOF'
---
name: test
description: 示例测试技能
---
这是一个用于演示的测试技能。
用它来理解技能加载机制。
EOF
```

### 3. 运行程序

```sh
python skill-load.py
```

### 4. 输入测试命令

程序启动后，会显示提示符 `s3 >>`，尝试以下操作：

1. `列出所有可用技能`
2. `加载 test 技能并展示其内容`
3. `创建一个 demo.py 文件，内容为 print('Hello from skill loaded agent')`

### 5. 预期结果

如果一切正常，你应该看到类似以下的输出：

```
s3 >> 列出所有可用技能
根据系统信息，当前可用技能如下：

- **test**：示例测试技能

如需使用任何技能，我可以通过 `load_skill` 函数加载。请告诉我你想使用哪个技能！
```

### 6. 退出程序

输入 `q` 或 `exit` 或直接按回车键退出程序。

---

## 四、标准答案

以下是完整的核心代码实现，用于参考和验证。

### SkillLoader 完整实现

```python
class SkillLoader:
    def __init__(self, skills_dir: Path):
        self.skills = {}
        for f in sorted(skills_dir.rglob("SKILL.md")):
            text = f.read_text()
            meta, body = self._parse_frontmatter(text)
            name = meta.get("name", f.parent.name)
            self.skills[name] = {"meta": meta, "body": body}

    def _parse_frontmatter(self, text: str) -> tuple[dict, str]:
        if text.startswith("---\n"):
            end = text.find("\n---\n", 4)
            if end != -1:
                import yaml
                meta = yaml.safe_load(text[4:end]) or {}
                body = text[end + 5:]
                return meta, body
        return {}, text

    def get_descriptions(self) -> str:
        lines = []
        for name, skill in self.skills.items():
            desc = skill["meta"].get("description", "")
            lines.append(f"  - {name}: {desc}")
        return "\n".join(lines)

    def get_content(self, name: str) -> str:
        skill = self.skills.get(name)
        if not skill:
            return f"错误: 未知技能 '{name}'。"
        return f"<skill name=\"{name}\">\n{skill['body']}\n</skill>"
```

### 系统提示构建

```python
def get_completion(messages: list):
    system = f"""你是一个位于 {WORKDIR} 的编程 Agent。
使用 load_skill 来访问专业知识。

可用技能：
{SKILL_LOADER.get_descriptions()}"""

    request = {
        "model": MODEL,
        "messages": [{"role": "system", "content": system}, *messages],
        "tools": TOOLS,
    }
    return client.chat.completions.create(**request)
```

---

## 五、学习要点

通过这个练习，你将学习到：

1. **双层知识注入**: 理解如何通过两层结构平衡 token 开销和知识完整性
2. **技能系统设计**: 学习如何组织和管理领域专用知识
3. **按需加载模式**: 理解为什么以及如何在需要时加载资源
4. **YAML 前置元信息**: 了解如何使用 frontmatter 管理结构化元数据

---

## 六、扩展思考

完成基础练习后，可以思考以下问题：

1. 如何实现技能的版本管理？
2. 如何支持技能的依赖关系（技能 A 依赖技能 B）？
3. 如何实现技能的缓存机制，避免重复加载？
4. 如何为技能添加权限控制（某些技能需要授权才能加载）？
