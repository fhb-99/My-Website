#pragma once

#include "repo/post_repo.h"
#include "SQLiteCpp/SQLiteCpp.h"

class PostRepoSqlite : public PostRepo
{
public:
    PostRepoSqlite() {}

    std::vector<Post> GetAll(int page, int limit) override;

    Post GetByID(int id, bool& ok) override;

    int create(const Post& post) override;
private:

};
