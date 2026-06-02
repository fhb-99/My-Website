#pragma once

#include <vector>
#include <optional>
#include "models/posts.h"


class PostRepo
{
public:
    virtual ~PostRepo() = default;
    PostRepo() = default;

    virtual std::vector<Post> GetAll(int page, int limit) = 0;

    virtual std::optional<Post> GetByID(int id) = 0;
     
    virtual int create(const Post& post) = 0;

    virtual bool update(int id, const Post& post) = 0;

    virtual bool remove(int id) = 0;

    virtual void incrementViews(int id) = 0;
    
    virtual std::vector<Post> search(const std::string& keyword, int limit) = 0;
private:

};