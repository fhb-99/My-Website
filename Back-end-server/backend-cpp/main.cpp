#include "third_party/httplib.h"
#include "third_party/json.hpp"
#include "SQLiteCpp/SQLiteCpp.h"
#include "repo/post_repo_sqlite.h"
#include "repo/database_schema.h"
#include "handlers/post_handler.h"
#include "handlers/public_content_handler.h"
#include "middleware/deepseek_moderation.h"

#include <ctime>
#include <iostream>
#include <memory>

using json = nlohmann::json;

static std::unique_ptr<SQLite::Database> g_db;
static std::unique_ptr<PostRepoSqlite> g_postRepo;
static std::unique_ptr<ModerationClient> g_moderationClient;

static bool InitDatabase()
{
    try {
        g_db.reset(new SQLite::Database(
            "data/blog.db",
            SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE));

        if (!InitializeDatabaseSchema(*g_db)) {
            return false;
        }

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

    g_moderationClient.reset(new ModerationClient());
    if (!g_moderationClient->IsConfigured()) {
        std::cerr << "[warn] DeepSeek moderation is not configured; AI moderation will stay pending." << std::endl;
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
        HandleListPublicPosts(*g_postRepo, req, res);
    });

    svr.Get("/api/config", [](const httplib::Request& req, httplib::Response& res) {
        HandleGetPublicConfig(*g_postRepo, req, res);
    });

    svr.Get("/api/notes", [](const httplib::Request& req, httplib::Response& res) {
        HandleGetNotes(*g_postRepo, req, res);
    });

    svr.Get("/api/projects", [](const httplib::Request& req, httplib::Response& res) {
        HandleGetProjects(*g_postRepo, req, res);
    });

    svr.Get("/api/tags", [](const httplib::Request& req, httplib::Response& res) {
        HandleGetTags(*g_postRepo, req, res);
    });

    svr.Get(R"(/api/posts/(\d+)/navigation)", [](const httplib::Request& req, httplib::Response& res) {
        HandleGetPostNavigation(*g_postRepo, req, res);
    });

    svr.Get(R"(/api/posts/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        HandleGetPostByID(*g_postRepo, req, res);
    });

    svr.Get(R"(/api/posts/slug/([A-Za-z0-9_-]+))", [](const httplib::Request& req, httplib::Response& res) {
        HandleGetPostBySlug(*g_postRepo, req, res);
    });

    // 用户在前端文章界面停留符合一定条件，通知后端增加阅读量
    svr.Post(R"(/api/posts/(\d+)/view)", [](const httplib::Request& req, httplib::Response& res){
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
        AdminTestModerationAI(*g_postRepo, *g_moderationClient, req, res);
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
        AdminModerateCommentWithAI(*g_postRepo, *g_moderationClient, req, res);
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

    // 评论
    svr.Get(R"(/api/posts/(\d+)/comments)", [](const httplib::Request& req, httplib::Response& res) {
        HandleGetPostComments(*g_postRepo, req, res);
    });

    svr.Post(R"(/api/posts/(\d+)/comments)", [](const httplib::Request& req, httplib::Response& res) {
        HandleCreatePostComment(*g_postRepo, *g_moderationClient, req, res);
    });

    // 搜索
    svr.Get("/api/search", [](const httplib::Request& req, httplib::Response& res) {
        HandleSearchPublicPosts(*g_postRepo, req, res);
    });

    // 留言
    svr.Get("/api/guestbook", [](const httplib::Request& req, httplib::Response& res) {
        HandleGetGuestbook(*g_postRepo, req, res);
    });

    svr.Post("/api/guestbook", [](const httplib::Request& req, httplib::Response& res) {
        HandleCreateGuestbook(*g_postRepo, *g_moderationClient, req, res);
    });
    svr.Post(R"(/api/admin/guestbook/(\d+)/moderate)", [](const httplib::Request& req, httplib::Response& res){
        if(!RequireAdmin(*g_postRepo, req, res)) {
            return;
        }
        AdminModerateGuestbookWithAI(*g_postRepo, *g_moderationClient, req, res);
    });


    std::cout << "Blog server running at http://0.0.0.0:8080" << std::endl;
    svr.listen("0.0.0.0", 8080);

    return 0;
}
