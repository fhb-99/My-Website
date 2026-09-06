#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 || ! -f $1 ]]; then
    echo "用法: $0 /var/backups/gentleyun-blog/pb_data-时间.tar.gz" >&2
    exit 2
fi

exec 9>/run/lock/gentleyun-blog-maintenance.lock
flock -x 9

archive=$(readlink -f "$1")
app_dir=/opt/gentleyun-blog
stamp=$(date +%Y%m%d-%H%M%S)
rollback_dir="$app_dir/pb_data.before-restore-$stamp"
temp_dir=$(mktemp -d /opt/gentleyun-blog-restore.XXXXXX)
trap 'rm -rf "$temp_dir"' EXIT

# 只接受由备份脚本生成的 pb_data 目录，避免错误归档覆盖应用其他文件。
tar -tzf "$archive" | grep -E '^pb_data/data\.db$' >/dev/null || {
    echo "备份中没有有效的 pb_data 目录" >&2
    exit 1
}
tar -xzf "$archive" -C "$temp_dir"
test -f "$temp_dir/pb_data/data.db"

rollback_restore() {
    status=$?
    trap - ERR
    systemctl stop gentleyun-pocketbase.service 2>/dev/null || true
    if [[ -d "$rollback_dir" ]]; then
        if [[ -d "$app_dir/pb_data" ]]; then
            mv "$app_dir/pb_data" "$app_dir/pb_data.failed-restore-$stamp"
        fi
        mv "$rollback_dir" "$app_dir/pb_data"
        chown -R pocketbase:pocketbase "$app_dir/pb_data"
        systemctl start gentleyun-pocketbase.service 2>/dev/null || true
    fi
    exit "$status"
}

systemctl stop gentleyun-pocketbase.service
mv "$app_dir/pb_data" "$rollback_dir"
trap rollback_restore ERR
mv "$temp_dir/pb_data" "$app_dir/pb_data"
chown -R pocketbase:pocketbase "$app_dir/pb_data"
find "$app_dir/pb_data" -type d -exec chmod 0750 {} +
find "$app_dir/pb_data" -type f -exec chmod 0640 {} +
systemctl start gentleyun-pocketbase.service
for _ in $(seq 1 20); do
    if curl --fail --silent --show-error --max-time 10 http://127.0.0.1:8080/api/health >/dev/null; then
        break
    fi
    sleep 1
done
curl --fail --silent --show-error --max-time 10 http://127.0.0.1:8080/api/health >/dev/null
trap - ERR

echo "恢复完成；恢复前数据保留在 $rollback_dir"
