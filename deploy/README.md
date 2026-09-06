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
