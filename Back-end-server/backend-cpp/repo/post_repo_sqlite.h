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
