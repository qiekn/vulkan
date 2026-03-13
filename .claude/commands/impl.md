用户提供一个 Vulkan 教程文档的 URL: $ARGUMENTS

请按以下步骤执行：

1. **获取文档内容**：访问该 URL，阅读文档的完整内容
2. **提取代码链接**：在 HTML 中找到形如 `<a href="../../_attachments/xxx.cpp">C++ code</a>` 的链接，拼接出完整 URL 并展示给用户确认
3. **获取 C++ 代码**：访问该代码链接，获取完整的 C++ 源码
4. **讲解文档**：用中文向我讲解这个章节的核心概念和关键知识点
5. **合并代码**：将获取到的 C++ 代码 merge 到当前项目中，适配项目的代码风格（C++23 Modules、Vulkan-Hpp RAII、Google C++ Style PascalCase、`_` 后缀成员变量、`std::ranges`/`std::views` 等）
6. **构建验证**：编译项目确保通过
