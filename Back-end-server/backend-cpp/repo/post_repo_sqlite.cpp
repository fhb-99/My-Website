#include "repo/post_repo_sqlite.h"
#include "third_party/json.hpp"

#include <algorithm>
#include <map>
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

std::string EscapeLike(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size());
    for (const char character : value) {
        if (character == '\\' || character == '%' || character == '_') {
            escaped.push_back('\\');
        }
        escaped.push_back(character);
    }
    return escaped;
}

std::string BuildPublishedPostWhere(const PublicPostQuery& query)
{
    std::string where = " WHERE is_published = 1";
    if (!query.keyword.empty()) {
        where += " AND (title LIKE ? ESCAPE '\\' OR summary LIKE ? ESCAPE '\\' OR content_md LIKE ? ESCAPE '\\')";
    }
    if (!query.tag.empty()) {
        where += " AND tags LIKE ? ESCAPE '\\'";
    }
    return where;
}

void BindPublishedPostFilters(SQLite::Statement& statement, const PublicPostQuery& query, int& index)
{
    if (!query.keyword.empty()) {
        const std::string pattern = "%" + EscapeLike(query.keyword) + "%";
        statement.bind(index++, pattern);
        statement.bind(index++, pattern);
        statement.bind(index++, pattern);
    }
    if (!query.tag.empty()) {
        const std::string pattern = "%\"" + EscapeLike(query.tag) + "\"%";
        statement.bind(index++, pattern);
    }
}

ModerationConfig ParseModerationConfig(const std::string& value)
{
    ModerationConfig config;

    if (value.empty()) {
        return config;
    }

    try {
        const json parsed = json::parse(value);
        config.agent_enabled = parsed.value("agent_enabled", config.agent_enabled);
        config.provider = parsed.value("provider", config.provider);
        config.api_base_url = parsed.value("api_base_url", config.api_base_url);
        config.model = parsed.value("model", config.model);
        config.strictness = parsed.value("strictness", config.strictness);
        config.max_links = parsed.value("max_links", config.max_links);
        config.confidence_threshold = parsed.value("confidence_threshold", config.confidence_threshold);
        config.system_prompt = parsed.value("system_prompt", config.system_prompt);
        config.auto_reject_enabled = parsed.value("auto_reject_enabled", config.auto_reject_enabled);
        config.auto_approve_enabled = parsed.value("auto_approve_enabled", config.auto_approve_enabled);

        if (parsed.contains("blocked_words") && parsed["blocked_words"].is_array()) {
            config.blocked_words.clear();
            for (const auto& item : parsed["blocked_words"]) {
                if (item.is_string()) {
                    config.blocked_words.push_back(item.get<std::string>());
                }
            }
        }
    } catch (const std::exception&) {
        // 配置损坏时返回默认配置，避免后台页面因为错误配置不可用。
    }

    return config;
}

std::string DumpModerationConfig(const ModerationConfig& config)
{
    json data;
    data["agent_enabled"] = config.agent_enabled;
    data["provider"] = config.provider;
    data["api_base_url"] = config.api_base_url;
    data["model"] = config.model;
    data["blocked_words"] = config.blocked_words;
    data["strictness"] = config.strictness;
    data["max_links"] = config.max_links;
    data["confidence_threshold"] = config.confidence_threshold;
    data["system_prompt"] = config.system_prompt;
    data["auto_reject_enabled"] = config.auto_reject_enabled;
    data["auto_approve_enabled"] = config.auto_approve_enabled;
    return data.dump();
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

Note ReadNote(SQLite::Statement& query)
{
    Note note;
    note.id = query.getColumn(0).getInt();
    note.content = query.getColumn(1).getString();
    note.mood = query.getColumn(2).getString();
    note.created_at = query.getColumn(3).getString();
    return note;
}

Project ReadProject(SQLite::Statement& query)
{
    Project project;
    project.id = query.getColumn(0).getInt();
    project.name = query.getColumn(1).getString();
    project.summary = query.getColumn(2).getString();
    project.url = query.getColumn(3).getString();
    project.tags = ParseTags(query.getColumn(4).getString());
    project.sort_order = query.getColumn(5).getInt();
    project.created_at = query.getColumn(6).getString();
    return project;
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


Guestbook ReadGuestbook(SQLite::Statement& query)
{
    Guestbook guestbook;
    guestbook.id = query.getColumn(0).getInt();
    guestbook.nickname = query.getColumn(1).getString();
    guestbook.email = query.getColumn(2).getString();
    guestbook.content = query.getColumn(3).getString();
    guestbook.is_approved = query.getColumn(4).getInt() != 0;
    guestbook.created_at = query.getColumn(5).getString();
    guestbook.updated_at = query.getColumn(6).getString();
    return guestbook;
}

ModerationLog ReadModerationLog(SQLite::Statement& query)
{
    ModerationLog log;
    log.id = query.getColumn(0).getInt();
    log.target_type = query.getColumn(1).getString();
    log.target_id = query.getColumn(2).getInt();
    log.decision = query.getColumn(3).getString();
    log.source = query.getColumn(4).getString();
    log.reason = query.getColumn(5).getString();
    log.confidence = query.getColumn(6).getDouble();
    log.created_at = query.getColumn(7).getString();
    return log;
}

const char* kPostColumns =
    "id, title, slug, summary, content_md, content_html, cover_url, "
    "tags, is_published, views, created_at, updated_at";

const char* kCommentColumns =
    "id, post_id, nickname, email, content, is_approved, created_at, updated_at";

const char* kGuestbookColumns =
    "id, nickname, email, content, is_approved, created_at, updated_at";

const char* kModerationLogColumns =
    "id, target_type, target_id, decision, source, reason, confidence, created_at";

const char* kModerationConfigKey = "moderation_config";

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


std::vector<Post> PostRepoSqlite::GetAllForAdmin(int page, int limit)
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
        " ORDER BY updated_at DESC, created_at DESC, id DESC"
        " LIMIT ? OFFSET ?");

    query.bind(1, page);
    query.bind(2, offset);

    std::vector<Post> posts;
    while(query.executeStep()) {
        posts.push_back(ReadPost(query));
    }

    return posts;
}


// 获取已发布文章总数，供 handler 计算 total_pages 和 has_more。
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

std::vector<Post> PostRepoSqlite::ListPublishedPosts(const PublicPostQuery& public_query)
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    PublicPostQuery query = public_query;
    query.page = std::max(query.page, 1);
    query.limit = std::max(query.limit, 1);
    const int offset = (query.page - 1) * query.limit;

    SQLite::Statement statement(*m_db,
        std::string("SELECT ") + kPostColumns +
        " FROM posts" + BuildPublishedPostWhere(query) +
        " ORDER BY created_at DESC, id DESC LIMIT ? OFFSET ?");
    int index = 1;
    BindPublishedPostFilters(statement, query, index);
    statement.bind(index++, query.limit);
    statement.bind(index, offset);

    std::vector<Post> posts;
    while (statement.executeStep()) {
        posts.push_back(ReadPost(statement));
    }
    return posts;
}

int PostRepoSqlite::CountPublishedPosts(const PublicPostQuery& query)
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    SQLite::Statement statement(*m_db,
        std::string("SELECT COUNT(*) FROM posts") + BuildPublishedPostWhere(query));
    int index = 1;
    BindPublishedPostFilters(statement, query, index);
    statement.executeStep();
    return statement.getColumn(0).getInt();
}

std::vector<TagSummary> PostRepoSqlite::ListPublishedTags()
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    SQLite::Statement statement(*m_db,
        "SELECT tags FROM posts WHERE is_published = 1");
    std::map<std::string, int> tag_counts;
    while (statement.executeStep()) {
        for (const std::string& tag : ParseTags(statement.getColumn(0).getString())) {
            if (!tag.empty()) {
                ++tag_counts[tag];
            }
        }
    }

    std::vector<TagSummary> tags;
    for (const auto& entry : tag_counts) {
        TagSummary summary;
        summary.name = entry.first;
        summary.post_count = entry.second;
        tags.push_back(summary);
    }
    return tags;
}

PostNavigation PostRepoSqlite::GetPublishedNavigation(int id)
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    bool found = false;
    const Post current = GetByID(id, found);
    PostNavigation navigation;
    navigation.post_found = found;
    if (!found) {
        return navigation;
    }

    SQLite::Statement previous_statement(*m_db,
        std::string("SELECT ") + kPostColumns +
        " FROM posts WHERE is_published = 1"
        " AND (created_at < ? OR (created_at = ? AND id < ?))"
        " ORDER BY created_at DESC, id DESC LIMIT 1");
    previous_statement.bind(1, current.created_at);
    previous_statement.bind(2, current.created_at);
    previous_statement.bind(3, current.id);
    if (previous_statement.executeStep()) {
        navigation.previous = ReadPost(previous_statement);
        navigation.has_previous = true;
    }

    SQLite::Statement next_statement(*m_db,
        std::string("SELECT ") + kPostColumns +
        " FROM posts WHERE is_published = 1"
        " AND (created_at > ? OR (created_at = ? AND id > ?))"
        " ORDER BY created_at ASC, id ASC LIMIT 1");
    next_statement.bind(1, current.created_at);
    next_statement.bind(2, current.created_at);
    next_statement.bind(3, current.id);
    if (next_statement.executeStep()) {
        navigation.next = ReadPost(next_statement);
        navigation.has_next = true;
    }
    return navigation;
}

SiteConfig PostRepoSqlite::GetPublicSiteConfig()
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    SQLite::Statement statement(*m_db,
        "SELECT key, value FROM site_settings "
        "WHERE key IN ('site_title', 'site_subtitle', 'site_announcement')");
    SiteConfig config;
    while (statement.executeStep()) {
        const std::string key = statement.getColumn(0).getString();
        const std::string value = statement.getColumn(1).getString();
        if (key == "site_title") {
            config.title = value;
        } else if (key == "site_subtitle") {
            config.subtitle = value;
        } else if (key == "site_announcement") {
            config.announcement = value;
        }
    }
    return config;
}

std::vector<Note> PostRepoSqlite::ListPublishedNotes(int page, int limit)
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    page = std::max(page, 1);
    limit = std::max(limit, 1);
    const int offset = (page - 1) * limit;
    SQLite::Statement statement(*m_db,
        "SELECT id, content, mood, created_at FROM notes"
        " WHERE is_published = 1 ORDER BY created_at DESC, id DESC LIMIT ? OFFSET ?");
    statement.bind(1, limit);
    statement.bind(2, offset);

    std::vector<Note> notes;
    while (statement.executeStep()) {
        notes.push_back(ReadNote(statement));
    }
    return notes;
}

int PostRepoSqlite::CountPublishedNotes()
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    SQLite::Statement statement(*m_db,
        "SELECT COUNT(*) FROM notes WHERE is_published = 1");
    statement.executeStep();
    return statement.getColumn(0).getInt();
}

std::vector<Project> PostRepoSqlite::ListPublishedProjects(int page, int limit)
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    page = std::max(page, 1);
    limit = std::max(limit, 1);
    const int offset = (page - 1) * limit;
    SQLite::Statement statement(*m_db,
        "SELECT id, name, summary, url, tags, sort_order, created_at FROM projects"
        " WHERE is_published = 1"
        " ORDER BY sort_order ASC, created_at DESC, id DESC LIMIT ? OFFSET ?");
    statement.bind(1, limit);
    statement.bind(2, offset);

    std::vector<Project> projects;
    while (statement.executeStep()) {
        projects.push_back(ReadProject(statement));
    }
    return projects;
}

int PostRepoSqlite::CountPublishedProjects()
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    SQLite::Statement statement(*m_db,
        "SELECT COUNT(*) FROM projects WHERE is_published = 1");
    statement.executeStep();
    return statement.getColumn(0).getInt();
}


int PostRepoSqlite::GetAdminPostCount()
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    SQLite::Statement query(*m_db, "SELECT COUNT(*) FROM posts");
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


Post PostRepoSqlite::GetByIDForAdmin(int id, bool& ok)
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    SQLite::Statement query(*m_db,
        std::string("SELECT ") + kPostColumns +
        " FROM posts"
        " WHERE id = ?"
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

    SQLite::Transaction transaction(*m_db);

    // 删除文章时同步清理从属数据，避免评论和阅读事件成为孤儿数据。
    SQLite::Statement delete_comments(*m_db, "DELETE FROM comments WHERE post_id = ?");
    delete_comments.bind(1, id);
    delete_comments.exec();

    SQLite::Statement delete_views(*m_db, "DELETE FROM post_view_events WHERE post_id = ?");
    delete_views.bind(1, id);
    delete_views.exec();

    SQLite::Statement query(*m_db, "DELETE FROM posts WHERE id = ?");
    query.bind(1, id);
    const bool removed = query.exec() > 0;

    transaction.commit();
    return removed;
}

bool PostRepoSqlite::incrementViews(int id, const std::string& visitor_id)
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    if(id < 1 || visitor_id.empty()) {
        return false;
    }

    SQLite::Transaction transaction(*m_db);

    SQLite::Statement query(*m_db,
        "INSERT OR IGNORE post_view_events (post_id, visitor_id, viewed_date) "
        "VALUES (?, ?, date('now', 'localtime'))");
    query.bind(1, id);
    query.bind(2, visitor_id);
    const int inserted = query.exec();

    if(inserted > 0) {
        SQLite::Statement update(*m_db, "UPDATE posts SET views = views + 1 WHERE id = ?");
        update.bind(1, id);
        update.exec();
    }

    transaction.commit();
    return inserted > 0;
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
        // 登录时先清理过期会话。
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




// 获取某篇已发布文章下已通过审核的评论。
std::vector<Comment> PostRepoSqlite::GetCommentsByPostID(int post_id, int page, int limit)
{
    if (!m_db) {
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

// 获取评论总数，用于前端分页展示。
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

// 创建文章评论。
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

// 管理端评论列表：post_id 为 0 时查询全部文章评论。
std::vector<Comment> PostRepoSqlite::GetCommentsForAdmin(int page, int limit, int post_id)
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    page = std::max(page, 1);
    limit = std::max(limit, 1);
    const int offset = (page - 1) * limit;

    std::vector<Comment> comments;

    if (post_id > 0) {
        SQLite::Statement query(*m_db,
            "SELECT c.id, c.post_id, c.nickname, c.email, c.content, c.is_approved, "
            "c.created_at, c.updated_at, COALESCE(p.title, ''), COALESCE(p.slug, '')"
            " FROM comments c"
            " LEFT JOIN posts p ON p.id = c.post_id"
            " WHERE c.post_id = ?"
            " ORDER BY c.created_at DESC, c.id DESC"
            " LIMIT ? OFFSET ?");
        query.bind(1, post_id);
        query.bind(2, limit);
        query.bind(3, offset);

        while (query.executeStep()) {
            Comment comment = ReadComment(query);
            comment.post_title = query.getColumn(8).getString();
            comment.post_slug = query.getColumn(9).getString();
            comments.push_back(comment);
        }
        return comments;
    }

    SQLite::Statement query(*m_db,
        "SELECT c.id, c.post_id, c.nickname, c.email, c.content, c.is_approved, "
        "c.created_at, c.updated_at, COALESCE(p.title, ''), COALESCE(p.slug, '')"
        " FROM comments c"
        " LEFT JOIN posts p ON p.id = c.post_id"
        " ORDER BY c.created_at DESC, c.id DESC"
        " LIMIT ? OFFSET ?");
    query.bind(1, limit);
    query.bind(2, offset);

    while (query.executeStep()) {
        Comment comment = ReadComment(query);
        comment.post_title = query.getColumn(8).getString();
        comment.post_slug = query.getColumn(9).getString();
        comments.push_back(comment);
    }
    return comments;
}

// 管理端评论总数：post_id 为 0 时统计全部文章评论。
int PostRepoSqlite::GetAdminCommentCount(int post_id)
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    if (post_id > 0) {
        SQLite::Statement query(*m_db,
            "SELECT COUNT(*) FROM comments WHERE post_id = ?");
        query.bind(1, post_id);
        query.executeStep();
        return query.getColumn(0).getInt();
    }

    SQLite::Statement query(*m_db, "SELECT COUNT(*) FROM comments");
    query.executeStep();
    return query.getColumn(0).getInt();
}

// 管理端审核评论：approved=true 为通过，false 为拒绝展示。
bool PostRepoSqlite::SetCommentApproved(int id, bool approved)
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    SQLite::Statement query(*m_db,
        "UPDATE comments"
        " SET is_approved = ?, updated_at = datetime('now','localtime')"
        " WHERE id = ?");
    query.bind(1, approved ? 1 : 0);
    query.bind(2, id);
    return query.exec() > 0;
}

// 管理端删除评论。
bool PostRepoSqlite::DeleteComment(int id)
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    SQLite::Statement query(*m_db,
        "DELETE FROM comments WHERE id = ?");

    query.bind(1, id);
    return query.exec() > 0;
}

std::vector<Guestbook> PostRepoSqlite::GetGuestbook(int page, int limit)
{
    if(!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    page = std::max(page, 1);
    limit = std::max(limit, 1);
    const int offset = (page - 1) * limit;

    SQLite::Statement query(*m_db,
        std::string("SELECT ") + kGuestbookColumns +
        " FROM guestbook_messages"
        " WHERE is_approved = 1"
        " ORDER BY created_at DESC, id DESC"
        " LIMIT ? OFFSET ?");
    query.bind(1, limit);
    query.bind(2, offset);

    std::vector<Guestbook> guestbooks;
    while(query.executeStep()) {
        guestbooks.push_back(ReadGuestbook(query));
    }
    return guestbooks;
}


int PostRepoSqlite::createGuestbook(const Guestbook& guestbook)
{
    if(!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    SQLite::Statement query(*m_db,
        "INSERT INTO guestbook_messages "
        "(nickname, email, content, is_approved) "
        "VALUES (?, ?, ?, ?)");
    query.bind(1, guestbook.nickname);
    query.bind(2, guestbook.email);
    query.bind(3, guestbook.content);
    // 审核结果必须随留言一起入库，避免 rejected 被表默认值覆盖成已通过。
    query.bind(4, guestbook.is_approved ? 1 : 0);
    query.exec();

    return static_cast<int>(m_db->getLastInsertRowid());
}


int PostRepoSqlite::GetGuestbookCount()
{
    if(!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    SQLite::Statement query(*m_db,
        "SELECT COUNT(*) FROM guestbook_messages WHERE is_approved = 1");
    query.executeStep();
    const int count = query.getColumn(0).getInt();
    return std::max(count, 0);
}


// 管理端留言列表：包含已通过和已拒绝的留言。
std::vector<Guestbook> PostRepoSqlite::GetGuestbookForAdmin(int page, int limit)
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    page = std::max(page, 1);
    limit = std::max(limit, 1);
    const int offset = (page - 1) * limit;

    SQLite::Statement query(*m_db,
        std::string("SELECT ") + kGuestbookColumns +
        " FROM guestbook_messages"
        " ORDER BY created_at DESC, id DESC"
        " LIMIT ? OFFSET ?");
    query.bind(1, limit);
    query.bind(2, offset);

    std::vector<Guestbook> guestbooks;
    while (query.executeStep()) {
        guestbooks.push_back(ReadGuestbook(query));
    }
    return guestbooks;
}

// 管理端留言总数。
int PostRepoSqlite::GetAdminGuestbookCount()
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    SQLite::Statement query(*m_db, "SELECT COUNT(*) FROM guestbook_messages");
    query.executeStep();
    return query.getColumn(0).getInt();
}

// 管理端审核留言：approved=true 为通过，false 为拒绝展示。
bool PostRepoSqlite::SetGuestbookApproved(int id, bool approved)
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    SQLite::Statement query(*m_db,
        "UPDATE guestbook_messages"
        " SET is_approved = ?, updated_at = datetime('now','localtime')"
        " WHERE id = ?");
    query.bind(1, approved ? 1 : 0);
    query.bind(2, id);
    return query.exec() > 0;
}

// 管理端删除留言。
bool PostRepoSqlite::DeleteGuestbook(int id)
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    SQLite::Statement query(*m_db, "DELETE FROM guestbook_messages WHERE id = ?");
    query.bind(1, id);
    return query.exec() > 0;
}


ModerationConfig PostRepoSqlite::GetModerationConfig()
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    SQLite::Statement query(*m_db,
        "SELECT value FROM site_settings WHERE key = ?");
    query.bind(1, kModerationConfigKey);

    if(!query.executeStep()) {
        return ModerationConfig();
    }

    return ParseModerationConfig(query.getColumn(0).getString());
}

void PostRepoSqlite::SaveModerationConfig(const ModerationConfig& config)
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    // 先更新，若不存在再插入；兼容较旧 SQLite 版本，避免依赖 UPSERT 语法。
    SQLite::Statement update(*m_db,
        "UPDATE site_settings"
        " SET value = ?, updated_at = datetime('now','localtime')"
        " WHERE key = ?");
    update.bind(1, DumpModerationConfig(config));
    update.bind(2, kModerationConfigKey);
    if (update.exec() > 0) {
        return;
    }

    SQLite::Statement insert(*m_db,
        "INSERT INTO site_settings (key, value) VALUES (?, ?)");
    insert.bind(1, kModerationConfigKey);
    insert.bind(2, DumpModerationConfig(config));
    insert.exec();
}

std::vector<ModerationLog> PostRepoSqlite::GetModerationLogs(int page, int limit)
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    page = std::max(page, 1);
    limit = std::max(limit, 1);
    const int offset = (page - 1) * limit;

    SQLite::Statement query(*m_db,
        std::string("SELECT ") + kModerationLogColumns +
        " FROM moderation_logs"
        " ORDER BY created_at DESC, id DESC"
        " LIMIT ? OFFSET ?");
    query.bind(1, limit);
    query.bind(2, offset);

    std::vector<ModerationLog> logs;
    while (query.executeStep()) {
        logs.push_back(ReadModerationLog(query));
    }
    return logs;
}

int PostRepoSqlite::GetModerationLogCount()
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    SQLite::Statement query(*m_db, "SELECT COUNT(*) FROM moderation_logs");
    query.executeStep();
    return query.getColumn(0).getInt();
}

void PostRepoSqlite::CreateModerationLog(const ModerationLog& log)
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    SQLite::Statement query(*m_db,
        "INSERT INTO moderation_logs"
        " (target_type, target_id, decision, source, reason, confidence)"
        " VALUES (?, ?, ?, ?, ?, ?)");
    query.bind(1, log.target_type);
    query.bind(2, log.target_id);
    query.bind(3, log.decision);
    query.bind(4, log.source);
    query.bind(5, log.reason);
    query.bind(6, log.confidence);
    query.exec();
}

Comment PostRepoSqlite::GetCommentForAdminByID(int id)
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    SQLite::Statement query(*m_db,
        "SELECT c.id, c.post_id, c.nickname, c.email, c.content, c.is_approved,"
        " c.created_at, c.updated_at, COALESCE(p.title, ''), COALESCE(p.slug, '')"
        " FROM comments c"
        " LEFT JOIN posts p ON p.id = c.post_id"
        " WHERE c.id = ?");
    query.bind(1, id);

    if (!query.executeStep()) {
        return Comment();
    }

    Comment comment = ReadComment(query);
    comment.post_title = query.getColumn(8).getString();
    comment.post_slug = query.getColumn(9).getString();
    return comment;
}

Guestbook PostRepoSqlite::GetGuestbookForAdminByID(int id)
{
    if (!m_db) {
        throw std::runtime_error("database is not initialized");
    }

    SQLite::Statement query(*m_db,
        std::string("SELECT ") + kGuestbookColumns +
        " FROM guestbook_messages"
        " WHERE id = ?");
    query.bind(1, id);

    if (!query.executeStep()) {
        return Guestbook();
    }

    return ReadGuestbook(query);
}
