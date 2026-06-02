<<<<<<< HEAD
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
=======
#pragma once

#include "repo/post_repo.h"
#include "SQLiteCpp/SQLiteCpp.h"

class PostRepoSqlite : public PostRepo
{
public:
    PostRepoSqlite() {}

    std::vector<Post> GetAll(int page, int limit) override;

    std::optional<Post> GetByID(int id) override;

    int create(const Post& post) override;
private:

};
>>>>>>> 0292ce2da7582b5a6f920a12e26890491ba760f6
