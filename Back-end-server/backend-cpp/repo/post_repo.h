#pragma once

#include <vector>
#include <optional>
#include "models/posts.h"


class PostRepo
{
public:
    virtual ~PostRepo() = default;
    PostRepo = default;

    virtual std::vector<Post> GetAll(int page, int limit) = 0;

    virtual std::optional<Post> GetByID(int id) = 0;
private:

};