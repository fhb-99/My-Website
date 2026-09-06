#!/usr/bin/env bash
set -Eeuo pipefail

umask 027

readonly incoming_root=/var/lib/github-deploy/blog/incoming
readonly state_root=/var/lib/github-deploy/blog
readonly app_root=/opt/gentleyun-blog
readonly live_data=/opt/gentleyun-blog/pb_data
readonly live_hooks=/opt/gentleyun-blog/pb_hooks
readonly live_migrations=/opt/gentleyun-blog/pb_migrations
readonly pocketbase_binary=/opt/gentleyun-blog/pocketbase
readonly backup_root=/var/backups/gentleyun-blog
readonly lock_file=/run/lock/gentleyun-blog-maintenance.lock
readonly service=gentleyun-pocketbase.service

release_id=
dry_run=0
work_dir=
backend_stage=
old_holder=
backup_dir=
service_stopped=0
backend_swapped=0
database_may_have_changed=0

usage() {
    echo "用法: $0 --release-id <40位小写Git SHA> [--dry-run]" >&2
}

fail() {
    echo "后端部署失败: $*" >&2
    exit 1
}

wait_for_health() {
    for _ in $(seq 1 30); do
        if curl --fail --silent --show-error --max-time 5 \
            http://127.0.0.1:8080/api/health >/dev/null; then
            return 0
        fi
        sleep 1
    done
    return 1
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --release-id)
            [[ $# -ge 2 ]] || fail "--release-id 缺少参数"
            release_id=$2
            shift 2
            ;;
        --dry-run)
            dry_run=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            usage
            fail "不支持的参数: $1"
            ;;
    esac
done

[[ $EUID -eq 0 ]] || fail "必须通过 sudo 以 root 身份运行"
[[ $release_id =~ ^[0-9a-f]{40}$ ]] || fail "release-id 格式错误"

readonly archive_name="gentleyun-blog-${release_id}.tar.gz"
readonly incoming_dir="${incoming_root}/${release_id}"
readonly incoming_archive="${incoming_dir}/${archive_name}"
readonly incoming_checksums="${incoming_dir}/sha256sums.txt"

[[ -d $incoming_dir && ! -L $incoming_dir ]] || fail "上传目录不存在或不是普通目录"
[[ -f $incoming_archive && ! -L $incoming_archive ]] || fail "发布包不存在或不是普通文件"
[[ -f $incoming_checksums && ! -L $incoming_checksums ]] || fail "校验文件不存在或不是普通文件"
[[ -x $pocketbase_binary && ! -L $pocketbase_binary ]] || fail "PocketBase 二进制不存在或不可执行"
[[ -d $live_data && ! -L $live_data ]] || fail "生产 pb_data 不存在或是符号链接"
[[ -d $live_hooks && ! -L $live_hooks ]] || fail "生产 Hooks 目录不存在或是符号链接"
[[ -d $live_migrations && ! -L $live_migrations ]] || fail "生产 Migrations 目录不存在或是符号链接"

exec 9>"$lock_file"
flock -x 9

rollback() {
    local status=$1

    trap - EXIT HUP INT TERM
    set +e

    if (( backend_swapped )); then
        echo "后端部署失败，正在恢复 Hooks、Migrations 和数据库" >&2
        systemctl stop "$service" 2>/dev/null || true

        if [[ -d $live_hooks ]]; then
            mv -- "$live_hooks" "$backup_dir/pb_hooks.failed" 2>/dev/null || true
        fi
        if [[ -d $live_migrations ]]; then
            mv -- "$live_migrations" "$backup_dir/pb_migrations.failed" 2>/dev/null || true
        fi
        if [[ -d $old_holder/pb_hooks ]]; then
            mv -- "$old_holder/pb_hooks" "$live_hooks"
        fi
        if [[ -d $old_holder/pb_migrations ]]; then
            mv -- "$old_holder/pb_migrations" "$live_migrations"
        fi

        if (( database_may_have_changed )) && [[ -f $backup_dir/pb_data.tar.gz ]]; then
            if [[ -d $live_data ]]; then
                mv -- "$live_data" "$backup_dir/pb_data.failed"
            fi
            tar -C "$app_root" -xzf "$backup_dir/pb_data.tar.gz"
            chown -R pocketbase:pocketbase "$live_data"
            find "$live_data" -type d -exec chmod 0750 {} +
            find "$live_data" -type f -exec chmod 0640 {} +
        fi

        systemctl start "$service" 2>/dev/null || true
        wait_for_health || true
    elif (( service_stopped )); then
        systemctl start "$service" 2>/dev/null || true
        wait_for_health || true
    fi

    [[ -z $work_dir || ! -e $work_dir ]] || rm -rf -- "$work_dir"
    [[ -z $backend_stage || ! -e $backend_stage ]] || rm -rf -- "$backend_stage"
    [[ -z $old_holder || ! -e $old_holder ]] || rm -rf -- "$old_holder"
    exit "$status"
}

trap 'rollback $?' EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

# 先复制到 root 专属目录，避免校验后上传账户再修改源文件。
work_dir=$(mktemp -d "${state_root}/.backend-deploy-${release_id}.XXXXXX")
install -m 0600 "$incoming_archive" "$work_dir/$archive_name"
install -m 0600 "$incoming_checksums" "$work_dir/sha256sums.txt"

mapfile -t checksum_lines <"$work_dir/sha256sums.txt"
[[ ${#checksum_lines[@]} -eq 1 ]] || fail "校验文件必须且只能包含一条记录"
[[ ${checksum_lines[0]} =~ ^[0-9a-f]{64}[[:space:]][[:space:]]gentleyun-blog-${release_id}\.tar\.gz$ ]] \
    || fail "校验文件内容或文件名不匹配"

(
    cd "$work_dir"
    sha256sum --check --strict sha256sums.txt
)

install -d -m 0700 "$work_dir/extracted"

# 只解压白名单路径，并拒绝链接、设备文件和目录穿越。
python3 - "$work_dir/$archive_name" "$work_dir/extracted" <<'PY'
import os
import shutil
import sys
import tarfile
from pathlib import Path, PurePosixPath

archive = Path(sys.argv[1])
destination = Path(sys.argv[2]).resolve()
allowed_roots = {"www", "pocketbase", "metadata.txt"}
seen = set()
regular_files = set()
total_size = 0

with tarfile.open(archive, "r:gz") as bundle:
    members = bundle.getmembers()
    if len(members) > 10_000:
        raise SystemExit("归档文件数量超过限制")

    for member in members:
        path = PurePosixPath(member.name)
        parts = tuple(part for part in path.parts if part not in ("", "."))
        if not parts:
            continue
        if path.is_absolute() or ".." in parts or "\\" in member.name:
            raise SystemExit(f"归档包含不安全路径: {member.name}")
        if parts[0] not in allowed_roots:
            raise SystemExit(f"归档包含白名单外路径: {member.name}")

        relative = "/".join(parts)
        if relative in seen:
            raise SystemExit(f"归档包含重复路径: {relative}")
        seen.add(relative)

        if member.isdir():
            destination.joinpath(*parts).mkdir(parents=True, exist_ok=True)
            continue
        if not member.isfile():
            raise SystemExit(f"归档包含非普通文件: {member.name}")

        total_size += member.size
        if total_size > 512 * 1024 * 1024:
            raise SystemExit("归档解压后大小超过 512 MiB")

        source = bundle.extractfile(member)
        if source is None:
            raise SystemExit(f"无法读取归档文件: {member.name}")
        target = destination.joinpath(*parts)
        target.parent.mkdir(parents=True, exist_ok=True)
        with source, target.open("xb") as output:
            shutil.copyfileobj(source, output)
        os.chmod(target, 0o600)
        regular_files.add(relative)

if "metadata.txt" not in regular_files:
    raise SystemExit("归档缺少 metadata.txt")
PY

metadata_file="$work_dir/extracted/metadata.txt"
mapfile -t metadata_lines <"$metadata_file"
[[ ${#metadata_lines[@]} -eq 3 ]] || fail "metadata.txt 必须且只能包含三行"
[[ ${metadata_lines[0]} == "COMMIT_SHA=${release_id}" ]] || fail "metadata Commit SHA 不匹配"
[[ ${metadata_lines[1]} =~ ^SOURCE_RUN_ID=[0-9]+$ ]] || fail "metadata Run ID 格式错误"
[[ ${metadata_lines[2]} == "BACKEND_CHANGED=1" ]] || fail "发布包未声明后端发生变化"

new_hooks="$work_dir/extracted/pocketbase/pb_hooks"
new_migrations="$work_dir/extracted/pocketbase/pb_migrations"
[[ -d $new_hooks && ! -L $new_hooks ]] || fail "发布包缺少 Hooks 目录"
[[ -d $new_migrations && ! -L $new_migrations ]] || fail "发布包缺少 Migrations 目录"

# 已经在生产执行过的迁移不可修改或删除，只允许追加新迁移。
while IFS= read -r -d '' live_migration; do
    relative=${live_migration#"$live_migrations/"}
    candidate="$new_migrations/$relative"
    [[ -f $candidate && ! -L $candidate ]] || fail "已有迁移被删除: $relative"
    diff --strip-trailing-cr --brief "$live_migration" "$candidate" >/dev/null \
        || fail "已有迁移被修改: $relative"
done < <(find "$live_migrations" -type f -print0)

mem_available_mib=$(( $(awk '/MemAvailable/ { print $2 }' /proc/meminfo) / 1024 ))
swap_used_mib=$(( $(awk '
    /SwapTotal/ { total=$2 }
    /SwapFree/ { free=$2 }
    END { print total-free }
' /proc/meminfo) / 1024 ))
disk_free_mib=$(df -Pm -- "$app_root" | awk 'NR == 2 { print $4 }')

(( mem_available_mib >= 350 )) || fail "可用内存不足 350 MiB"
(( swap_used_mib <= 256 )) || fail "Swap 使用量超过 256 MiB"
(( disk_free_mib >= 5120 )) || fail "可用磁盘不足 5 GiB"

backend_stage=$(mktemp -d "${app_root}/.next-backend-${release_id}.XXXXXX")
cp -a -- "$new_hooks" "$backend_stage/pb_hooks"
cp -a -- "$new_migrations" "$backend_stage/pb_migrations"
chown -R root:root "$backend_stage"
find "$backend_stage" -type d -exec chmod 0755 {} +
find "$backend_stage" -type f -exec chmod 0644 {} +

if (( dry_run )); then
    echo "dry_run=ok"
    echo "release_id=$release_id"
    echo "component=backend"
    echo "migration_preflight=deferred_until_consistent_backup"
    echo "mem_available_mib=$mem_available_mib"
    echo "swap_used_mib=$swap_used_mib"
    echo "disk_free_mib=$disk_free_mib"
    exit 0
fi

systemctl is-active --quiet "$service" || fail "PocketBase 服务当前未运行"

stamp=$(date -u +%Y%m%d-%H%M%S)
backup_dir="${backup_root}/backend-${stamp}-${release_id:0:12}"
[[ ! -e $backup_dir ]] || fail "备份目录已存在，请稍后重试"
install -d -m 0700 "$backup_dir"
cp -a -- "$live_hooks" "$backup_dir/pb_hooks"
cp -a -- "$live_migrations" "$backup_dir/pb_migrations"
install -m 0600 "$metadata_file" "$backup_dir/metadata.txt"

# 短暂停止 PocketBase，保证 SQLite 数据备份和迁移预检基于同一份数据。
systemctl stop "$service"
service_stopped=1
tar -C "$app_root" -czf "$backup_dir/pb_data.tar.gz.tmp" pb_data
mv -- "$backup_dir/pb_data.tar.gz.tmp" "$backup_dir/pb_data.tar.gz"

install -d -m 0700 "$work_dir/preflight"
cp -a -- "$live_data" "$work_dir/preflight/pb_data"
if ! "$pocketbase_binary" migrate up \
    --dir="$work_dir/preflight/pb_data" \
    --hooksDir="$backend_stage/pb_hooks" \
    --migrationsDir="$backend_stage/pb_migrations" \
    >"$backup_dir/migration-preflight.log" 2>&1; then
    fail "数据库副本迁移预检失败，详情见 $backup_dir/migration-preflight.log"
fi

old_holder=$(mktemp -d "${app_root}/.previous-backend-${release_id}.XXXXXX")
backend_swapped=1
mv -- "$live_hooks" "$old_holder/pb_hooks"
mv -- "$backend_stage/pb_hooks" "$live_hooks"
mv -- "$live_migrations" "$old_holder/pb_migrations"
mv -- "$backend_stage/pb_migrations" "$live_migrations"

# 服务启动时会在生产 pb_data 上执行已经预检通过的新迁移。
database_may_have_changed=1
systemctl start "$service"
service_stopped=0
wait_for_health || fail "PocketBase 启动后健康检查超时"
systemctl is-active --quiet "$service" || fail "PocketBase 服务未保持运行"
curl --fail --silent --show-error --max-time 15 https://gentleyun.cn/api/health >/dev/null \
    || fail "公网 API 健康检查失败"

current_release_tmp=$(mktemp "${state_root}/.current-backend-release.XXXXXX")
printf 'COMMIT_SHA=%s\nSOURCE_RUN_ID=%s\nDEPLOYED_AT=%s\nCOMPONENT=backend\n' \
    "$release_id" "${metadata_lines[1]#SOURCE_RUN_ID=}" "$(date -u +%Y-%m-%dT%H:%M:%SZ)" \
    >"$current_release_tmp"
chmod 0644 "$current_release_tmp"
mv -- "$current_release_tmp" "$state_root/current-backend-release"

backend_swapped=0
database_may_have_changed=0
rm -rf -- "$old_holder"
old_holder=

logger -t gentleyun-blog-deploy \
    "release=${release_id} component=backend backup=${backup_dir}"
echo "release=ok"
echo "release_id=$release_id"
echo "component=backend"
echo "backup=$backup_dir"
