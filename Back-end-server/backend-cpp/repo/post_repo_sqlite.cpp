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

Comment ReadComment(SQLite::Statement& query)
{
    Comment comment;
    comment.id = query.getColumn(0).getInt();
    comment.post_id = query.getColumn(1).getInt();
    comment.nickname = query.getColumn(2).getString();
    comment.email = query.getColumn(3).getString();
    comment.content = query.getColumn(4).getString();
    comment.is_approved = query.getColumn(5).getInt() != 0;
    comment.created_at = query.getColumn(6).getString();
    comment.updated_at = query.getColumn(7).getString();
    return comment;
}

const char* kPostColumns =
    "id, title, slug, summary, content_md, content_html, cover_url, "
    "tags, is_published, views, created_at, updated_at";

const char* kCommentColumns =
    "id, post_id, nickname, email, content, is_approved, created_at, updated_at";

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

// 获取已发布文章总数，供 handler 计算 total_pages 和 has_more
int PostRepoSqlite::GetPublishedCount()
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    SQLite::Statement query(*m_db, 
        "SELECT COUNT(*) FROM posts WHERE is_published = 1");
    query.executeStep();
    return query.getColumn(0).getInt();
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


User PostRepoSqlite::GetUserByUsername(const std::string& name, bool& flag)
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    try
    {
        SQLite::Statement query(*m_db, 
            "SELECT id, username, password_hash, password_salt, password_algo, "
            "password_iterations, role, is_active "
            "FROM users WHERE username = ? LIMIT 1");
        query.bind(1, name);

        User user;

        if(!query.executeStep()) {
            return user; 
        }

        flag = true;
        user.id = query.getColumn("id").getInt();
        user.username = query.getColumn("username").getString();
        user.password_hash = query.getColumn("password_hash").getString();
        user.password_salt = query.getColumn("password_salt").getString();
        user.password_algo = query.getColumn("password_algo").getString();
        user.password_iterations = query.getColumn("password_iterations").getInt();
        user.role = query.getColumn("role").getString();
        user.is_active = query.getColumn("is_active").getInt() != 0;

        return user;
    }
    catch(const SQLite::Exception& e)
    {
        throw std::runtime_error(std::string("Query user failed: ") + e.what());
    }
}


std::string PostRepoSqlite::CreateAdminSession(int user_id, const std::string& token_hash, 
    int ttl_hours, const std::string& user_agent)
{
    if(!m_db) throw std::runtime_error("database is not initialized");

    if(ttl_hours < 1) ttl_hours = 1;

    try{
        //登录时，先清理过期会话
        m_db->exec("DELETE FROM admin_sessions WHERE expires_at <= datetime('now', 'localtime')");

        const std::string ttl_modifier = "+" + std::to_string(ttl_hours) + " hours";
        SQLite::Statement insert(*m_db, 
            "INSERT INTO admin_sessions (token_hash, user_id, expires_at, user_agent) "
            "VALUES (?, ?, datetime('now', 'localtime', ?), ?)");
        insert.bind(1, token_hash);
        insert.bind(2, user_id);
        insert.bind(3, ttl_modifier);
        insert.bind(4, user_agent);
        insert.exec();

        SQLite::Statement update_user(*m_db, 
            "UPDATE users SET last_login_at = datetime('now', 'localtime'), "
            "updated_at = datetime('now', 'localtime') WHERE id = ?");
        update_user.bind(1, user_id);
        update_user.exec();

        SQLite::Statement query(*m_db,
            "SELECT expires_at FROM admin_sessions WHERE token_hash = ? LIMIT 1");
        query.bind(1, token_hash);

        if (query.executeStep()) {
            return query.getColumn("expires_at").getString();
        }

        return "";
    }
    catch(const SQLite::Exception& e) {
        throw std::runtime_error(std::string("Create admin session failed: ") + e.what());
    }
}



bool PostRepoSqlite::IsAdminSessionValid(const std::string& token_hash)
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    try {
        SQLite::Statement query(*m_db,
            "SELECT 1 FROM admin_sessions "
            "WHERE token_hash = ? "
            "AND expires_at > datetime('now','localtime') "
            "AND (revoked_at IS NULL OR revoked_at = '') "
            "LIMIT 1");
        query.bind(1, token_hash);
        return query.executeStep();
    } catch (const SQLite::Exception& e) {
        throw std::runtime_error(std::string("Validate admin session failed: ") + e.what());
    }
}


bool PostRepoSqlite::IsSlugExists(const std::string& slug)
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    try{
        SQLite::Statement query(*m_db, 
            "SELECT 1 FROM posts WHERE slug = ?");
        query.bind(1, slug);
        return query.executeStep();
    }
    catch (const SQLite::Exception& e) {
        throw std::runtime_error(std::string("Validate admin session failed: ") + e.what());
    }
}

Post PostRepoSqlite::GetBySlug(const std::string& slug, bool& ok)
{
    if(!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    try {
        SQLite::Statement query(*m_db, 
            std::string("SELECT ") + kPostColumns + 
            " FROM posts"
            " WHERE slug = ? AND is_published = 1"
            " LIMIT 1"
        );
        query.bind(1, slug);

        if(!query.executeStep()) {
            ok = false;
            return Post{};
        }

        ok = true;
        return ReadPost(query);
    }
    catch(const SQLite::Exception& e) {
        throw std::runtime_error(std::string("select by slug failed: ") + e.what());
    }
}




// 获取某篇文章已发布文章下已通过审核的评论
std::vector<Comment> PostRepoSqlite::GetCommentsByPostID(int post_id, int page, int limit)
{
    if(!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    page = std::max(page, 1);
    limit = std::max(limit, 1);
    const int offset = (page - 1) * limit;

    SQLite::Statement query(*m_db,
        std::string("SELECT ") + kCommentColumns +
        " FROM comments"
        " WHERE post_id = ? AND is_approved = 1"
        " ORDER BY created_at DESC, id DESC"
        " LIMIT ? OFFSET ?");

    query.bind(1, post_id);
    query.bind(2, limit);
    query.bind(3, offset);

    std::vector<Comment> comments;
    while (query.executeStep()) {
        comments.push_back(ReadComment(query));
    }
    return comments;
}

// 获取评论总数,用于前端分页展示
int PostRepoSqlite::GetApprovedCommentCount(int post_id)
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    SQLite::Statement query(*m_db,
        "SELECT COUNT(*) FROM comments WHERE post_id = ? AND is_approved = 1");
    query.bind(1, post_id);
    query.executeStep();
    return query.getColumn(0).getInt();
}

// 创建文章评论，不返回邮箱
int PostRepoSqlite::createComment(const Comment& comment)
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    SQLite::Statement query(*m_db,
        "INSERT INTO comments "
        "(post_id, nickname, email, content, is_approved) "
        "VALUES (?, ?, ?, ?, ?)");

    query.bind(1, comment.post_id);
    query.bind(2, comment.nickname);
    query.bind(3, comment.email);
    query.bind(4, comment.content);
    query.bind(5, comment.is_approved ? 1 : 0);
    query.exec();

    return static_cast<int>(m_db->getLastInsertRowid());
}


