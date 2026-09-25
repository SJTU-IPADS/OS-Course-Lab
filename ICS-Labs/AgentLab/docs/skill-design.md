# Skill 设计指南

这份文档只讲 skill，不讲工具实现细节。目标是帮你写出“能指导模型行动，但不会把 prompt 撑爆”的 skill。

## skill 负责什么

好的 skill 通常回答四个问题：

- 什么时候该用这个 skill。
- 遇到这个场景时应该按什么步骤行动。
- 哪些检查项必须优先看。
- 最终回答或提交时有哪些硬性要求。

工具负责原子能力，skill 负责流程和判断标准。不要把两者混在一起。

## 推荐结构

一个实用的 `SKILL.md` 往往包含：

1. `When To Use`: 触发条件。
2. `Procedure`: 3-6 步的简短流程。
3. `Checklist` 或 `Decision Rules`: 关键判断标准。
4. `Final Answer` 或 `Submission Rules`: 输出语言、格式和必要字段。

## 好的 skill 长什么样

```markdown
---
name: patch-review
description: Review a patch for security and behavior risks.
---

## When To Use
Use this skill when the user asks for a patch review.

## Procedure
1. Read the diff.
2. Read related files only when the diff is not enough.
3. Check workspace boundaries, path handling, error handling, and tests.
4. If the patch reads user-controlled paths without workspace-safe resolution, request changes.
5. Submit the review after the verdict and comments are explicit.

## Comment Requirements
- Explain the concrete risk.
- Recommend a fix direction.
- Mention regression test coverage.
```

这个例子有几个特点：

- 短。
- 步骤是动作导向的。
- checklist 明确，但没有把当前场景答案写死。
- 没有重复工具 schema，也没有长篇解释背景故事。

## 常见反模式

避免这些写法：

- 只写一句“按要求完成任务”。这几乎没有指导价值。
- 把当前 eval 的完整答案、完整通知文本、完整 review 评论直接写进 skill。
- 重复大段工具文档或 system prompt 里已经出现的规则。
- 写很长的散文解释，让模型抓不到主线。

## 对中等模型更友好的写法

如果你用的模型指令遵循一般，优先这样写：

- 用短句和编号步骤，不要写多层嵌套说明。
- 把最关键的硬性要求写成 checklist。
- 必要时给一个非常短的 JSON 调用示例，但不要给太多例子。
- 如果模型反复漏掉某个要求，先把 skill 变短、更直接，而不是继续加长。

很多失败不是因为 skill 太短，而是因为 skill 太长、太散。

## 工具与 skill 的边界

- `data_redaction`: 工具做读取/验证/提交，skill 写“哪些信息要脱敏、哪些上下文要保留、何时验证”。
- `ephemeral_dispatch`: 工具处理 time-critical 的本地连续动作，skill 写“必须优先使用这个工具，不要拆成多轮”。
- `patch_review`: 工具做读 diff / 读文件 / 提交 review，skill 写 review checklist 和 verdict 规则。

## 什么时候该精简 prompt

如果你看到这些症状，优先精简 skill 或 system prompt：

- `parse_error` 很多。
- `estimated_total_tokens` 经常超限。
- 模型会复述大段规则，但不执行动作。
- patch_review 明明信息足够，模型却还在空谈总结。

精简方向通常是：

- 删掉背景介绍里的重复句。
- 把长段落改成 3-6 个 checklist 条目。
- 只保留一个最必要的调用示例。
- 不要把完整 skill 正文直接塞进 system prompt；让模型先 `load_skill`。
