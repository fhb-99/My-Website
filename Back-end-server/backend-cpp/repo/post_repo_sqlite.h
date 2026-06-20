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

    //通过slug来判断文章是否存在
    bool IsSlugExists(const std::string& slug) override;

    Post GetBySlug(const std::string& slug, bool& ok) override;

    //管理员登录校验
    User GetUserByUsername(const std::string& name, bool& flag) override;

    //登录成功后，拿到token，保存其哈希值，原始token返回给前端
    std::string CreateAdminSession(int user_id, const std::string& token_hash, 
            int ttl_hours, const std::string& user_agent) override;

    //后端对前端请求传来的原始token进行鉴权
    bool IsAdminSessionValid(const std::string& token_hash) override;
private:
    SQLite::Database* m_db;
};
