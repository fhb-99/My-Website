#pragma once

#include "repo/post_repo.h"
#include "SQLiteCpp/SQLiteCpp.h"

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

private:
    SQLite::Database* m_db;
};
