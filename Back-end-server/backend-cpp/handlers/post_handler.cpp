#include "handlers/post_handler.h"
#include "third_party/json.hpp"
#include "middleware/auth_token.h"

#include <iostream>
#include <string>
#include <vector>

using json = nlohmann::json;

namespace {

bool SafeStoi(const std::string& value, int& out)
{
    if (value.empty()) {
        return false;
    }

    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] < '0' || value[i] > '9') {
            return false;
        }
    }

    try {
        out = std::stoi(value);
    } catch (const std::exception&) {
        return false;
    }

    return true;
}

void WriteJsonError(httplib::Response& res, int status, const std::string& message)
{
    res.status = status;
    json body;
    body["error"] = message;
    res.set_content(body.dump(), "application/json; charset=utf-8");
}

void WriteInternalError(httplib::Response& res, const char* context, const std::exception& e)
{
    std::cerr << "[post_handler] " << context << ": " << e.what() << std::endl;
    WriteJsonError(res, 500, "internal server error");
}

} // namespace

void HandleGetAllPosts(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    int page = 1;
    int limit = 10;

    if (req.has_param("page")) {
        if (!SafeStoi(req.get_param_value("page"), page) || page < 1) {
            WriteJsonError(res, 400, "page must be a positive integer");
            return;
        }
    }

    if (req.has_param("limit")) {
        if (!SafeStoi(req.get_param_value("limit"), limit) || limit < 1 || limit > 100) {
            WriteJsonError(res, 400, "limit must be an integer between 1 and 100");
            return;
        }
    }

    try {
        const std::vector<Post> posts = repo.GetAll(page, limit);

        json data = json::array();
        for (const auto& post : posts) {
            data.push_back(post.to_json_summary());
        }

        json body;
        body["data"] = data;
        body["page"] = page;
        body["limit"] = limit;
        res.set_content(body.dump(), "application/json; charset=utf-8");
    } catch (const std::exception& e) {
        WriteInternalError(res, "failed to list posts", e);
    }
}

void HandleGetPostByID(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    int id = 0;
    if (req.matches.size() < 2 || !SafeStoi(req.matches[1], id) || id < 1) {
        WriteJsonError(res, 400, "id must be a positive integer");
        return;
    }

    try {
        bool ok = false;
        Post post = repo.GetByID(id, ok);

        if (!ok) {
            WriteJsonError(res, 404, "post not found");
            return;
        }

        repo.incrementViews(id);
        res.set_content(post.to_json().dump(), "application/json; charset=utf-8");
    } catch (const std::exception& e) {
        WriteInternalError(res, "failed to get post", e);
    }
}


void HandleLogin(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    json body;
    try 
    {
        body = json::parse(req.body);
    } 
    catch (const std::exception& e) 
    {
        WriteJsonError(res, 400, "invalid JSON body");
        return;
    }

    const std::string& username = body["username"].get<std::string>();
    const std::string& password = body["password"].get<std::string>();

    if (username.empty() || password.empty()) {
        WriteJsonError(res, 400, "username and password are required");
        return;
    }
    
    bool ok = false;
    User user = repo.GetUserByUsername(username, ok);
    if (!ok) {
        WriteJsonError(res, 401, "invalid username or password");
        return;
    }

    if (user.password != password) {
        WriteJsonError(res, 401, "invalid username or password");
        return;
    }

    Json value;
    //生成一个唯一的token todo
    std::string token = Authorization::GenerateToken();

    value["token"] = token;

    res.set_content(value.dummp(), "application/json; charset=utf-8");
}


void HandlerCreatePost(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    
}
