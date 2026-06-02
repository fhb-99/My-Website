#include "post_repo_sqlite.h"

std::vector<Post> PostRepoSqlite::GetAll(int page, int limit)
{

}

Post PostRepoSqlite::GetByID(int id, bool& ok)
{
    ok = false;
    return Post{};
}

int PostRepoSqlite::create(const Post& post)
{

}