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

    // 获取已发布文章总数，用于分页元信息（total_pages / has_more）
    virtual int GetPublishedCount() = 0;

    // C++11 compatibility: use an output flag instead of std::optional.
    virtual Post GetByID(int id, bool& ok) = 0;

    virtual int create(const Post& post) = 0;

    virtual bool update(int id, const Post& post) = 0;

    virtual bool remove(int id) = 0;

    virtual void incrementViews(int id) = 0;

    virtual std::vector<Post> search(const std::string& keyword, int limit) = 0;

    //通过slug来判断文章是否存在
    virtual bool IsSlugExists(const std::string& slug) = 0;

    virtual Post GetBySlug(const std::string& slug, bool& ok) = 0;

    //管理员登录校验
    virtual User GetUserByUsername(const std::string& name, bool& flag) = 0;

    //登录成功后，拿到token，保存其哈希值，原始token返回给前端
    virtual std::string CreateAdminSession(int user_id, const std::string& token_hash, 
                                        int ttl_hours, const std::string& user_agent) = 0;

    //后端对前端请求传来的原始token进行鉴权
    virtual bool IsAdminSessionValid(const std::string& token_hash) = 0;
};
