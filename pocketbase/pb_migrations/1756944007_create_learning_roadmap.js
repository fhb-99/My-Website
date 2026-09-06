/// <reference path="../pb_data/types.d.ts" />

migrate((app) => {
  let roadmap
  try {
    roadmap = app.findCollectionByNameOrId("learning_roadmap")
  } catch {
    roadmap = new Collection({
      type: "base",
      name: "learning_roadmap",
      listRule: "is_published = true",
      viewRule: "is_published = true",
      createRule: null,
      updateRule: null,
      deleteRule: null,
      fields: [
        { type: "text", name: "title", required: true, max: 200, presentable: true },
        { type: "text", name: "subtitle", max: 500 },
        { type: "text", name: "content_md" },
        {
          type: "file",
          name: "cover",
          maxSelect: 1,
          maxSize: 5242880,
          mimeTypes: ["image/jpeg", "image/png", "image/webp", "image/gif"],
        },
        { type: "bool", name: "is_published" },
        { type: "autodate", name: "created", onCreate: true },
        { type: "autodate", name: "updated", onCreate: true, onUpdate: true },
      ],
    })
    app.save(roadmap)
  }

  try {
    app.findFirstRecordByFilter(roadmap, "title != ''")
  } catch {
    const record = new Record(roadmap)
    record.set("title", "C++学习路线 - 从C++入门到入土")
    record.set("subtitle", "从语法基础、现代 C++ 到工程实践与方向进阶，按阶段建立可落地的学习路径。")
    record.set("content_md", `# C++ 学习路线

## 第一阶段：语法与基础能力

- 变量、类型、流程控制、函数与作用域
- 指针、引用、数组、字符串与基础内存模型
- 类、对象、封装、继承和多态
- 熟悉 vector、string、map、set 等常用 STL 容器

## 第二阶段：现代 C++

- RAII、智能指针、移动语义和资源管理
- lambda、模板、泛型编程与常用算法
- 掌握 C++11 到 C++20 中常用且稳定的语言特性

## 第三阶段：工程化实践

- 使用 CMake 组织多目录项目
- 学会调试、单元测试、日志和性能分析
- 理解编译、链接、静态库与动态库

## 第四阶段：系统能力

- Linux 常用工具、进程、线程与并发
- 网络编程、TCP/IP 和常见服务端模型
- 数据结构、算法与计算机基础知识

## 第五阶段：选择一个深入方向

- Qt 桌面客户端
- C++ 后端与高性能网络服务
- 音视频、WebRTC 与实时通信
- 图形渲染、游戏或嵌入式开发

## 第六阶段：用项目形成闭环

完成至少一个可以构建、运行、测试和部署的项目，并持续记录问题、方案、取舍与验证结果。`)
    record.set("is_published", true)
    app.save(record)
  }
}, () => {
  // 学习路线是独立内容，回滚时保留集合，避免管理员已经编辑的正文丢失。
})
