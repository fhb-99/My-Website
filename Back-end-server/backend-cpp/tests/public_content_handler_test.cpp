#include "SQLiteCpp/SQLiteCpp.h"
#include "handlers/public_content_handler.h"
#include "repo/database_schema.h"
#include "repo/post_repo_sqlite.h"
#include "third_party/json.hpp"

#include <iostream>
#include <regex>
#include <string>

using json = nlohmann::json;

namespace {

bool Require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "[failed] " << message << std::endl;
    }
    return condition;
}

int InsertPost(SQLite::Database& db,
    const std::string& title,
    const std::string& slug,
    const std::string& tags,
    const std::string& created_at)
{
    SQLite::Statement statement(db,
        "INSERT INTO posts "
        "(title, slug, summary, content_md, content_html, tags, is_published, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, 1, ?, ?)");
    statement.bind(1, title);
    statement.bind(2, slug);
    statement.bind(3, title + " summary");
    statement.bind(4, title + " markdown");
    statement.bind(5, "<p>" + title + "</p>");
    statement.bind(6, tags);
    statement.bind(7, created_at);
    statement.bind(8, created_at);
    statement.exec();
    return static_cast<int>(db.getLastInsertRowid());
}

} // namespace

int main()
{
    SQLite::Database db(":memory:", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    bool ok = Require(InitializeDatabaseSchema(db), "schema initializer succeeds");
    const int first_post_id = InsertPost(
        db, "C++ 基础", "cpp-basics", "[\"Cpp\"]", "2026-07-20 10:00:00");
    const int second_post_id = InsertPost(
        db, "Vue 实践", "vue-practice", "[\"Vue\"]", "2026-07-21 10:00:00");
    db.exec("INSERT INTO posts (title, slug, summary, content_md, is_published) "
        "VALUES ('C++ 草稿', 'cpp-draft', '', '', 0)");
    db.exec("INSERT INTO notes (content, mood, is_published) VALUES "
        "('公开随记', '平静', 1), ('草稿随记', '平静', 0)");
    db.exec("INSERT INTO projects (name, summary, url, tags, sort_order, is_published) VALUES "
        "('公开项目', '说明', 'https://example.com', '[\"Cpp\"]', 1, 1), "
        "('草稿项目', '说明', '', '[]', 2, 0)");
    db.exec("INSERT INTO site_settings (key, value) VALUES "
        "('site_title', '真实博客'), "
        "('site_subtitle', '真实说明'), "
        "('site_announcement', '欢迎访问'), "
        "('moderation_config', '{\"api_key\":\"private\"}')");

    PostRepoSqlite repo(db);

    httplib::Request posts_request;
    posts_request.params.emplace("page", "1");
    posts_request.params.emplace("limit", "10");
    posts_request.params.emplace("q", "C++");
    posts_request.params.emplace("tag", "Cpp");
    httplib::Response posts_response;
    HandleListPublicPosts(repo, posts_request, posts_response);
    const json posts_body = json::parse(posts_response.body);
    ok = Require(posts_response.status == 200, "filtered post request succeeds") && ok;
    ok = Require(posts_body["total"] == 1 && posts_body["data"].size() == 1,
        "filtered post response excludes drafts") && ok;

    httplib::Request empty_request;
    httplib::Response config_response;
    HandleGetPublicConfig(repo, empty_request, config_response);
    const json config_body = json::parse(config_response.body);
    ok = Require(config_body["title"] == "真实博客", "config returns display settings") && ok;
    ok = Require(!config_body.contains("moderation_config"), "config hides moderation data") && ok;

    httplib::Response notes_response;
    HandleGetNotes(repo, empty_request, notes_response);
    ok = Require(json::parse(notes_response.body)["total"] == 1, "notes hide drafts") && ok;

    httplib::Response projects_response;
    HandleGetProjects(repo, empty_request, projects_response);
    ok = Require(json::parse(projects_response.body)["total"] == 1, "projects hide drafts") && ok;

    httplib::Response tags_response;
    HandleGetTags(repo, empty_request, tags_response);
    ok = Require(json::parse(tags_response.body)["data"].size() == 2, "tags list published tags") && ok;

    httplib::Request navigation_request;
    navigation_request.path = "/api/posts/2/navigation";
    std::regex_match(navigation_request.path, navigation_request.matches,
        std::regex(R"(/api/posts/(\d+)/navigation)"));
    httplib::Response navigation_response;
    HandleGetPostNavigation(repo, navigation_request, navigation_response);
    const json navigation_body = json::parse(navigation_response.body);
    ok = Require(navigation_body["previous"]["id"] == first_post_id,
        "navigation returns previous public post") && ok;
    ok = Require(navigation_body["next"] == nullptr,
        "navigation returns null when no newer public post") && ok;
    ok = Require(second_post_id > first_post_id, "fixtures have stable ordering") && ok;
    return ok ? 0 : 1;
}
