#pragma once

#include <string>
#include <vector>
#include "models/posts.h"
#include "models/global.h"

class PostRepo
{
public:
    virtual ~PostRepo() = default;
    PostRepo() = default;

    virtual std::vector<Post> GetAll(int page, int limit) = 0;

    // C++11 compatibility: use an output flag instead of std::optional.
    virtual Post GetByID(int id, bool& ok) = 0;

    virtual int create(const Post& post) = 0;

    virtual bool update(int id, const Post& post) = 0;

    virtual bool remove(int id) = 0;

    virtual void incrementViews(int id) = 0;

    virtual std::vector<Post> search(const std::string& keyword, int limit) = 0;

    //管理员登录校验
    virtual User GetUserByUsername(const std::string& name, bool& ok) = 0;
};
