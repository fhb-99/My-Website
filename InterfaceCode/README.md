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

## 构建验证

```bash
cd InterfaceCode/user-web
npm run typecheck
npm run build

cd ../admin-web
npm run typecheck
npm run build
```

## 接口策略

用户端和管理端的 API 层已经从占位方法升级为原生 `fetch` 通信层：

- 用户端：`postsApi`、`commentsApi`、`guestbookApi`、`notesApi`、`projectsApi`、`siteConfigApi`。
- 管理端：`authApi`、`adminPostsApi`、`uploadApi`、`moderationApi`、`settingsApi`。

默认后端地址为：

```text
http://127.0.0.1:8080
```

也可以通过 `VITE_API_BASE_URL` 或浏览器本地保存的 `blog-api-base` 切换。

## 当前注意事项

- 用户端文章列表、文章详情、评论会优先请求后端；失败时保留占位内容，避免页面白屏。
- 用户端留言、项目、碎碎念、站点配置的后端路由还未补齐，目前仍可能回退到占位内容。
- 管理端 API 层已能请求后端，但部分管理页面仍是 UI 占位，后续需要逐步接入真实接口。
- 管理端文章更新、删除、审核、站点配置等目标接口已在前端 API 层预留，但 C++ 后端还需要补对应路由。
