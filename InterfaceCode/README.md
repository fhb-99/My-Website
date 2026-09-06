# InterfaceCode 前端重构说明

当前主前端已切换为两个独立 Vue 工程：

- `user-web/`：用户端博客界面。
- `admin-web/`：管理端界面。
- `shared/`：两个工程共享的类型与 API 基础类型。

旧的根目录 HTML 文件暂时保留为历史参考，不再作为新前端主入口。

## 本地运行

```bash
cd InterfaceCode/user-web
npm install
npm run dev
```

```bash
cd InterfaceCode/admin-web
npm install
npm run dev
```

本地地址分别为用户端 `http://127.0.0.1:5173/` 和管理端 `http://127.0.0.1:5174/admin/`。管理端使用 PocketBase 超级管理员邮箱和密码登录。

## 构建验证

```bash
cd InterfaceCode/user-web
npm run typecheck
npm run test
npm run build

cd ../admin-web
npm run typecheck
npm run build
```

## 接口策略

用户端和管理端的 API 层均使用原生 `fetch` 访问 PocketBase：

- 用户端：`postsApi`、`commentsApi`、`guestbookApi`、`notesApi`、`projectsApi`、`siteConfigApi`。
- 管理端：`authApi`、`adminPostsApi`、`adminContentApi`、`uploadApi`、`moderationApi`、`settingsApi`。

默认 PocketBase 地址为：

```text
http://127.0.0.1:8090
```

也可以通过 `VITE_POCKETBASE_URL` 或浏览器本地保存的 `blog-api-base` 切换。

PocketBase 的集合迁移和运行说明位于仓库根目录 `pocketbase/`。管理端直接维护 `posts`、`notes`、`projects`、`comments`、`guestbook_messages`、`site_config` 和 `music_tracks` 集合，不需要再启动旧 C++ 博客服务。

## 用户端公开内容

- 用户端不再内置文章、随记、项目、评论或留言样例；加载、空数据和失败状态都会明确展示。
- 公开接口包括 `/api/config`、`/api/posts?q=&tag=&page=&limit=`、`/api/tags`、`/api/notes`、`/api/projects` 和 `/api/posts/{id}/navigation`。
- 收藏仅保存在当前浏览器的 `blog-favorite-post-ids`，不会同步到服务端。

## 当前注意事项

- 根目录旧 HTML 文件仅作历史参考，不参与两个 Vue 工程的构建和部署。
- AI 自动审核尚未迁移到 PocketBase，当前管理端只提供人工审核。
- 普通文章和八股文在管理端分开维护；八股文按 C++ 基础语法、C++ 进阶、Linux、计算机网络、Qt、音视频等子目录组织。子目录可在八股文管理中增删并用于筛选文章，新建或 Markdown 导入八股文时从现有目录中选择。文章封面在编辑抽屉中上传，Markdown 文件默认导入为未发布草稿。
- 用户进入文章详情页满 10 秒后上报一次有效阅读；PocketBase 按同一访客、同一文章、同一天去重。
