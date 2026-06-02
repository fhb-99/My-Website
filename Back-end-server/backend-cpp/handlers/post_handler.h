#pragma once

#include "third_party/httplib.h"
#include "repo/post_repo.h" 
#include "models/singleton.h"

class Handler : public Singleton<Handler>
{
protected:
    Handler() = default;
public:
    ~Handler() {}

    //获取所有的文章列表
    void HandlerGetAllPosts(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

    //通过id来获取具体的文章
    void HandlerGetPostByID(PostRepo& repo, const httplib::Request& req, httplib::Response& res);
private:

};