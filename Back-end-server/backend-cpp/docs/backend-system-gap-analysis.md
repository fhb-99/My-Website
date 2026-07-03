# 博客网站当前实现缺口分析

本文档基于当前 `blog-platform` 最新代码状态整理：哪些能力已经实现，哪些只是部分实现，以及距离一个可上线、可运营的个人博客网站还缺哪些业务。

## 最近同步状态

- 已从远程 `my-website/develop` 拉取最新代码。
- 本次远程拉取为 fast-forward；恢复本地改动时，后端冲突文件已按“保留远程”处理。
- 远程最新代码补齐了管理端文章 CRUD、搜索路由、留言板路由、留言板查询 SQL，以及 `POST /api/guestbook` 的创建逻辑。
- 后端阅读量统计已调整为独立接口：`GET /api/posts/{id}` 只读取文章，`POST /api/posts/{id}/view` 统计有效阅读。
- 阅读量按“同一访客、同一文章、同一天”去重，前端会在用户停留一段时间后再上报。
- 用户端和管理端 API 层已经接入真实 `fetch` 通信，不再是单纯空壳。
- 管理端登录页已经调用 `POST /api/auth/login`，登录成功后保存 token 并用于后续管理端请求。
- 管理端线上 API 默认地址已修正为同源 `/api`，避免线上页面错误请求浏览器本机 `127.0.0.1:8080`。
- 用户端和管理端路由已切换为 hash 模式，减少静态部署时对 Nginx history fallback 的依赖。
- 管理端文章 CRUD 已补齐：后台列表包含草稿，页面支持详情回填、创建、更新和删除。
- 管理端 AI 辅助审核页面已搭建，后端已补齐审核配置持久化、审核日志、规则审核测试、评论 / 留言手动触发审核接口。
- 当前自动审核只实现本地规则版：屏蔽词、链接数量、自动拒绝、自动通过；DeepSeek 等第三方 AI 调用尚未接入。
- 注意：远程版阅读统计 SQL 仍需要在 Linux 环境做接口回归验证，避免语句细节导致运行时失败。

## 当前已实现能力

### C++ 后端

当前后端入口位于 `Back-end-server/backend-cpp/main.cpp`，后端只由 C++ 实现。

已注册接口：

```text
GET  /api/health
GET  /api/posts
GET  /api/posts/{id}
POST /api/posts/{id}/view
GET  /api/posts/slug/{slug}
POST /api/auth/login
GET  /api/admin/posts
POST /api/admin/posts
GET  /api/admin/posts/{id}
PUT  /api/admin/posts/{id}
DELETE /api/admin/posts/{id}
POST /api/admin/uploads/images
POST /api/admin/uploads/markdown
GET  /api/admin/moderation/config
PUT  /api/admin/moderation/config
GET  /api/admin/moderation/logs
POST /api/admin/moderation/test
GET  /api/admin/comments
PUT  /api/admin/comments/{id}/approve
PUT  /api/admin/comments/{id}/reject
DELETE /api/admin/comments/{id}
POST /api/admin/comments/{id}/moderate
GET  /api/admin/guestbook
PUT  /api/admin/guestbook/{id}/approve
PUT  /api/admin/guestbook/{id}/reject
DELETE /api/admin/guestbook/{id}
POST /api/admin/guestbook/{id}/moderate
GET  /api/posts/{id}/comments
POST /api/posts/{id}/comments
GET  /api/search?q=keyword&limit=10
GET  /api/guestbook?page=1&limit=20
POST /api/guestbook
```

已具备的核心能力：

- 公开文章列表、文章详情、slug 详情。
- 文章列表分页元信息：`total`、`total_pages`、`has_more`。
- 文章详情读取和阅读量统计已经拆分：读取文章不再自动增加阅读量。
- 阅读量统计通过 `post_view_events` 去重，同一访客同一天阅读同一文章只增加一次阅读量。
- 管理员登录，密码使用 PBKDF2 校验，登录后创建服务端 session。
- 管理接口通过 `Authorization: Bearer <token>` 鉴权。
- 管理员文章 CRUD：列表、详情、创建、更新、删除。
- 后台文章列表包含草稿和未发布文章，不再复用公开文章过滤逻辑。
- 删除文章时会同步清理文章评论和阅读统计事件，避免产生孤儿数据。
- 图片上传，保存到 `uploads/images` 并通过 `/uploads` 暴露。
- Markdown 上传，生成文章记录，并备份到 `content/posts`。
- 文章评论列表与评论提交。
- 搜索文章，当前基于 SQLite `LIKE` 查询标题、摘要和 Markdown 正文。
- 留言板公开列表与留言提交。
- 文章评论和留言板已经拆成不同结构体、不同数据表，业务边界更清楚。
- 管理端评论 / 留言人工审核接口已实现，包含列表、审核通过、审核拒绝和删除。
- 管理端自动审核规则链路已实现：配置持久化、审核日志、测试文本、评论 / 留言手动触发审核。
- SQLite 初始化 `posts`、`users`、`admin_sessions`、`comments`、`guestbook_messages`、`site_settings`、`moderation_logs` 表。

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

用户端已经接入或部分接入的接口：

- 文章列表会请求 `GET /api/posts`，失败时回退占位内容。
- 文章详情会请求 `GET /api/posts/{id}` 或 `GET /api/posts/slug/{slug}`，失败时回退占位内容。
- 文章详情加载成功后会延迟上报 `POST /api/posts/{id}/view`，用于有效阅读统计。
- 文章评论会请求 `GET/POST /api/posts/{id}/comments`。
- 留言板会请求 `GET/POST /api/guestbook`，后端当前已经有对应接口。
- 用户端会在本地保存匿名访客 ID，仅在阅读量上报时通过 `X-Visitor-Id` 发送给后端做去重。

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

管理端已经接入或部分接入的接口：

- 登录页调用 `authApi.login`，对接 `POST /api/auth/login`。
- 文章管理页已调用 `adminPostsApi.listPosts/getPost/savePost/deletePost`，可以读取后台文章列表、回填编辑、创建、更新和删除文章。
- 上传管理页已调用 `uploadApi.uploadImage/uploadMarkdown`，对接图片上传和 Markdown 上传接口。
- 评论 / 留言管理页已经调用 `moderationApi`，可以触发审核通过、审核拒绝和删除操作。
- `moderationApi` 已补齐审核通过、审核拒绝、删除、自动审核配置、审核日志、测试文本、评论 / 留言手动触发审核方法。
- 基础设置页已经调用 `settingsApi`，但后端站点配置接口尚未补齐。
- 管理端 token 会保存在本地，并通过 `Authorization: Bearer <token>` 发送给受保护接口。

## 仍未完整实现的业务

### P0：上线前仍需补齐

1. 管理端页面继续补齐真实业务

   API 层和部分页面已经接入真实接口，但仍有业务未闭环：
   - 文章管理页已有基础 CRUD 交互，后续可继续优化分页、搜索、批量操作和更完整的编辑体验。
   - 审核页已调用 `moderationApi`，人工审核 / 删除流程已经接通，后续可补批量操作和审核原因。
   - 设置页已调用 `settingsApi`，但后端缺站点配置接口。
   - 管理首页统计数据仍是静态占位。
   - token 过期后的自动跳转登录、续期或提示流程还需要完善。

2. 评论 / 留言审核管理

   管理端人工审核和删除已完成：后台可以查看评论 / 留言，执行通过、拒绝和删除操作。拒绝会把 `is_approved` 更新为 `false`，公开接口只展示 `is_approved = true` 的内容。

   已实现人工审核接口：

   ```text
   GET    /api/admin/comments
   PUT    /api/admin/comments/{id}/approve
   PUT    /api/admin/comments/{id}/reject
   DELETE /api/admin/comments/{id}

   GET    /api/admin/guestbook
   PUT    /api/admin/guestbook/{id}/approve
   PUT    /api/admin/guestbook/{id}/reject
   DELETE /api/admin/guestbook/{id}
   ```

   已实现自动审核规则接口：

   ```text
   GET    /api/admin/moderation/config
   PUT    /api/admin/moderation/config
   GET    /api/admin/moderation/logs
   POST   /api/admin/moderation/test
   POST   /api/admin/comments/{id}/moderate
   POST   /api/admin/guestbook/{id}/moderate
   ```

   当前自动审核已经支持配置持久化和审核日志：
   - 配置存储在 `site_settings` 表的 `moderation_config` 中。
   - 审核日志写入 `moderation_logs` 表。
   - `agent_enabled = false` 时只返回 `pending`，不自动处理内容。
   - 命中屏蔽词或链接数量超过 `max_links` 时，根据 `auto_reject_enabled` 决定是否自动拒绝。
   - 未命中风险规则时，根据 `auto_approve_enabled` 决定是否自动通过。

   后续如果要默认先审核再展示，只需要把新评论 / 新留言的 `is_approved` 默认值改为 `false`，再通过管理端审核接口放行。

   后续自动审核 agent 建议继续按三层推进：
   - 第一层规则审核：已完成屏蔽词和链接数量；后续可补内容长度、重复提交、IP / 邮箱频率限制。
   - 第二层 AI 辅助审核：接入 DeepSeek 等模型，只对规则无法明确判断的内容做二次判断。
   - 第三层审计与兜底：所有自动通过 / 拒绝都记录审核原因，管理端可以人工改判；agent 关闭时仍保留人工审核流程。

   不建议一开始就完全依赖大模型审核。更稳的做法是“规则引擎优先 + agent 辅助 + 人工兜底”：成本低、实时性好，也方便解释为什么某条评论被拒绝。

3. DeepSeek AI 审核接入

   当前还没有真正调用 DeepSeek API，只是预留了管理端配置字段：`provider`、`api_base_url`、`model`、`system_prompt`、`confidence_threshold`。

   DeepSeek 官方 OpenAI 兼容 API Base URL：

   ```text
   https://api.deepseek.com
   ```

   生产环境建议使用服务端环境变量，不要把真实 API Key 写入代码、前端或文档：

   ```bash
   export DEEPSEEK_API_BASE_URL="https://api.deepseek.com"
   export DEEPSEEK_API_KEY="替换为服务器上的真实密钥"
   export DEEPSEEK_MODEL="deepseek-v4-flash"
   ```

   接入时建议新增后端服务逻辑：
   - 只在 C++ 后端调用 DeepSeek，前端永远不接触 API Key。
   - 请求使用 `Authorization: Bearer ${DEEPSEEK_API_KEY}`。
   - 让模型只返回 JSON，例如 `{ "decision": "approved|pending|rejected", "reason": "...", "confidence": 0.0 }`。
   - 对模型返回做严格校验，非法输出一律降级为 `pending`。
   - 对超时、429、5xx 做失败兜底，不要影响评论 / 留言写入。
   - 所有 AI 判定结果写入 `moderation_logs`，方便后续排查误判。

4. 搜索前端接入与返回结构对齐

   后端已经有：

   ```text
   GET /api/search?q=keyword&limit=10
   ```

   但用户端当前还没有真正的搜索 UI 页面或搜索结果展示流程。

   另外，当前用户端 `postsApi.searchPosts()` 类型声明为直接返回 `PostSummary[]`，而后端 `HandleSearchPosts()` 返回结构是：

   ```json
   {
     "data": [],
     "message": "success"
   }
   ```

   后续需要统一接口契约：要么后端直接返回数组，要么前端改为读取 `data` 字段。

5. 部署配置

   需要明确：
   - `BLOG_DB_PATH` 或其他方式配置数据库路径。
   - `uploads` 目录的绝对路径和写权限。
   - Nginx 反代 `/api/` 到 C++ 服务。
   - Nginx 或 C++ 服务暴露 `/uploads/`。
   - 用户端 `dist/*` 部署到 `/var/www/gentleyun/html/`。
   - 管理端 `dist/*` 部署到 `/var/www/gentleyun/html/admin/`。
   - 管理端线上请求应走同源 `/api`，不要再请求 `http://127.0.0.1:8080`。
   - DeepSeek API Key 必须放在服务端环境变量或 systemd `EnvironmentFile` 中，不能提交到 Git。
   - 生产环境关闭宽松 CORS，改为同源或指定域名。
   - systemd 守护进程和日志策略。

### P1：完整博客运营能力

1. 站点配置

   前端已有 `siteConfigApi`，但后端还没有：

   ```text
   GET /api/config
   GET /api/admin/config
   PUT /api/admin/config/{key}
   ```

   建议增加 `site_config` 表支撑：
   - 站点标题
   - 副标题
   - 公告
   - 首页展示数量
   - 社交链接
   - 主题配置

2. 碎碎念和项目

   前端已有 `notesApi`、`projectsApi`，但后端还没有：

   ```text
   GET /api/notes
   GET /api/projects
   ```

   也还没有对应管理端 CRUD。需要后端数据表和管理接口，让 `notes`、`projects` 不再写死在前端。

3. Markdown 渲染升级

   当前 Markdown 上传使用轻量渲染器，只适合第一版。后续建议接入成熟 Markdown/GFM 渲染能力，支持：
   - 代码块语言标记
   - 表格
   - 链接
   - 图片
   - 目录
   - 更完整的 HTML 安全策略

4. 上传资产治理

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

1. 在 Linux 环境编译并回归测试自动审核配置、审核日志、评论 / 留言手动审核接口。
2. 接入 DeepSeek AI 辅助审核，但保留规则审核优先和人工兜底。
3. 将新评论 / 新留言改为默认待审核，并在创建时触发规则 / AI 审核。
4. 接用户端搜索 UI，并统一搜索接口返回结构。
5. 补站点配置接口，让备案号、站点标题、社交链接等信息后端化。
6. 补碎碎念、项目接口和对应管理端 CRUD。
7. 完善 Linux 部署配置：Nginx、systemd、数据库和上传目录权限、备份脚本、静态资源缓存策略。
8. 增加运行日志、访问日志和管理员操作审计，方便线上排查问题。

## 当前结论

当前网站已经有“文章读取 + 有效阅读统计 + 登录 + 管理员文章 CRUD + 图片上传 + Markdown 上传 + 文章评论 + 公开留言板 + 基础搜索”的后端基础能力。

Vue 用户端和管理端已经不再是纯静态占位工程：用户端文章、评论、留言等流程已经接入真实接口；管理端登录、文章创建、上传等流程也已经接入真实接口。

真正上线前，最关键的缺口是：

- DeepSeek AI 辅助审核接入，以及新评论 / 新留言默认待审核策略。
- 搜索前端 UI 与返回结构对齐。
- 站点配置、碎碎念、项目后端化。
- 部署、安全、日志、备份和运行配置。
