# 博客网站当前实现缺口分析

本文档基于当前 `blog-platform` 代码状态整理：哪些能力已经实现，哪些前后端接口还没有对应实现，以及距离一个可上线、可运营的个人博客网站还缺哪些业务。

## 当前已实现能力

### C++ 后端

当前后端入口位于 `Back-end-server/backend-cpp/main.cpp`，后端只由 C++ 实现。

已注册接口：

```text
GET  /api/health
GET  /api/posts
GET  /api/posts/{id}
GET  /api/posts/slug/{slug}
POST /api/auth/login
GET  /api/admin/posts
POST /api/admin/posts
POST /api/admin/uploads/images
POST /api/admin/uploads/markdown
GET  /api/posts/{id}/comments
POST /api/posts/{id}/comments
```

已具备的核心能力：

- 公开文章列表、文章详情、slug 详情。
- 文章列表分页元信息：`total`、`total_pages`、`has_more`。
- 管理员登录，密码使用 PBKDF2 校验，登录后创建服务端 session。
- 管理接口通过 `Authorization: Bearer <token>` 鉴权。
- 管理员创建文章。
- 图片上传，保存到 `uploads/images` 并通过 `/uploads` 暴露。
- Markdown 上传，生成文章记录，并备份到 `content/posts`。
- 文章评论列表与评论提交。
- SQLite 初始化 `posts`、`users`、`admin_sessions`、`comments` 表。

### Vue 前端

当前前端已从原 HTML 方案切换为 Vue 3 + Vite + TypeScript：

```text
InterfaceCode/user-web   用户端
InterfaceCode/admin-web  管理端
InterfaceCode/shared     共享类型和 API 基础类型
```

用户端已有页面：

- 欢迎页
- 首页
- 文章列表
- 文章详情
- 评论区
- 留言板
- 关于
- 项目
- 碎碎念

用户端 API 层已实现：

- `postsApi`
- `commentsApi`
- `guestbookApi`
- `notesApi`
- `projectsApi`
- `siteConfigApi`

管理端已有页面：

- 登录页
- 管理首页
- 文章管理
- 上传管理
- 留言 / 评论管理
- 基础设置

管理端 API 层已实现：

- `authApi`
- `adminPostsApi`
- `uploadApi`
- `moderationApi`
- `settingsApi`

## 前后端接口缺口

下面这些接口已经被前端 API 层引用或预留，但 C++ 后端还没有对应路由，因此当前调用会返回 404 或无法完成真实业务。

### 用户端缺口

```text
GET  /api/search?q=keyword&limit=10
GET  /api/guestbook?page=1&limit=20
POST /api/guestbook
GET  /api/notes
GET  /api/projects
GET  /api/config
```

影响：

- 搜索框无法真实搜索文章。
- 留言板仍然只能显示占位内容，不能持久化留言。
- 碎碎念、项目、站点配置仍然依赖前端占位数据。
- 首页公告、精选内容、站点标题等还不能由后台配置。

### 管理端缺口

```text
GET    /api/admin/posts/{id}
PUT    /api/admin/posts/{id}
DELETE /api/admin/posts/{id}
GET    /api/admin/guestbook
PUT    /api/admin/guestbook/{id}/approve
DELETE /api/admin/guestbook/{id}
GET    /api/admin/comments
PUT    /api/admin/comments/{id}/approve
DELETE /api/admin/comments/{id}
GET    /api/admin/config
PUT    /api/admin/config/{key}
```

影响：

- 管理员目前只能创建文章，不能编辑、删除、查看草稿详情。
- 管理端文章列表当前复用了公开文章列表逻辑，只能看到已发布文章，不适合后台管理草稿。
- 评论目前默认直接展示，缺少审核、删除、隐藏能力。
- 留言管理还没有后端业务。
- 站点配置无法通过管理端维护。

### 前端页面接入缺口

虽然 API 层已经搭好，但部分页面仍是 UI 占位，尚未真正调用接口：

- 管理端登录页仍写入演示 token，尚未调用 `authApi.login`。
- 管理端文章保存按钮仍是占位，尚未调用 `adminPostsApi`。
- 管理端上传页面仍是占位，尚未调用 `uploadApi`。
- 管理端审核页面仍是占位，尚未调用 `moderationApi`。
- 管理端设置页面仍是占位，尚未调用 `settingsApi`。
- 用户端留言、项目、碎碎念、站点配置在接口失败时会回退到占位内容。

## 业务缺口

### P0：上线前必须补齐

1. 管理端真实登录流程

   管理端登录页需要改为调用 `POST /api/auth/login`，登录成功后保存 token，后续管理接口统一携带 `Authorization`。

2. 管理端文章完整 CRUD

   需要补齐：

   ```text
   GET    /api/admin/posts/{id}
   PUT    /api/admin/posts/{id}
   DELETE /api/admin/posts/{id}
   ```

   同时后台文章列表应返回草稿和未发布文章，而不是复用公开文章列表。

3. 留言板闭环

   需要新增 `guestbook` 表和公开接口：

   ```text
   GET  /api/guestbook
   POST /api/guestbook
   ```

   前端已有“先登记邮箱，再留言”的交互，但目前邮箱只保存在浏览器本地；后端仍需要保存邮箱用于管理侧查看或后续通知，但公开接口不能返回邮箱。

4. 评论 / 留言审核

   当前评论默认直接展示。上线前至少需要管理员删除能力，最好接入审核状态：

   ```text
   GET    /api/admin/comments
   PUT    /api/admin/comments/{id}/approve
   DELETE /api/admin/comments/{id}
   ```

5. 部署配置

   需要明确：

   - `BLOG_DB_PATH` 或其他方式配置数据库路径。
   - `uploads` 目录的绝对路径和写权限。
   - Nginx 反代 `/api/` 到 C++ 服务。
   - Nginx 或 C++ 服务暴露 `/uploads/`。
   - 生产环境关闭宽松 CORS，改为同源或指定域名。
   - systemd 守护进程和日志策略。

### P1：完整博客运营能力

1. 搜索

   后端 repo 已有 `search()`，但缺 HTTP 路由：

   ```text
   GET /api/search?q=keyword&limit=10
   ```

   第一版可以使用 SQLite `LIKE`，后续再升级 FTS5。

2. 站点配置

   需要 `site_config` 表支撑：

   - 站点标题
   - 副标题
   - 公告
   - 首页展示数量
   - 社交链接
   - 主题配置

3. 碎碎念和项目

   需要后端数据表和管理接口，让 `notes`、`projects` 不再写死在前端。

4. Markdown 渲染升级

   当前 Markdown 上传使用轻量渲染器，只适合第一版。后续建议接入成熟 Markdown/GFM 渲染能力，支持：

   - 代码块语言标记
   - 表格
   - 链接
   - 图片
   - 目录
   - 更完整的 HTML 安全策略

5. 上传资产治理

   需要补充：

   - 图片 MIME 二次校验。
   - 上传文件大小配置化。
   - 孤儿图片清理策略。
   - 删除文章时的关联资源处理。

### P2：安全和稳定性

1. SQLite 并发访问

   当前使用全局 SQLite 连接。`cpp-httplib` 可能并发处理请求，建议在 repo 层增加互斥保护，或者后续改为每请求连接 / 连接池。

2. 数据库迁移

   当前建表逻辑写在 `main.cpp`。随着表越来越多，建议抽离 migration 机制，避免生产库字段升级困难。

3. 访问限流

   评论、留言、登录、上传都需要限流，至少基于 IP 或 token 做简单防刷。

4. 日志和审计

   需要记录管理员操作：登录、创建文章、修改文章、删除文章、审核评论、删除留言等。

5. 数据备份

   上线前至少需要定期备份 SQLite 数据库和 `uploads` 目录。

## 建议实现顺序

1. 接真实管理端登录页：`authApi.login`。
2. 补后台文章详情、编辑、删除接口，并让管理端文章页调用真实接口。
3. 补留言板后端接口，让用户端留言闭环。
4. 补评论 / 留言管理接口，让互动内容可审核、可删除。
5. 补搜索接口，接用户端搜索。
6. 补站点配置、碎碎念、项目接口。
7. 做 Linux 部署配置：Nginx、systemd、数据库和上传目录权限、备份脚本。

## 本次 review 修复记录

本次检查中发现并修复了几个会影响真实使用的问题：

- 管理接口缺少 `Authorization` 时，原先 `RequireAdmin` 直接返回但不写响应；现在统一返回 401 JSON 错误。
- 登录接口原先在字段缺失或类型不对时可能抛出未捕获异常；现在会校验 `username` 和 `password` 后再进入鉴权。
- 评论接口依赖 `comments` 表，但数据库初始化中没有创建该表；现在启动时会自动创建 `comments` 表和索引。
- `global.h` 使用 `nlohmann::json`，现在显式包含 JSON 头，降低头文件隐式依赖。
- `post_handler.h` 中登录接口注释由 `GET` 修正为真实的 `POST`。

## 当前结论

当前网站已经有“文章读取 + 登录 + 管理员创建文章 + 图片上传 + Markdown 上传 + 文章评论”的基础能力，但还不是完整可运营博客。

真正上线前，最关键的缺口是：

- 管理端页面接真实登录和文章管理接口。
- 后端补齐文章编辑 / 删除。
- 留言板后端闭环。
- 评论 / 留言审核和删除。
- 部署、安全、备份和运行配置。
