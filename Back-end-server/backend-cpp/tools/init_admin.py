#!/usr/bin/env python3
# 初始化管理员账号和鉴权相关表。
#
# 示例：
#   python3 tools/init_admin.py
#   python3 tools/init_admin.py --db data/blog.db --username admin
#   python3 tools/init_admin.py --username admin --replace

import argparse
import getpass
import hashlib
import os
import secrets
import sqlite3
import sys


PBKDF2_ITERATIONS = 260000
SALT_BYTES = 16


def hash_password(password, salt=None):
    """使用标准库 PBKDF2 生成密码哈希，数据库中不保存明文密码。"""
    if salt is None:
        salt = secrets.token_bytes(SALT_BYTES)

    digest = hashlib.pbkdf2_hmac(
        "sha256",
        password.encode("utf-8"),
        salt,
        PBKDF2_ITERATIONS,
    )
    return salt.hex(), digest.hex()


def ensure_tables(conn):
    """创建可扩展的用户表和会话表；后续可继续增加字段而不影响现有数据。"""
    conn.execute(
        """
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT NOT NULL UNIQUE,
            password_hash TEXT NOT NULL,
            password_salt TEXT NOT NULL,
            password_algo TEXT NOT NULL DEFAULT 'pbkdf2_sha256',
            password_iterations INTEGER NOT NULL DEFAULT 260000,
            role TEXT NOT NULL DEFAULT 'admin',
            display_name TEXT DEFAULT '',
            email TEXT DEFAULT '',
            is_active INTEGER NOT NULL DEFAULT 1,
            created_at TEXT NOT NULL DEFAULT (datetime('now','localtime')),
            updated_at TEXT NOT NULL DEFAULT (datetime('now','localtime')),
            last_login_at TEXT DEFAULT ''
        );
        """
    )

    conn.execute(
        """
        CREATE TABLE IF NOT EXISTS admin_sessions (
            token_hash TEXT PRIMARY KEY,
            user_id INTEGER NOT NULL,
            expires_at TEXT NOT NULL,
            created_at TEXT NOT NULL DEFAULT (datetime('now','localtime')),
            revoked_at TEXT DEFAULT '',
            user_agent TEXT DEFAULT '',
            ip_hash TEXT DEFAULT '',
            FOREIGN KEY (user_id) REFERENCES users(id)
        );
        """
    )

    conn.execute("CREATE INDEX IF NOT EXISTS idx_users_username ON users(username);")
    conn.execute("CREATE INDEX IF NOT EXISTS idx_admin_sessions_user_id ON admin_sessions(user_id);")
    conn.execute("CREATE INDEX IF NOT EXISTS idx_admin_sessions_expires_at ON admin_sessions(expires_at);")


def prompt_password():
    password = getpass.getpass("管理员密码: ")
    confirm = getpass.getpass("再次输入密码: ")
    if password != confirm:
        raise ValueError("两次输入的密码不一致")
    if len(password) < 8:
        raise ValueError("密码至少需要 8 个字符")
    return password


def upsert_admin(conn, args):
    password = args.password or prompt_password()
    salt, password_hash = hash_password(password)

    existing = conn.execute(
        "SELECT id FROM users WHERE username = ?",
        (args.username,),
    ).fetchone()

    if existing and not args.replace:
        raise ValueError("管理员账号已存在；如需重置密码，请加 --replace")

    if existing:
        conn.execute(
            """
            UPDATE users
            SET password_hash = ?,
                password_salt = ?,
                password_algo = 'pbkdf2_sha256',
                password_iterations = ?,
                role = ?,
                display_name = ?,
                email = ?,
                is_active = 1,
                updated_at = datetime('now','localtime')
            WHERE username = ?
            """,
            (
                password_hash,
                salt,
                PBKDF2_ITERATIONS,
                args.role,
                args.display_name,
                args.email,
                args.username,
            ),
        )
        return "updated"

    conn.execute(
        """
        INSERT INTO users (
            username,
            password_hash,
            password_salt,
            password_algo,
            password_iterations,
            role,
            display_name,
            email,
            is_active
        ) VALUES (?, ?, ?, 'pbkdf2_sha256', ?, ?, ?, ?, 1)
        """,
        (
            args.username,
            password_hash,
            salt,
            PBKDF2_ITERATIONS,
            args.role,
            args.display_name,
            args.email,
        ),
    )
    return "created"


def parse_args():
    parser = argparse.ArgumentParser(description="初始化博客后台管理员账号")
    parser.add_argument("--db", default=os.path.join("data", "blog.db"), help="SQLite 数据库路径")
    parser.add_argument("--username", default="admin", help="管理员用户名")
    parser.add_argument("--password", default="", help="管理员密码；不传则安全地交互输入")
    parser.add_argument("--role", default="admin", help="用户角色，预留给后续权限扩展")
    parser.add_argument("--display-name", default="", help="展示名称")
    parser.add_argument("--email", default="", help="邮箱，选填")
    parser.add_argument("--replace", action="store_true", help="账号已存在时重置密码和资料")
    return parser.parse_args()


def main():
    args = parse_args()
    db_dir = os.path.dirname(os.path.abspath(args.db))
    if db_dir:
        os.makedirs(db_dir, exist_ok=True)

    try:
        conn = sqlite3.connect(args.db)
        ensure_tables(conn)
        action = upsert_admin(conn, args)
        conn.commit()
    except Exception as exc:
        print(f"[ERROR] {exc}", file=sys.stderr)
        return 1
    finally:
        try:
            conn.close()
        except Exception:
            pass

    print(f"[OK] 管理员账号已{ '创建' if action == 'created' else '更新' }")
    print(f"  DB:       {args.db}")
    print(f"  Username: {args.username}")
    print(f"  Role:     {args.role}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
