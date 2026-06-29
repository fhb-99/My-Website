# blog-platform

个人博客网站项目，当前主要由两部分组成：

```text
blog-platform/
├── Back-end-server/backend-cpp/   # C++ 后端服务，提供 /api/* 接口
└── InterfaceCode/                 # Vue 前端工程
    ├── user-web/                  # 用户端
    ├── admin-web/                 # 管理端
    └── shared/                    # 前端共享类型与 API 基础类型
```

## 当前技术栈

- 后端：C++、cpp-httplib、SQLiteCpp、SQLite、nlohmann/json。
- 前端：Vue 3、Vite、TypeScript、Vue Router、Pinia。
- 数据库：SQLite。

## 已有核心能力

- 公开文章列表和详情。
- slug 访问文章。
- 管理员登录和 token session。
- 管理员创建文章。
- 图片上传和 Markdown 上传。
- 文章评论列表和提交。
- Vue 用户端和管理端工程化结构。

## 重要文档

- 后端与业务缺口分析：`Back-end-server/backend-cpp/docs/backend-system-gap-analysis.md`
- 前端工程说明：`InterfaceCode/README.md`

## 本地开发提示

前端分别进入两个工程安装依赖并启动：

```bash
cd InterfaceCode/user-web
npm install
npm run dev

cd ../admin-web
npm install
npm run dev
```

后端在 Linux 环境中构建和运行，默认监听：

```text
http://0.0.0.0:8080
```

前端默认后端地址为：

```text
http://127.0.0.1:8080
```

也可以通过 `VITE_API_BASE_URL` 配置。
