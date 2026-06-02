#pragma once

#include "repo/post_repo.h"
#include "SQLiteCpp/SQLiteCpp.h"

class PostRepoSqlite : public PostRepo
{
public:
    PostRepoSqlite(SQLite::Database& db) : m_db(&db) {}
    ~PostRepoSqlite() noexcept override = default;

    std::vector<Post> GetAll(int page, int limit) override;

    std::optional<Post> GetByID(int id) override;

    int create(const Post& post) override;
private:
    SQLite::Database * m_db;
};
