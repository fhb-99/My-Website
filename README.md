## blog-platform 后端总览

> 根目录：`e:\coding\blog-platform\`  
> 前端静态站：`e:\coding\InterfaceCode\`（与本目录**同级**，不在 `blog-platform` 内）。

### 目录结构（当前）

```text
blog-platform/
├── backend-go/          # Go API 服务（业务、路由、DB）
│   ├── cmd/server/      # 入口 main.go
│   ├── internal/
│   │   ├── api/         # HTTP handler（Gin/Fiber 等）
│   │   ├── service/     # 业务逻辑
│   │   ├── repo/        # 数据访问（SQLite / Postgres）
│   │   ├── auth/        # 登录 / JWT / 权限
│   │   └── cppclient/   # 调用 C++ gRPC/HTTP 服务
│   └── proto/           # Go 侧使用的 proto（可与上级 proto/ 共享）
├── backend-cpp/         # C++ 高性能服务（搜索 / 推荐等）
│   ├── src/
│   └── include/
├── proto/               # 语言无关接口定义（gRPC / Protobuf）
├── deploy/              # docker-compose、nginx.conf 等部署文件
└── docs/                # 设计文档、笔记
```

### 前后端关系

- `InterfaceCode/`：纯静态前端（HTML/CSS/JS），由 Nginx 或静态托管平台直接服务。  
- `blog-platform/backend-go`：提供 `/api/*` 的 REST/gRPC 接口，给前端使用。  
- `blog-platform/backend-cpp`：可选的 C++ 算法/搜索服务，由 Go 通过 gRPC/HTTP 调用。

### 下一步建议

1. 在 `backend-go` 下使用 `go mod init` 初始化模块，并选择一个 Web 框架（如 Gin）。  
2. 先实现最小 API：`GET /api/posts` 和 `GET /api/posts/:id`，让 `posts.html` 能从后端取数据。  
3. 在 `proto/` 下补全 `search.proto`，为未来的 C++ 搜索服务预留接口。  
4. 在 `deploy/` 下补一个 `docker-compose.yml` 与 `nginx.conf`，方便一键启动前端 + 后端。

