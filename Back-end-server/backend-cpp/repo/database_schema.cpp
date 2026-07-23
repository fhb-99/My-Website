#include "repo/database_schema.h"

bool InitializeDatabaseSchema(SQLite::Database& db)
{
    db.exec("PRAGMA journal_mode=WAL;");

    db.exec(R"(
        CREATE TABLE IF NOT EXISTS posts (
            id            INTEGER PRIMARY KEY AUTOINCREMENT,
            title         TEXT    NOT NULL,
            slug          TEXT    NOT NULL UNIQUE,
            summary       TEXT    DEFAULT '',
            content_md    TEXT    NOT NULL,
            content_html  TEXT    DEFAULT '',
            cover_url     TEXT    DEFAULT '',
            tags          TEXT    DEFAULT '[]',
            is_published  INTEGER DEFAULT 0,
            views         INTEGER DEFAULT 0,
            created_at    TEXT    DEFAULT (datetime('now','localtime')),
            updated_at    TEXT    DEFAULT (datetime('now','localtime'))
        );
    )");

    db.exec(R"(
        CREATE TABLE IF NOT EXISTS users (
            id                  INTEGER PRIMARY KEY AUTOINCREMENT,
            username            TEXT    NOT NULL UNIQUE,
            password_hash       TEXT    NOT NULL,
            password_salt       TEXT    NOT NULL,
            password_algo       TEXT    NOT NULL DEFAULT 'pbkdf2_sha256',
            password_iterations INTEGER NOT NULL DEFAULT 260000,
            role                TEXT    NOT NULL DEFAULT 'admin',
            display_name        TEXT    DEFAULT '',
            email               TEXT    DEFAULT '',
            is_active           INTEGER NOT NULL DEFAULT 1,
            created_at          TEXT    NOT NULL DEFAULT (datetime('now','localtime')),
            updated_at          TEXT    NOT NULL DEFAULT (datetime('now','localtime')),
            last_login_at       TEXT    DEFAULT ''
        );
    )");

    db.exec(R"(
        CREATE TABLE IF NOT EXISTS admin_sessions (
            token_hash TEXT PRIMARY KEY,
            user_id    INTEGER NOT NULL,
            expires_at TEXT    NOT NULL,
            created_at TEXT    NOT NULL DEFAULT (datetime('now','localtime')),
            revoked_at TEXT    DEFAULT '',
            user_agent TEXT    DEFAULT '',
            ip_hash    TEXT    DEFAULT '',
            FOREIGN KEY (user_id) REFERENCES users(id)
        );
    )");

    db.exec(R"(
        CREATE TABLE IF NOT EXISTS comments (
            id           INTEGER PRIMARY KEY AUTOINCREMENT,
            post_id      INTEGER NOT NULL,
            nickname     TEXT    NOT NULL,
            email        TEXT    NOT NULL,
            content      TEXT    NOT NULL,
            is_approved  INTEGER NOT NULL DEFAULT 1,
            created_at   TEXT    NOT NULL DEFAULT (datetime('now','localtime')),
            updated_at   TEXT    NOT NULL DEFAULT (datetime('now','localtime')),
            FOREIGN KEY (post_id) REFERENCES posts(id)
        );
    )");

    db.exec(R"(
        CREATE TABLE IF NOT EXISTS guestbook_messages (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            nickname TEXT NOT NULL,
            email TEXT NOT NULL,
            content TEXT NOT NULL,
            is_approved INTEGER NOT NULL DEFAULT 1,
            created_at TEXT NOT NULL DEFAULT (datetime('now','localtime')),
            updated_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
        );
    )");

    db.exec(R"(
        CREATE TABLE IF NOT EXISTS post_view_events (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            post_id INTEGER NOT NULL,
            visitor_id TEXT NOT NULL,
            viewed_date TEXT NOT NULL,
            created_at TEXT NOT NULL DEFAULT (datetime('now','localtime')),
            UNIQUE(post_id, visitor_id, viewed_date)
        );
    )");

    db.exec(R"(
        CREATE TABLE IF NOT EXISTS moderation_logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            target_type TEXT NOT NULL,
            target_id INTEGER NOT NULL,
            decision TEXT NOT NULL,
            source TEXT NOT NULL,
            reason TEXT NOT NULL,
            confidence REAL DEFAULT 0,
            created_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
        );
    )");

    db.exec(R"(
        CREATE TABLE IF NOT EXISTS site_settings (
            key TEXT PRIMARY KEY,
            value TEXT NOT NULL,
            created_at TEXT NOT NULL DEFAULT (datetime('now','localtime')),
            updated_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
        );
    )");

    db.exec(R"(
        CREATE TABLE IF NOT EXISTS notes (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            content TEXT NOT NULL,
            mood TEXT DEFAULT '',
            is_published INTEGER NOT NULL DEFAULT 0,
            created_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
        );
    )");

    db.exec(R"(
        CREATE TABLE IF NOT EXISTS projects (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            summary TEXT NOT NULL DEFAULT '',
            url TEXT NOT NULL DEFAULT '',
            tags TEXT NOT NULL DEFAULT '[]',
            sort_order INTEGER NOT NULL DEFAULT 0,
            is_published INTEGER NOT NULL DEFAULT 0,
            created_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
        );
    )");

    db.exec("CREATE INDEX IF NOT EXISTS idx_users_username ON users(username);");
    db.exec("CREATE INDEX IF NOT EXISTS idx_admin_sessions_user_id ON admin_sessions(user_id);");
    db.exec("CREATE INDEX IF NOT EXISTS idx_admin_sessions_expires_at ON admin_sessions(expires_at);");
    db.exec("CREATE INDEX IF NOT EXISTS idx_comments_post_id ON comments(post_id);");
    db.exec("CREATE INDEX IF NOT EXISTS idx_comments_approved ON comments(is_approved);");
    db.exec("CREATE INDEX IF NOT EXISTS idx_notes_public_created ON notes(is_published, created_at DESC, id DESC);");
    db.exec("CREATE INDEX IF NOT EXISTS idx_projects_public_order ON projects(is_published, sort_order ASC, created_at DESC, id DESC);");
    return true;
}
