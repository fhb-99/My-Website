# Gentleyun 单机部署

生产环境由 Nginx 提供用户端和 `/admin/` 静态文件，并将同源 `/api/` 转发到只监听 `127.0.0.1:8080` 的 PocketBase。云服务器的 `8090` 已被 SyncRTC RealtimeServer 使用，博客不能复用该端口。

## 目录

- `/opt/gentleyun-blog/`：PocketBase 可执行文件、迁移、Hooks 和 `pb_data`。
- `/var/www/gentleyun/html/`：用户端 `dist`，管理端放在其中的 `admin/`。
- `/var/backups/gentleyun-blog/`：每日数据备份，保留 14 天。
- `/etc/nginx/sites-available/gentleyun`：站点反向代理和写请求限流。

## 运行与验证

```bash
systemctl status gentleyun-pocketbase.service
systemctl list-timers 'gentleyun-blog-*'
curl --fail http://127.0.0.1:8080/api/health
curl --fail https://gentleyun.cn/api/health
nginx -t
```

日志由 journald 统一轮转，不再单独写无限增长的应用日志：

```bash
journalctl -u gentleyun-pocketbase.service --since today
journalctl -u gentleyun-blog-health.service --since today
```

恢复数据前先确认备份文件，再执行：

```bash
sudo /usr/local/sbin/restore-gentleyun-blog /var/backups/gentleyun-blog/pb_data-时间.tar.gz
```

恢复脚本会保留恢复前的 `pb_data.before-restore-*`，确认无误后再人工清理。

## 日常前端 CD

日常发布使用 `/usr/local/sbin/deploy-gentleyun-blog`，不要再次运行一次性迁移脚本
`install-gentleyun-release.sh`。GitHub 的 `Blog CI` 在 `main` 构建发布包；成功后
`Blog CD` 通过受限账户上传，并调用 root 所有的固定部署器。

首版 CD 只接受前端发布，不更新 PocketBase 二进制、Hooks、Migrations 或 `pb_data`：

```bash
sudo /usr/local/sbin/deploy-gentleyun-blog \
    --release-id 40位小写Git提交SHA \
    --component frontend \
    --dry-run
```

部署器会校验发布包和元数据，检查内存、Swap 与磁盘，备份当前前端后再切换目录。
本地/API/公网检查任一失败时，会自动恢复切换前的前端。成功发布记录保存在：

```text
/var/lib/github-deploy/blog/current-release
```
