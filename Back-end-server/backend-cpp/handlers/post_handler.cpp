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

bool ReadStringField(const json& body, const char* key, std::string& out)
{
    if (!body.contains(key) || !body[key].is_string()) {
        return false;
    }

    out = body[key].get<std::string>();
    return !out.empty();
}

std::string ExtractBearerToken(const httplib::Request& req)
{
    const std::string prefix = "Bearer ";
    if (!req.has_header("Authorization")) {
        return "";
    }

    const std::string header = req.get_header_value("Authorization");
    if (header.size() <= prefix.size() || header.compare(0, prefix.size(), prefix) != 0) {
        return "";
    }

    return header.substr(prefix.size());
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

    std::string username;
    std::string password;

    if (!ReadStringField(body, "username", username) || !ReadStringField(body, "password", password)) {
        WriteJsonError(res, 400, "username and password are required");
        return;
    }

    try {
        bool ok = false;
        User user = repo.GetUserByUsername(username, ok);
        if (!ok || !user.is_active || user.role != "admin") {
            WriteJsonError(res, 401, "invalid username or password");
            return;
        }

        if (user.password_algo != "pbkdf2_sha256" ||
            !Authorization::VerifyPassword(password, user.password_salt, user.password_iterations, user.password_hash)) {
            WriteJsonError(res, 401, "invalid username or password");
            return;
        }

        const int kSessionTtlHours = 24;
        const std::string token = Authorization::GenerateToken();
        const std::string token_hash = Authorization::HashToken(token);
        const std::string user_agent = req.has_header("User-Agent") ? req.get_header_value("User-Agent") : "";
        const std::string expires_at = repo.CreateAdminSession(user.id, token_hash, kSessionTtlHours, user_agent);

        json value;
        value["token"] = token;
        value["token_type"] = "Bearer";
        value["expires_at"] = expires_at;
        res.set_content(value.dump(), "application/json; charset=utf-8");
    } catch (const std::exception& e) {
        WriteInternalError(res, "failed to login", e);
    }
}

bool RequireAdmin(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    const std::string token = ExtractBearerToken(req);
    if (token.empty()) {
        WriteJsonError(res, 401, "authorization token is required");
        return false;
    }

    try {
        if (!repo.IsAdminSessionValid(Authorization::HashToken(token))) {
            WriteJsonError(res, 401, "invalid or expired authorization token");
            return false;
        }
        return true;
    } catch (const std::exception& e) {
        WriteInternalError(res, "failed to authorize request", e);
        return false;
    }
}


void HandlerCreatePost(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
}
