#!/usr/bin/env bash
set -euo pipefail

backup_dir=/var/backups/gentleyun-blog
app_dir=/opt/gentleyun-blog
service=gentleyun-pocketbase.service
stamp=$(date +%Y%m%d-%H%M%S)
archive="$backup_dir/pb_data-$stamp.tar.gz"

exec 9>/run/lock/gentleyun-blog-maintenance.lock
flock -x 9

install -d -m 0700 "$backup_dir"

# PocketBase 使用 SQLite；短暂停止服务后打包，避免复制到一半时数据库仍在写入。
systemctl stop "$service"
trap 'systemctl start "$service"' EXIT
tar -C "$app_dir" -czf "$archive.tmp" pb_data
mv "$archive.tmp" "$archive"
systemctl start "$service"
trap - EXIT

# systemctl 返回时进程可能刚启动，等待监听端口就绪后再判定备份是否成功。
for _ in $(seq 1 20); do
    if curl --fail --silent --show-error --max-time 10 http://127.0.0.1:8080/api/health >/dev/null; then
        break
    fi
    sleep 1
done
curl --fail --silent --show-error --max-time 10 http://127.0.0.1:8080/api/health >/dev/null
find "$backup_dir" -maxdepth 1 -type f -name 'pb_data-*.tar.gz' -mtime +14 -delete
