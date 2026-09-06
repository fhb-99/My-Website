#!/usr/bin/env bash
set -Eeuo pipefail

umask 027

readonly incoming_root=/var/lib/github-deploy/blog/incoming
readonly state_root=/var/lib/github-deploy/blog
readonly live_html=/var/www/gentleyun/html
readonly web_root=/var/www/gentleyun
readonly backup_root=/var/backups/gentleyun-blog
readonly lock_file=/run/lock/gentleyun-blog-maintenance.lock

release_id=
component=
dry_run=0
work_dir=
frontend_stage=
old_holder=
swapped=0

usage() {
    echo "用法: $0 --release-id <40位小写Git SHA> --component frontend [--dry-run]" >&2
}

fail() {
    echo "部署失败: $*" >&2
    exit 1
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --release-id)
            [[ $# -ge 2 ]] || fail "--release-id 缺少参数"
            release_id=$2
            shift 2
            ;;
        --component)
            [[ $# -ge 2 ]] || fail "--component 缺少参数"
            component=$2
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
[[ $component == frontend ]] || fail "首版发布器只允许 component=frontend"

readonly archive_name="gentleyun-blog-${release_id}.tar.gz"
readonly incoming_dir="${incoming_root}/${release_id}"
readonly incoming_archive="${incoming_dir}/${archive_name}"
readonly incoming_checksums="${incoming_dir}/sha256sums.txt"

[[ -d $incoming_dir && ! -L $incoming_dir ]] || fail "上传目录不存在或不是普通目录"
[[ -f $incoming_archive && ! -L $incoming_archive ]] || fail "发布包不存在或不是普通文件"
[[ -f $incoming_checksums && ! -L $incoming_checksums ]] || fail "校验文件不存在或不是普通文件"

exec 9>"$lock_file"
flock -x 9

rollback() {
    local status=$1

    trap - EXIT HUP INT TERM
    set +e
    if (( swapped )); then
        echo "部署失败，正在恢复旧前端" >&2
        if [[ -d $live_html && -n $frontend_stage && -d $frontend_stage ]]; then
            mv -- "$live_html" "$frontend_stage/failed-html"
        fi
        if [[ -n $old_holder && -d $old_holder/html ]]; then
            mv -- "$old_holder/html" "$live_html"
        fi
        curl --fail --silent --show-error --max-time 10 https://gentleyun.cn/ >/dev/null || true
    fi

    [[ -z $work_dir || ! -e $work_dir ]] || rm -rf -- "$work_dir"
    [[ -z $frontend_stage || ! -e $frontend_stage ]] || rm -rf -- "$frontend_stage"
    [[ -z $old_holder || ! -e $old_holder ]] || rm -rf -- "$old_holder"
    exit "$status"
}

trap 'rollback $?' EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

# 先复制到 root 专属目录，避免校验后上传账户再修改源文件。
work_dir=$(mktemp -d "${state_root}/.deploy-${release_id}.XXXXXX")
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

# 拒绝绝对路径、父目录、链接、设备文件、重复路径和异常大的归档。
python3 - "$work_dir/$archive_name" "$work_dir/extracted" <<'PY'
import os
import shutil
import sys
import tarfile
from pathlib import Path, PurePosixPath

archive = Path(sys.argv[1])
destination = Path(sys.argv[2]).resolve()
allowed_roots = {"www", "pocketbase", "metadata.txt"}
required_files = {"www/index.html", "www/admin/index.html", "metadata.txt"}
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
            (destination.joinpath(*parts)).mkdir(parents=True, exist_ok=True)
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

missing = required_files - regular_files
if missing:
    raise SystemExit("归档缺少必要文件: " + ", ".join(sorted(missing)))
PY

metadata_file="$work_dir/extracted/metadata.txt"
mapfile -t metadata_lines <"$metadata_file"
[[ ${#metadata_lines[@]} -eq 2 || ${#metadata_lines[@]} -eq 3 ]] \
    || fail "metadata.txt 必须包含两行或三行"
[[ ${metadata_lines[0]} == "COMMIT_SHA=${release_id}" ]] || fail "metadata Commit SHA 不匹配"
[[ ${metadata_lines[1]} =~ ^SOURCE_RUN_ID=[0-9]+$ ]] || fail "metadata Run ID 格式错误"
if [[ ${#metadata_lines[@]} -eq 3 ]]; then
    [[ ${metadata_lines[2]} =~ ^BACKEND_CHANGED=[01]$ ]] || fail "metadata 后端变化标记错误"
fi

[[ -s $work_dir/extracted/www/index.html ]] || fail "用户端 index.html 为空"
[[ -s $work_dir/extracted/www/admin/index.html ]] || fail "管理端 index.html 为空"
[[ -d $live_html && ! -L $live_html ]] || fail "生产前端目录不存在或是符号链接"

mem_available_mib=$(( $(awk '/MemAvailable/ { print $2 }' /proc/meminfo) / 1024 ))
swap_used_mib=$(( $(awk '
    /SwapTotal/ { total=$2 }
    /SwapFree/ { free=$2 }
    END { print total-free }
' /proc/meminfo) / 1024 ))
disk_free_mib=$(df -Pm -- "$web_root" | awk 'NR == 2 { print $4 }')

(( mem_available_mib >= 350 )) || fail "可用内存不足 350 MiB"
(( swap_used_mib <= 256 )) || fail "Swap 使用量超过 256 MiB"
(( disk_free_mib >= 5120 )) || fail "可用磁盘不足 5 GiB"

frontend_stage=$(mktemp -d "${web_root}/.next-${release_id}.XXXXXX")
install -d -m 0755 "$frontend_stage/html"
cp -a -- "$work_dir/extracted/www/." "$frontend_stage/html/"
chown -R root:root "$frontend_stage/html"
find "$frontend_stage/html" -type d -exec chmod 0755 {} +
find "$frontend_stage/html" -type f -exec chmod 0644 {} +

if (( dry_run )); then
    echo "dry_run=ok"
    echo "release_id=$release_id"
    echo "component=$component"
    echo "mem_available_mib=$mem_available_mib"
    echo "swap_used_mib=$swap_used_mib"
    echo "disk_free_mib=$disk_free_mib"
    exit 0
fi

stamp=$(date -u +%Y%m%d-%H%M%S)
backup_dir="${backup_root}/frontend-${stamp}-${release_id:0:12}"
[[ ! -e $backup_dir ]] || fail "备份目录已存在，请稍后重试"
install -d -m 0700 "$backup_dir"
cp -a -- "$live_html" "$backup_dir/html"

# 新旧目录位于同一文件系统，缩短 Nginx 看不到 html 的时间窗口。
old_holder=$(mktemp -d "${web_root}/.previous-${release_id}.XXXXXX")
mv -- "$live_html" "$old_holder/html"
swapped=1
mv -- "$frontend_stage/html" "$live_html"

nginx -t
curl --fail --silent --show-error --max-time 10 http://127.0.0.1:8080/api/health >/dev/null
curl --fail --silent --show-error --max-time 15 https://gentleyun.cn/ >/dev/null
curl --fail --silent --show-error --max-time 15 https://gentleyun.cn/admin/ >/dev/null

current_release_tmp=$(mktemp "${state_root}/.current-release.XXXXXX")
printf 'COMMIT_SHA=%s\nSOURCE_RUN_ID=%s\nDEPLOYED_AT=%s\nCOMPONENT=%s\n' \
    "$release_id" "${metadata_lines[1]#SOURCE_RUN_ID=}" "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$component" \
    >"$current_release_tmp"
chmod 0644 "$current_release_tmp"
mv -- "$current_release_tmp" "$state_root/current-release"

swapped=0
rm -rf -- "$old_holder"
old_holder=

logger -t gentleyun-blog-deploy "release=${release_id} component=${component} backup=${backup_dir}"
rm -rf -- "$incoming_dir"
echo "release=ok"
echo "release_id=$release_id"
echo "component=$component"
echo "backup=$backup_dir"
