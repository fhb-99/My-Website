#include "post_handler.h"

using json = nlohmann::json;

void Handler::HandlerGetAllPosts(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    int page = req.has_param("page") ? std::stoi(req.get_param_value("page")) : 1;
    int limit = req.has_param("limit") ? std::stoi(req.get_param_value("limit")) : 10;

    auto posts = repo.GetAll(page, limit);

    json arr = json::array();
    for(auto& p : posts)
    {
        arr.push_back(p.to_json_summary());
    }

    json root = {{"data", arr}, {"page", page}, {"limit", limit}};
    res.set_content(root.dump(), "application/json; charset=utf-8");
}

void Handler::HandlerGetPostByID(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    int id = std::stoi(req.matches[1]);
    auto post = repo.GetByID(id);

    if (post) 
    {
        repo.incrementViews(id);
        res.set_content(post->to_json().dump(), "application/json; charset=utf-8");
    } 
    else 
    {
        res.status = 404;
        res.set_content(R"({"error":"文章不存在"})", "application/json; charset=utf-8");
    }
}