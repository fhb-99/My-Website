#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 || ! -d $1/payload ]]; then
    echo "用法: $0 /opt/gentleyun-staging/发布目录" >&2
    exit 2
fi

stage=$(readlink -f "$1")
payload="$stage/payload"
stamp=$(date +%Y%m%d-%H%M%S)
backup_root="/var/backups/gentleyun-blog-pre-pocketbase/$stamp"
old_frontend=/var/www/gentleyun/html
nginx_site=/etc/nginx/sites-available/gentleyun
old_service=/etc/systemd/system/blog-server.service

test -x "$payload/pocketbase"
test -f "$payload/pb_data/data.db"
test -f "$payload/www/index.html"
test -f "$payload/www/admin/index.html"

install -d -m 0700 "$backup_root"
cp -a "$nginx_site" "$backup_root/nginx-gentleyun.conf"
cp -a "$old_service" "$backup_root/blog-server.service"
if [[ -f /etc/gentleyun-blog.env ]]; then
    cp -a /etc/gentleyun-blog.env "$backup_root/gentleyun-blog.env"
fi

rollback() {
    status=$?
    trap - ERR
    echo "发布失败，开始恢复旧博客，退出码：$status" >&2
    systemctl disable --now gentleyun-blog-health.timer gentleyun-blog-backup.timer 2>/dev/null || true
    systemctl stop gentleyun-pocketbase.service 2>/dev/null || true
    cp -a "$backup_root/nginx-gentleyun.conf" "$nginx_site" 2>/dev/null || true
    cp -a "$backup_root/blog-server.service" "$old_service" 2>/dev/null || true
    if [[ -f "$backup_root/gentleyun-blog.env" ]]; then
        cp -a "$backup_root/gentleyun-blog.env" /etc/gentleyun-blog.env
    fi
    if [[ -d "$backup_root/old-blog-platform" && ! -e /opt/blog-platform ]]; then
        mv "$backup_root/old-blog-platform" /opt/blog-platform
    fi
    if [[ -d "$backup_root/frontend" ]]; then
        rm -rf "$old_frontend"
        mv "$backup_root/frontend" "$old_frontend"
    fi
    systemctl daemon-reload 2>/dev/null || true
    nginx -t >/dev/null 2>&1 && systemctl reload nginx || true
    systemctl start blog-server.service 2>/dev/null || true
    exit "$status"
}
trap rollback ERR

# 先停止旧 C++ 服务，确保 SQLite 数据处于一致状态，再保留完整回滚副本。
systemctl stop blog-server.service
mv /opt/blog-platform "$backup_root/old-blog-platform"
mv "$old_frontend" "$backup_root/frontend"
if [[ -d /opt/gentleyun-blog ]]; then
    mv /opt/gentleyun-blog "$backup_root/previous-gentleyun-blog"
fi

if ! id pocketbase >/dev/null 2>&1; then
    useradd --system --home-dir /opt/gentleyun-blog --shell /usr/sbin/nologin pocketbase
fi

install -d -m 0755 /opt/gentleyun-blog "$old_frontend"
install -m 0755 "$payload/pocketbase" /opt/gentleyun-blog/pocketbase
cp -a "$payload/pb_data" "$payload/pb_hooks" "$payload/pb_migrations" /opt/gentleyun-blog/
cp -a "$payload/www/." "$old_frontend/"
chown -R root:root /opt/gentleyun-blog "$old_frontend"
chown -R pocketbase:pocketbase /opt/gentleyun-blog/pb_data
find /opt/gentleyun-blog/pb_data -type d -exec chmod 0750 {} +
find /opt/gentleyun-blog/pb_data -type f -exec chmod 0640 {} +
find /opt/gentleyun-blog/pb_hooks /opt/gentleyun-blog/pb_migrations -type d -exec chmod 0755 {} +
find /opt/gentleyun-blog/pb_hooks /opt/gentleyun-blog/pb_migrations -type f -exec chmod 0644 {} +
find "$old_frontend" -type d -exec chmod 0755 {} +
find "$old_frontend" -type f -exec chmod 0644 {} +

install -m 0755 "$payload/deploy/backup-gentleyun-blog.sh" /usr/local/sbin/backup-gentleyun-blog
install -m 0755 "$payload/deploy/restore-gentleyun-blog.sh" /usr/local/sbin/restore-gentleyun-blog
install -m 0755 "$payload/deploy/health-check-gentleyun-blog.sh" /usr/local/sbin/health-check-gentleyun-blog
install -m 0644 "$payload/deploy/gentleyun-pocketbase.service" /etc/systemd/system/gentleyun-pocketbase.service
install -m 0644 "$payload/deploy/gentleyun-blog-health.service" /etc/systemd/system/gentleyun-blog-health.service
install -m 0644 "$payload/deploy/gentleyun-blog-health.timer" /etc/systemd/system/gentleyun-blog-health.timer
install -m 0644 "$payload/deploy/gentleyun-blog-backup.service" /etc/systemd/system/gentleyun-blog-backup.service
install -m 0644 "$payload/deploy/gentleyun-blog-backup.timer" /etc/systemd/system/gentleyun-blog-backup.timer
install -m 0644 "$payload/deploy/nginx-gentleyun.conf" "$nginx_site"

systemd-analyze verify /etc/systemd/system/gentleyun-pocketbase.service \
    /etc/systemd/system/gentleyun-blog-health.service \
    /etc/systemd/system/gentleyun-blog-health.timer \
    /etc/systemd/system/gentleyun-blog-backup.service \
    /etc/systemd/system/gentleyun-blog-backup.timer
nginx -t
systemctl daemon-reload
systemctl enable --now gentleyun-pocketbase.service

for _ in $(seq 1 20); do
    if curl --fail --silent --show-error http://127.0.0.1:8080/api/health >/dev/null; then
        break
    fi
    sleep 1
done
curl --fail --silent --show-error http://127.0.0.1:8080/api/health >/dev/null

systemctl reload nginx
curl --fail --silent --show-error https://gentleyun.cn/api/health >/dev/null
curl --fail --silent --show-error https://gentleyun.cn/ >/dev/null
curl --fail --silent --show-error https://gentleyun.cn/admin/ >/dev/null

systemctl enable --now gentleyun-blog-health.timer gentleyun-blog-backup.timer
systemctl start gentleyun-blog-health.service
systemctl start gentleyun-blog-backup.service
curl --fail --silent --show-error http://127.0.0.1:8080/api/health >/dev/null

# 新服务、静态站点和首份备份均通过后，移除旧服务的活动配置。
systemctl disable blog-server.service 2>/dev/null || true
rm -f "$old_service" /etc/gentleyun-blog.env
systemctl daemon-reload
ufw --force delete allow 8080/tcp >/dev/null 2>&1 || true

trap - ERR
echo "release=ok"
echo "backup=$backup_root"
