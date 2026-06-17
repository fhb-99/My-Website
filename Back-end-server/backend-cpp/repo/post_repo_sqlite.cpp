#include "repo/post_repo_sqlite.h"
#include "third_party/json.hpp"

#include <algorithm>
#include <stdexcept>

using json = nlohmann::json;

namespace {

std::vector<std::string> ParseTags(const std::string& value)
{
    std::vector<std::string> tags;

    if (value.empty()) {
        return tags;
    }

    try {
        json parsed = json::parse(value);
        if (!parsed.is_array()) {
            return tags;
        }

        for (const auto& item : parsed) {
            if (item.is_string()) {
                tags.push_back(item.get<std::string>());
            }
        }
    } catch (const std::exception&) {
        // Invalid tag JSON should not break public post reads.
    }

    return tags;
}

std::string DumpTags(const std::vector<std::string>& tags)
{
    return json(tags).dump();
}

Post ReadPost(SQLite::Statement& query)
{
    Post post;
    post.id = query.getColumn(0).getInt();
    post.title = query.getColumn(1).getString();
    post.slug = query.getColumn(2).getString();
    post.summary = query.getColumn(3).getString();
    post.content_md = query.getColumn(4).getString();
    post.content_html = query.getColumn(5).getString();
    post.cover_url = query.getColumn(6).getString();
    post.tags = ParseTags(query.getColumn(7).getString());
    post.is_published = query.getColumn(8).getInt() != 0;
    post.views = query.getColumn(9).getInt();
    post.created_at = query.getColumn(10).getString();
    post.updated_at = query.getColumn(11).getString();
    return post;
}

const char* kPostColumns =
    "id, title, slug, summary, content_md, content_html, cover_url, "
    "tags, is_published, views, created_at, updated_at";

} // namespace

std::vector<Post> PostRepoSqlite::GetAll(int page, int limit)
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    page = std::max(page, 1);
    limit = std::max(limit, 1);
    const int offset = (page - 1) * limit;

    SQLite::Statement query(*m_db,
        std::string("SELECT ") + kPostColumns +
        " FROM posts"
        " WHERE is_published = 1"
        " ORDER BY created_at DESC, id DESC"
        " LIMIT ? OFFSET ?");

    query.bind(1, limit);
    query.bind(2, offset);

    std::vector<Post> posts;
    while (query.executeStep()) {
        posts.push_back(ReadPost(query));
    }
    return posts;
}

Post PostRepoSqlite::GetByID(int id, bool& ok)
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    SQLite::Statement query(*m_db,
        std::string("SELECT ") + kPostColumns +
        " FROM posts"
        " WHERE id = ? AND is_published = 1"
        " LIMIT 1");

    query.bind(1, id);

    if (!query.executeStep()) {
        ok = false;
        return Post{};
    }

    ok = true;
    return ReadPost(query);
}

int PostRepoSqlite::create(const Post& post)
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    SQLite::Statement query(*m_db,
        "INSERT INTO posts "
        "(title, slug, summary, content_md, content_html, cover_url, tags, is_published) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?)");

    query.bind(1, post.title);
    query.bind(2, post.slug);
    query.bind(3, post.summary);
    query.bind(4, post.content_md);
    query.bind(5, post.content_html);
    query.bind(6, post.cover_url);
    query.bind(7, DumpTags(post.tags));
    query.bind(8, post.is_published ? 1 : 0);
    query.exec();

    return static_cast<int>(m_db->getLastInsertRowid());
}

bool PostRepoSqlite::update(int id, const Post& post)
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    SQLite::Statement query(*m_db,
        "UPDATE posts SET "
        "title = ?, slug = ?, summary = ?, content_md = ?, content_html = ?, "
        "cover_url = ?, tags = ?, is_published = ?, updated_at = datetime('now','localtime') "
        "WHERE id = ?");

    query.bind(1, post.title);
    query.bind(2, post.slug);
    query.bind(3, post.summary);
    query.bind(4, post.content_md);
    query.bind(5, post.content_html);
    query.bind(6, post.cover_url);
    query.bind(7, DumpTags(post.tags));
    query.bind(8, post.is_published ? 1 : 0);
    query.bind(9, id);

    return query.exec() > 0;
}

bool PostRepoSqlite::remove(int id)
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    SQLite::Statement query(*m_db, "DELETE FROM posts WHERE id = ?");
    query.bind(1, id);
    return query.exec() > 0;
}

void PostRepoSqlite::incrementViews(int id)
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    SQLite::Statement query(*m_db, "UPDATE posts SET views = views + 1 WHERE id = ?");
    query.bind(1, id);
    query.exec();
}

std::vector<Post> PostRepoSqlite::search(const std::string& keyword, int limit)
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    limit = std::max(limit, 1);
    const std::string pattern = "%" + keyword + "%";

    SQLite::Statement query(*m_db,
        std::string("SELECT ") + kPostColumns +
        " FROM posts"
        " WHERE is_published = 1 AND (title LIKE ? OR summary LIKE ? OR content_md LIKE ?)"
        " ORDER BY created_at DESC, id DESC"
        " LIMIT ?");

    query.bind(1, pattern);
    query.bind(2, pattern);
    query.bind(3, pattern);
    query.bind(4, limit);

    std::vector<Post> posts;
    while (query.executeStep()) {
        posts.push_back(ReadPost(query));
    }
    return posts;
}
User PostRepoSqlite::GetUserByUsername(const std::string& name, bool& ok)
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    ok = false;

    try
    {
        SQLite::Statement query(*m_db, "SELECT username, password_hash AS password FROM users WHERE username = ?");
        query.bind(1, name);

        User user;

        if(!query.executeStep()) {
            return user; 
        }

        ok = true;
        user.username = query.getColumn("username").getString();
        user.password = query.getColumn("password").getString();

        return user;
    }
    catch(const SQLite::Exception& e)
    {
        throw std::runtime_error(std::string("Query user failed: ") + e.what());
    }
}
