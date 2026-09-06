# PocketBase 数据服务

这里保存博客使用的 PocketBase 数据结构，不提交可执行文件和运行数据。

## 本地启动

1. 从 PocketBase 官方发布页下载 `v0.40.2`。
2. 将可执行文件解压到当前目录。
3. 启动服务：

```powershell
.\pocketbase.exe serve --http=127.0.0.1:8090
```

首次启动会自动执行 `pb_migrations/`，随后访问：

- 管理后台：`http://127.0.0.1:8090/_/`
- 数据接口：`http://127.0.0.1:8090/api/`
- 中文博客管理端：`http://127.0.0.1:5174/admin/`

按管理后台提示创建第一个超级管理员。不要把管理员密码写入仓库。

## 启动用户端

复制 `InterfaceCode/user-web/.env.example` 为 `.env.local`，然后运行：

```powershell
cd ..\InterfaceCode\user-web
npm install
npm run dev
```

设置 `VITE_POCKETBASE_URL` 后，用户端会读取 PocketBase；未设置时仍使用原来的 C++ REST API，方便迁移期间回退。

## 启动中文管理端

```powershell
cd ..\InterfaceCode\admin-web
npm install
npm run dev
```

使用同一个 PocketBase 超级管理员账号登录。这个管理端只是另一套前端界面，所有数据仍保存在当前 PocketBase 服务中，不会新增第二个后端。

## 第一阶段约束

- 内容优先通过中文管理端维护，PocketBase 自带后台保留为底层数据管理工具。
- `posts.content_type` 使用 `article` 和 `interview` 区分普通文章与八股文；迁移前的空值按普通文章处理。
- `interview_categories` 提供八股文子目录的公开读取和管理员增删接口；`posts.interview_category` 保存文章所属目录名称，空值会在用户端归入“未分类”。
- `learning_roadmap` 独立保存 C++ 学习路线的标题、封面和 Markdown 正文；公开端只能读取已发布记录，写入仍仅限超级管理员。
- `pb_hooks/post_views.pb.js` 提供公开阅读上报接口；`post_view_events` 按访客、文章和日期去重，同一访客每天只增加一次阅读量。
- 中文管理端使用 Markdown 编辑文章；导入的 Markdown 默认保存为草稿。
- 评论和留言只保存昵称与正文，不上传访客邮箱。
- 阅读量通过 `pb_hooks/post_views.pb.js` 写回，并按访客、文章和日期去重。
- `pb_data/` 包含数据库和上传文件，部署时必须单独备份。

## 生产部署约束

- 用户端和管理端分别构建 `dist/`，不在服务器长期运行 Vite 开发服务。
- PocketBase 只监听服务器回环地址，由 Nginx 将同源 `/api/` 请求转发给它。
- 公网禁止访问 PocketBase 自带的 `/_/` 管理后台，内容管理统一使用 `/admin/`。
- 服务器只保留一个 PocketBase 超级管理员；账号口令只保存在 PocketBase 数据目录中。
- 发布前备份旧服务、前端和数据；发布后验证健康接口、公开内容和管理端静态资源。
