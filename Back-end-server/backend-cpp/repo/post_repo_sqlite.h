#pragma once

#include "repo/post_repo.h"
#include "SQLiteCpp/SQLiteCpp.h"
#include "models/global.h"

class PostRepoSqlite : public PostRepo
{
public:
    explicit PostRepoSqlite(SQLite::Database& db) : m_db(&db) {}
    ~PostRepoSqlite() noexcept override = default;

    std::vector<Post> GetAll(int page, int limit) override;

    Post GetByID(int id, bool& ok) override;

    int create(const Post& post) override;

    bool update(int id, const Post& post) override;

    bool remove(int id) override;

    void incrementViews(int id) override;

    std::vector<Post> search(const std::string& keyword, int limit) override;

    // 管理员登录校验
    User GetUserByUsername(const std::string& name, bool& ok) override;

    std::string CreateAdminSession(int user_id,
                                   const std::string& token_hash,
                                   int ttl_hours,
                                   const std::string& user_agent) override;

    bool IsAdminSessionValid(const std::string& token_hash) override;

private:
    SQLite::Database* m_db;
};
