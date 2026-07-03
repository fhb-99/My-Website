#include "third_party/httplib.h"
#include "third_party/json.hpp"
#include "SQLiteCpp/SQLiteCpp.h"
#include "repo/post_repo_sqlite.h"
#include "handlers/post_handler.h"

#include <ctime>
#include <iostream>
#include <memory>

using json = nlohmann::json;

static std::unique_ptr<SQLite::Database> g_db;
static std::unique_ptr<PostRepoSqlite> g_postRepo;

static bool InitDatabase()
{
    try {
        // Keep the database setup in main; all post queries stay inside the repo.
        g_db.reset(new SQLite::Database(
            "data/blog.db",
            SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE));

        g_db->exec("PRAGMA journal_mode=WAL;");

        // 文章表
        g_db->exec(R"(
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

        // 管理员表
        g_db->exec(R"(
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

        // 管理员会话表
        g_db->exec(R"(
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

        // 评论表
        g_db->exec(R"(
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

        // 留言表
        g_db->exec(R"(
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

        // 文章阅读量表
        g_db->exec(R"(
            CREATE TABLE IF NOT EXISTS post_view_events (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                post_id INTEGER NOT NULL,
                visitor_id TEXT NOT NULL,
                viewed_date TEXT NOT NULL,
                created_at TEXT NOT NULL DEFAULT (datetime('now','localtime')),
                UNIQUE(post_id, visitor_id, viewed_date)
            );
        )");

        // 审核日志表
        g_db->exec(R"(
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

        // 审核配置表
        g_db->exec(R"(
            CREATE TABLE IF NOT EXISTS site_settings (
                key TEXT PRIMARY KEY,
                value TEXT NOT NULL,
                created_at TEXT NOT NULL DEFAULT (datetime('now','localtime')),
                updated_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
            );
        )");

        g_db->exec("CREATE INDEX IF NOT EXISTS idx_users_username ON users(username);");
        g_db->exec("CREATE INDEX IF NOT EXISTS idx_admin_sessions_user_id ON admin_sessions(user_id);");
        g_db->exec("CREATE INDEX IF NOT EXISTS idx_admin_sessions_expires_at ON admin_sessions(expires_at);");
        g_db->exec("CREATE INDEX IF NOT EXISTS idx_comments_post_id ON comments(post_id);");
        g_db->exec("CREATE INDEX IF NOT EXISTS idx_comments_approved ON comments(is_approved);");
        g_db->exec("CREATE INDEX IF NOT EXISTS idx_moderation_logs_created_at ON moderation_logs(created_at);");


        g_postRepo.reset(new PostRepoSqlite(*g_db));
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[fatal] database initialization failed: " << e.what() << std::endl;
        return false;
    }
}

int main()
{
    if (!InitDatabase()) {
        return 1;
    }

    httplib::Server svr;

    // Uploaded files are stored on disk and served through public /uploads URLs.
    svr.set_mount_point("/uploads", "uploads");

    // Development CORS support. In production, prefer same-origin Nginx proxying.
    svr.set_pre_routing_handler([](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");

        if (req.method == "OPTIONS") {
            res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
            res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
            res.status = 204;
            return httplib::Server::HandlerResponse::Handled;
        }

        return httplib::Server::HandlerResponse::Unhandled;
    });

    svr.Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
        json body;
        body["status"] = "ok";
        body["time"] = std::time(nullptr);
        res.set_content(body.dump(), "application/json; charset=utf-8");
    });

    svr.Get("/api/posts", [](const httplib::Request& req, httplib::Response& res) {
        HandleGetAllPosts(*g_postRepo, req, res);
    });

    svr.Get(R"(/api/posts/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        HandleGetPostByID(*g_postRepo, req, res);
    });

    svr.Get(R"(/api/posts/slug/([A-Za-z0-9_-]+))", [](const httplib::Request& req, httplib::Response& res) {
        HandleGetPostBySlug(*g_postRepo, req, res);
    });

    // 用户在前端文章界面停留符合一定条件，通知后端增加阅读量
    svr.Post(R"(/api/posts/(\d+)/view)", [](const httplib::Request& req, httplib::Response res){
        HandleRecordPostView(*g_postRepo, req, res);
    });

    svr.Post("/api/auth/login", [](const httplib::Request& req, httplib::Response& res){
        HandleLogin(*g_postRepo, req, res);
    });

    svr.Post("/api/admin/posts", [](const httplib::Request& req, httplib::Response& res){
        if(!RequireAdmin(*g_postRepo, req, res)) {
            return;
        }
        HandlerCreatePost(*g_postRepo, req, res);
    });

    svr.Get("/api/admin/posts", [](const httplib::Request& req, httplib::Response& res){
        if(!RequireAdmin(*g_postRepo, req, res)) {
            return;
        }
        AdminGetAllPosts(*g_postRepo, req, res);
    });

    svr.Get(R"(/api/admin/posts/(\d+))", [](const httplib::Request& req, httplib::Response& res){
        if(!RequireAdmin(*g_postRepo, req, res)) {
            return;
        }
        AdminGetPostByID(*g_postRepo, req, res);
    });

    svr.Put(R"(/api/admin/posts/(\d+))", [](const httplib::Request& req, httplib::Response& res){
        if(!RequireAdmin(*g_postRepo, req, res)) {
            return;
        }
        AdminUpdatePost(*g_postRepo, req, res);
    });

    svr.Delete(R"(/api/admin/posts/(\d+))", [](const httplib::Request& req, httplib::Response& res){
        if(!RequireAdmin(*g_postRepo, req, res)) {
            return;
        }
        AdminDeletePost(*g_postRepo, req, res);
    });


    // 上传文章图片
    svr.Post("/api/admin/uploads/images", [](const httplib::Request& req, httplib::Response& res){
        if(!RequireAdmin(*g_postRepo, req, res)) {
            return;
        }
        AdminPostImages(*g_postRepo, req, res);
    });

    // 上传md文档
    svr.Post("/api/admin/uploads/markdown", [](const httplib::Request& req, httplib::Response& res){
        if(!RequireAdmin(*g_postRepo, req, res)) {
            return;
        }
        AdminPostMarkdown(*g_postRepo, req, res);
    });

    // 评论
    svr.Get(R"(/api/posts/(\d+)/comments)", [](const httplib::Request& req, httplib::Response& res) {
        HandleGetPostComments(*g_postRepo, req, res);
    });

    svr.Post(R"(/api/posts/(\d+)/comments)", [](const httplib::Request& req, httplib::Response& res) {
        HandleCreatePostComment(*g_postRepo, req, res);
    });

    // 搜索
    svr.Get("/api/search", [](const httplib::Request& req, httplib::Response& res) {
        HandleSearchPosts(*g_postRepo, req, res);
    });

    // 留言
    svr.Get("/api/guestbook", [](const httplib::Request& req, httplib::Response& res) {
        HandleGetGuestbook(*g_postRepo, req, res);
    });

    svr.Post("/api/guestbook", [](const httplib::Request& req, httplib::Response& res) {
        HandleCreateGuestbook(*g_postRepo, req, res);
    });

    // 审核评论以及留言
    // 管理端审核配置：后续用于控制自动审核开关、屏蔽词和审核策略。
    svr.Get("/api/admin/moderation/config", [](const httplib::Request& req, httplib::Response& res){
        if(!RequireAdmin(*g_postRepo, req, res)) {
            return;
        }
        AdminGetModerationConfig(*g_postRepo, req, res);
    });

    svr.Put("/api/admin/moderation/config", [](const httplib::Request& req, httplib::Response& res){
        if(!RequireAdmin(*g_postRepo, req, res)) {
            return;
        }
        AdminUpdateModerationConfig(*g_postRepo, req, res);
    });

    svr.Get("/api/admin/moderation/logs", [](const httplib::Request& req, httplib::Response& res){
        if(!RequireAdmin(*g_postRepo, req, res)) {
            return;
        }
        AdminGetModerationLogs(*g_postRepo, req, res);
    });

    svr.Post("/api/admin/moderation/test", [](const httplib::Request& req, httplib::Response& res){
        if(!RequireAdmin(*g_postRepo, req, res)) {
            return;
        }
        AdminTestModerationAI(*g_postRepo, req, res);
    });

    // 管理端评论审核：当前只搭接口框架，具体查询、通过、拒绝和删除逻辑后续补齐。
    svr.Get("/api/admin/comments", [](const httplib::Request& req, httplib::Response& res){
        if(!RequireAdmin(*g_postRepo, req, res)) {
            return;
        }
        AdminGetComments(*g_postRepo, req, res);
    });

    svr.Put(R"(/api/admin/comments/(\d+)/approve)", [](const httplib::Request& req, httplib::Response& res){
        if(!RequireAdmin(*g_postRepo, req, res)) {
            return;
        }
        AdminApproveComment(*g_postRepo, req, res);
    });

    svr.Put(R"(/api/admin/comments/(\d+)/reject)", [](const httplib::Request& req, httplib::Response& res){
        if(!RequireAdmin(*g_postRepo, req, res)) {
            return;
        }
        AdminRejectComment(*g_postRepo, req, res);
    });

    svr.Delete(R"(/api/admin/comments/(\d+))", [](const httplib::Request& req, httplib::Response& res){
        if(!RequireAdmin(*g_postRepo, req, res)) {
            return;
        }
        AdminDeleteComment(*g_postRepo, req, res);
    });

    svr.Post(R"(/api/admin/comments/(\d+)/moderate)", [](const httplib::Request& req, httplib::Response& res){
        if(!RequireAdmin(*g_postRepo, req, res)) {
            return;
        }
        AdminModerateCommentWithAI(*g_postRepo, req, res);
    });

    // 管理端留言审核：留言板和文章评论分开管理，避免后续数据含义混在一起。
    svr.Get("/api/admin/guestbook", [](const httplib::Request& req, httplib::Response& res){
        if(!RequireAdmin(*g_postRepo, req, res)) {
            return;
        }
        AdminGetGuestbook(*g_postRepo, req, res);
    });

    svr.Put(R"(/api/admin/guestbook/(\d+)/approve)", [](const httplib::Request& req, httplib::Response& res){
        if(!RequireAdmin(*g_postRepo, req, res)) {
            return;
        }
        AdminApproveGuestbook(*g_postRepo, req, res);
    });

    svr.Put(R"(/api/admin/guestbook/(\d+)/reject)", [](const httplib::Request& req, httplib::Response& res){
        if(!RequireAdmin(*g_postRepo, req, res)) {
            return;
        }
        AdminRejectGuestbook(*g_postRepo, req, res);
    });

    svr.Delete(R"(/api/admin/guestbook/(\d+))", [](const httplib::Request& req, httplib::Response& res){
        if(!RequireAdmin(*g_postRepo, req, res)) {
            return;
        }
        AdminDeleteGuestbook(*g_postRepo, req, res);
    });

    svr.Post(R"(/api/admin/guestbook/(\d+)/moderate)", [](const httplib::Request& req, httplib::Response& res){
        if(!RequireAdmin(*g_postRepo, req, res)) {
            return;
        }
        AdminModerateGuestbookWithAI(*g_postRepo, req, res);
    });


    std::cout << "Blog server running at http://0.0.0.0:8080" << std::endl;
    svr.listen("0.0.0.0", 8080);

    return 0;
}
