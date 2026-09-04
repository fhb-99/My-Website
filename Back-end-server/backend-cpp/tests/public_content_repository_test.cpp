#include "SQLiteCpp/SQLiteCpp.h"
#include "models/public_content.h"
#include "repo/database_schema.h"
#include "repo/post_repo_sqlite.h"

#include <iostream>
#include <string>

namespace {

bool Require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "[failed] " << message << std::endl;
    }
    return condition;
}

bool TableExists(SQLite::Database& db, const std::string& name)
{
    SQLite::Statement query(
        db,
        "SELECT 1 FROM sqlite_master WHERE type = 'table' AND name = ?");
    query.bind(1, name);
    return query.executeStep();
}

int InsertPost(SQLite::Database& db,
    const std::string& title,
    const std::string& slug,
    const std::string& tags,
    bool is_published,
    const std::string& created_at)
{
    SQLite::Statement statement(db,
        "INSERT INTO posts "
        "(title, slug, summary, content_md, content_html, tags, is_published, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");
    statement.bind(1, title);
    statement.bind(2, slug);
    statement.bind(3, title + " summary");
    statement.bind(4, title + " markdown");
    statement.bind(5, "<p>" + title + "</p>");
    statement.bind(6, tags);
    statement.bind(7, is_published ? 1 : 0);
    statement.bind(8, created_at);
    statement.bind(9, created_at);
    statement.exec();
    return static_cast<int>(db.getLastInsertRowid());
}

void InsertNote(SQLite::Database& db, const std::string& content, bool is_published)
{
    SQLite::Statement statement(db,
        "INSERT INTO notes (content, mood, is_published) VALUES (?, ?, ?)");
    statement.bind(1, content);
    statement.bind(2, "平静");
    statement.bind(3, is_published ? 1 : 0);
    statement.exec();
}

void InsertProject(SQLite::Database& db, const std::string& name, bool is_published)
{
    SQLite::Statement statement(db,
        "INSERT INTO projects (name, summary, url, tags, sort_order, is_published) "
        "VALUES (?, ?, ?, ?, ?, ?)");
    statement.bind(1, name);
    statement.bind(2, name + " summary");
    statement.bind(3, "https://example.com/" + name);
    statement.bind(4, "[\"Cpp\"]");
    statement.bind(5, is_published ? 1 : 2);
    statement.bind(6, is_published ? 1 : 0);
    statement.exec();
}

} // namespace

int main()
{
    SQLite::Database db(":memory:", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    const bool schema_ready = InitializeDatabaseSchema(db);

    bool ok = Require(schema_ready, "schema initializer succeeds");
    ok = Require(TableExists(db, "notes"), "notes table exists") && ok;
    ok = Require(TableExists(db, "projects"), "projects table exists") && ok;
    ok = Require(TableExists(db, "site_settings"), "site settings table exists") && ok;

    Note note;
    note.id = 1;
    note.content = "真实随记";
    note.mood = "平静";
    note.created_at = "2026-07-23 10:00:00";
    const nlohmann::json note_json = note.to_json_public();
    ok = Require(note_json["content"] == "真实随记", "note exposes public content") && ok;
    ok = Require(!note_json.contains("is_published"), "note excludes management fields") && ok;

    SiteConfig config;
    config.title = "我的站点";
    config.subtitle = "真实内容";
    config.announcement = "欢迎访问";
    const nlohmann::json config_json = config.to_json_public();
    ok = Require(config_json["announcement"] == "欢迎访问", "config exposes announcement") && ok;

    const int first_post_id = InsertPost(
        db, "C++ 基础", "cpp-basics", "[\"Cpp\"]", true, "2026-07-20 10:00:00");
    const int second_post_id = InsertPost(
        db, "C++ 进阶", "cpp-advanced", "[\"Cpp\"]", true, "2026-07-21 10:00:00");
    InsertPost(db, "Vue 实践", "vue-practice", "[\"Vue\"]", true, "2026-07-22 10:00:00");
    InsertPost(db, "C++ 草稿", "cpp-draft", "[\"Cpp\"]", false, "2026-07-23 10:00:00");
    InsertNote(db, "已发布随记", true);
    InsertNote(db, "草稿随记", false);
    InsertProject(db, "已发布项目", true);
    InsertProject(db, "草稿项目", false);
    db.exec("INSERT INTO site_settings (key, value) VALUES "
        "('site_title', '真实博客'), "
        "('site_subtitle', '记录实践'), "
        "('site_announcement', '欢迎访问')");

    PostRepoSqlite repo(db);
    PublicPostQuery keyword_query;
    keyword_query.keyword = "C++";
    keyword_query.page = 1;
    keyword_query.limit = 10;
    ok = Require(repo.ListPublishedPosts(keyword_query).size() == 2,
        "keyword filtering excludes drafts") && ok;

    PublicPostQuery tag_query;
    tag_query.tag = "Vue";
    tag_query.page = 1;
    tag_query.limit = 10;
    ok = Require(repo.CountPublishedPosts(tag_query) == 1,
        "tag filtering counts exact published tag") && ok;
    const std::vector<TagSummary> tags = repo.ListPublishedTags();
    ok = Require(tags.size() == 2 && tags[0].name == "Cpp" && tags[0].post_count == 2,
        "tag list aggregates only published posts") && ok;

    const PostNavigation navigation = repo.GetPublishedNavigation(second_post_id);
    ok = Require(navigation.post_found && navigation.has_previous && navigation.previous.id == first_post_id,
        "navigation returns the nearest older public post") && ok;
    ok = Require(navigation.has_next && navigation.next.slug == "vue-practice",
        "navigation returns the nearest newer public post") && ok;
    ok = Require(repo.ListPublishedNotes(1, 10).size() == 1,
        "notes hide drafts") && ok;
    ok = Require(repo.CountPublishedNotes() == 1,
        "note count hides drafts") && ok;
    ok = Require(repo.ListPublishedProjects(1, 10).size() == 1,
        "projects hide drafts") && ok;
    ok = Require(repo.CountPublishedProjects() == 1,
        "project count hides drafts") && ok;
    const SiteConfig saved_config = repo.GetPublicSiteConfig();
    ok = Require(saved_config.title == "真实博客" && saved_config.announcement == "欢迎访问",
        "public config reads only display settings") && ok;

    // 管理端需要看到草稿，创建、修改、删除不应影响公开列表的过滤规则。
    Note admin_note;
    admin_note.content = "后台草稿随记";
    admin_note.mood = "专注";
    admin_note.is_published = false;
    const int admin_note_id = repo.CreateNote(admin_note);
    ok = Require(repo.CountNotesForAdmin() == 3,
        "admin notes include drafts") && ok;
    admin_note.mood = "放松";
    ok = Require(repo.UpdateNote(admin_note_id, admin_note),
        "admin note update succeeds") && ok;
    ok = Require(repo.DeleteNote(admin_note_id) && repo.CountNotesForAdmin() == 2,
        "admin note delete succeeds") && ok;

    Project admin_project;
    admin_project.name = "后台草稿项目";
    admin_project.summary = "仅管理端可见";
    admin_project.url = "https://example.com/admin-project";
    admin_project.tags.push_back("Cpp");
    admin_project.sort_order = 3;
    admin_project.is_published = false;
    const int admin_project_id = repo.CreateProject(admin_project);
    ok = Require(repo.CountProjectsForAdmin() == 3,
        "admin projects include drafts") && ok;
    admin_project.sort_order = 4;
    ok = Require(repo.UpdateProject(admin_project_id, admin_project),
        "admin project update succeeds") && ok;
    ok = Require(repo.DeleteProject(admin_project_id) && repo.CountProjectsForAdmin() == 2,
        "admin project delete succeeds") && ok;

    // 管理端写入后仍由公开配置读取接口返回，避免两端使用不同数据源。
    repo.SaveSiteSetting("site_title", "更新后的博客");
    ok = Require(repo.GetPublicSiteConfig().title == "更新后的博客",
        "public config reads admin setting updates") && ok;

    MusicConfig music_config;
    music_config.enabled = true;
    music_config.volume = 0.45;
    MusicTrack public_track;
    public_track.id = 1;
    public_track.title = "公开曲目";
    public_track.audio_url = "https://example.com/public.mp3";
    public_track.sort_order = 2;
    music_config.tracks.push_back(public_track);
    MusicTrack disabled_track;
    disabled_track.id = 2;
    disabled_track.title = "后台曲目";
    disabled_track.audio_url = "https://example.com/admin.mp3";
    disabled_track.sort_order = 1;
    disabled_track.is_enabled = false;
    music_config.tracks.push_back(disabled_track);
    repo.SaveMusicConfig(music_config);
    ok = Require(repo.GetAdminMusicConfig().tracks.size() == 2,
        "admin music config includes disabled tracks") && ok;
    const MusicConfig public_music = repo.GetPublicMusicConfig();
    ok = Require(public_music.tracks.size() == 1 && public_music.tracks[0].title == "公开曲目",
        "public music config hides disabled tracks") && ok;
    ok = Require(!public_music.to_json_public()["tracks"][0].contains("is_enabled"),
        "public music response excludes management fields") && ok;
    return ok ? 0 : 1;
}
