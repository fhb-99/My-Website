#include "SQLiteCpp/SQLiteCpp.h"
#include "models/public_content.h"
#include "repo/database_schema.h"

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
    return ok ? 0 : 1;
}
