#!/usr/bin/env bash
set -euo pipefail

exec 9>/run/lock/gentleyun-blog-maintenance.lock

# 备份或恢复期间服务会短暂停止，此时跳过检查，避免产生误报警。
if ! flock -n 9; then
    exit 0
fi

curl --fail --silent --show-error --max-time 10 http://127.0.0.1:8080/api/health >/dev/null
