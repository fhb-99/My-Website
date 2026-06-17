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

    // 管理员登录校验：只读取用户凭据，不在 handler 中直接写 SQL。
    virtual User GetUserByUsername(const std::string& name, bool& ok) = 0;

    // 登录成功后保存 token 的哈希值，原始 token 只返回给前端。
    virtual std::string CreateAdminSession(int user_id,
                                           const std::string& token_hash,
                                           int ttl_hours,
                                           const std::string& user_agent) = 0;

    // 后台接口鉴权：校验 token hash 是否存在、未过期、未撤销。
    virtual bool IsAdminSessionValid(const std::string& token_hash) = 0;
};
