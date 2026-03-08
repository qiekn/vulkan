---
description: 创建或更新 mdbook 文档
allowed-tools: Read, Write, Edit, Glob, Grep
---

用户输入: $ARGUMENTS

你是文档管理助手。根据用户输入执行以下操作：

## 创建新文档

当用户输入一个文件名（如 `pipeline`）时：

1. 创建 `docs/src/<name>.md`，写入合适的标题和初始内容
2. 更新 `docs/src/SUMMARY.md`，在末尾添加新条目

## 更新已有文档

当用户通过 @ 指定了已有文件时：

1. 读取该文件当前内容
2. 根据用户要求更新内容
3. 确保 SUMMARY.md 中已有对应条目，没有则补充

## 注意事项

- 文档使用中文书写
- 保持 SUMMARY.md 条目顺序合理
- 文件名使用 kebab-case
