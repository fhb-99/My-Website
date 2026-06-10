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

    std::cout << "Blog server running at http://0.0.0.0:8080" << std::endl;
    svr.listen("0.0.0.0", 8080);

    return 0;
}
