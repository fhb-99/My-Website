# C++ 博客后台系统缺口分析

本文档基于当前 `Back-end-server/backend-cpp` 的实现状态，整理距离一个完整博客后台系统还缺少的能力，以及建议的实现顺序。

## 当前已有能力

- `GET /api/health`：服务健康检查。
- `GET /api/posts`：公开文章列表。
- `GET /api/posts/{id}`：公开文章详情。
- SQLite `posts` 表初始化。
- `Post` 模型、`PostRepo` 接口、`PostRepoSqlite` 实现、文章 handler 的基础分层。
- 公开文章接口默认过滤 `is_published = 1`，避免草稿暴露。
- 基础参数校验与异常兜底，内部错误返回统一 JSON。

当前后端已经具备“前台读取文章”的基础，但还不是完整后台系统。完整后台还需要内容管理、鉴权、互动、配置、搜索、上传、部署治理等能力。

## P0：Phase 1 收尾

这些事项建议优先完成，否则后续功能会建立在不稳定基础上。

1. Linux 环境验证

   在目标 Linux 环境中验证构建和接口：

   ```bash
   cmake --build build
   ./bin/blog-server
   curl http://127.0.0.1:8080/api/health
   curl http://127.0.0.1:8080/api/posts
   curl http://127.0.0.1:8080/api/posts/1
   ```

2. 数据库路径配置化

   当前数据库路径是 `data/blog.db`，依赖进程工作目录。建议支持环境变量：

   ```text
   BLOG_DB_PATH=/opt/blog-platform/Back-end-server/backend-cpp/data/blog.db
   ```

   systemd 中同时设置 `WorkingDirectory`，避免相对路径失效。

3. SQLite 并发访问处理

   当前服务使用单个全局 SQLite 连接。`cpp-httplib` 可能并发处理请求，短期建议在 repo 层增加 `std::mutex`，所有 SQL 操作加锁；后续再考虑每请求连接或连接池。

4. 分页元信息

   `GET /api/posts` 建议增加：

   ```json
   {
     "data": [],
     "page": 1,
     "limit": 10,
     "total": 42,
     "total_pages": 5,
     "has_more": true
   }
   ```

5. slug 查询

   真实博客更适合使用稳定的 slug 链接。建议补：

   ```text
   GET /api/posts/slug/{slug}
   ```

## P1：管理后台与鉴权

完整后台最核心的缺口是“管理员如何发文章、改文章、删除文章、管理草稿”。

建议接口：

```text
POST   /api/auth/login
GET    /api/admin/posts
GET    /api/admin/posts/{id}
POST   /api/admin/posts
PUT    /api/admin/posts/{id}
DELETE /api/admin/posts/{id}
```

公开接口只返回已发布文章；管理接口应能查看草稿、编辑草稿、发布或取消发布。

建议新增能力：

- 管理员登录。
- JWT 或 session 鉴权。
- 管理接口统一校验 `Authorization: Bearer <token>`。
- 管理员密码使用哈希存储，不能明文存储。
- `BLOG_JWT_SECRET`、`BLOG_ADMIN_PASSWORD` 等敏感配置从环境变量读取。

可选数据表：

```sql
CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT NOT NULL UNIQUE,
    password_hash TEXT NOT NULL,
    created_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
);
```

个人博客第一版也可以先只用环境变量中的管理员密码，但生产环境仍建议使用哈希校验。

## P2：文章内容生产链路

当前 `posts` 表已有 `content_md` 和 `content_html`，但还需要确定 Markdown 到 HTML 的稳定流程。

推荐第一版采用“导入时渲染”：

1. 管理员编写 Markdown。
2. 导入工具或管理接口解析 Markdown。
3. 同时写入 `content_md` 和 `content_html`。
4. 读取接口直接返回 `content_html`。

后续需要补齐：

- Markdown 渲染库，例如 `cmark-gfm`。
- 文章摘要自动生成。
- 标签解析和标签筛选。
- 封面图路径管理。
- 文章内图片路径规范。
- 草稿、发布、取消发布状态切换。

## P3：留言板与评论

当前前端存在留言板页面，但后端尚未支持留言。

建议接口：

```text
GET    /api/guestbook?page=1&limit=20
POST   /api/guestbook
PUT    /api/admin/guestbook/{id}/approve
DELETE /api/admin/guestbook/{id}
```

建议数据表：

```sql
CREATE TABLE IF NOT EXISTS guestbook (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    nickname TEXT NOT NULL,
    email TEXT DEFAULT '',
    content TEXT NOT NULL,
    ip_hash TEXT DEFAULT '',
    user_agent TEXT DEFAULT '',
    approved INTEGER DEFAULT 0,
    created_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
);
```

必须处理：

- 昵称和内容长度限制。
- HTML 转义，防止 XSS。
- IP 限流，防止刷留言。
- 默认审核后展示，或直接展示但保留删除能力。
- 管理员审核和删除。

如果后续支持文章评论，可增加 `comments` 表，并关联 `post_id`。

## P4：碎碎念、项目和站点配置

前端已有 `notes.html`、`projects.html`、首页精选、公告、音乐等页面或模块，但后端还没有接管这些数据。

建议公开接口：

```text
GET /api/notes
GET /api/projects
GET /api/config
```

建议管理接口：

```text
POST   /api/admin/notes
PUT    /api/admin/notes/{id}
DELETE /api/admin/notes/{id}

POST   /api/admin/projects
PUT    /api/admin/projects/{id}
DELETE /api/admin/projects/{id}

PUT    /api/admin/config/{key}
```

建议数据表：

```sql
CREATE TABLE IF NOT EXISTS notes (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    content TEXT NOT NULL,
    is_published INTEGER DEFAULT 1,
    created_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
);

CREATE TABLE IF NOT EXISTS projects (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    title TEXT NOT NULL,
    description TEXT DEFAULT '',
    url TEXT DEFAULT '',
    repo_url TEXT DEFAULT '',
    tags TEXT DEFAULT '[]',
    sort_order INTEGER DEFAULT 0,
    is_visible INTEGER DEFAULT 1,
    created_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
);

CREATE TABLE IF NOT EXISTS site_config (
    key TEXT PRIMARY KEY,
    value_json TEXT NOT NULL,
    updated_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
);
```

`site_config` 可用于公告、精选文章 ID、音乐列表、站点标题、社交链接等。

## P5：搜索

当前 repo 层已有 `search()`，但还缺 HTTP 路由。

建议接口：

```text
GET /api/search?q=cpp&limit=10
```

第一版可用 `LIKE` 查询标题、摘要和正文。后续建议升级为 SQLite FTS5，因为当前 CMake 已启用 `SQLITE_ENABLE_FTS5`。

升级方向：

- FTS5 全文索引。
- 搜索结果高亮。
- 标题、摘要、正文不同权重。
- 搜索为空、无结果、查询过长的明确响应。

## P6：图片上传与静态资源

完整后台需要支持封面图和文章内图片上传。

建议接口：

```text
POST /api/admin/uploads
```

需要处理：

- 只允许图片 MIME 类型。
- 限制文件大小。
- 使用服务端生成的安全文件名。
- 保存到 `uploads/`。
- 由 Nginx 或 C++ 服务暴露 `/uploads/*`。
- 删除文章时考虑是否清理孤儿图片。

## P7：前后端对接

当前前端仍有较多静态内容。完整博客需要逐步改为 API 驱动。

建议对接顺序：

1. `posts.html` 调用 `/api/posts`。
2. 文章详情页调用 `/api/posts/{id}` 或 slug API。
3. `guestbook.html` 调用 `/api/guestbook`。
4. `home.html` 调用 `/api/config` 和 `/api/posts?limit=...`。
5. 搜索框调用 `/api/search?q=...`。

建议抽离公共 JS：

```text
InterfaceCode/assets/js/api.js
```

公共 JS 负责：

- API base path。
- fetch 错误处理。
- loading、empty、error 状态。
- 时间格式化。

## P8：Linux 部署治理

上线部署还需要：

- systemd service 文件。
- Nginx 配置：`/` 走静态前端，`/api/` 反代到 C++ 服务。
- HTTPS 由 Nginx 处理。
- 日志输出策略。
- 数据库备份脚本。
- 环境变量配置。
- 生产环境关闭宽松 CORS。
- 确认 `build/`、`data/`、`uploads/` 不进入 git。

systemd 示例：

```ini
[Unit]
Description=Blog C++ API Server
After=network.target

[Service]
WorkingDirectory=/opt/blog-platform/Back-end-server/backend-cpp
Environment=BLOG_DB_PATH=/opt/blog-platform/Back-end-server/backend-cpp/data/blog.db
ExecStart=/opt/blog-platform/Back-end-server/backend-cpp/bin/blog-server
Restart=always
RestartSec=3

[Install]
WantedBy=multi-user.target
```

## 建议实施顺序

1. 收尾 Phase 1：Linux 构建验证、DB 路径配置、SQLite 加锁、分页总数、slug 查询。
2. Phase 2：留言板 `GET/POST /api/guestbook`。
3. Phase 3：管理员登录、JWT 鉴权、文章 CRUD。
4. 前端对接文章列表和详情页。
5. Linux 部署：Nginx、systemd、备份脚本。
6. 扩展搜索、上传、碎碎念、项目、站点配置。

## 总结

当前 C++ 后端已经具备“公开文章读取 API”的基础。距离完整博客后台系统，还缺：

- 管理员鉴权。
- 文章管理 CRUD。
- 留言板与审核。
- Markdown 内容生产链路。
- 搜索。
- 上传。
- 站点配置。
- 前端 API 对接。
- Linux 部署治理。

优先把“文章读写闭环”和“留言板”做出来，博客就能从静态展示进入可运营状态。
