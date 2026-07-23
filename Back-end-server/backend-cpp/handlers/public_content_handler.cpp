#include "handlers/public_content_handler.h"

#include "third_party/json.hpp"

#include <exception>
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
    for (const char character : value) {
        if (character < '0' || character > '9') {
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
    res.set_content(json{{"error", message}}.dump(), "application/json; charset=utf-8");
}

void WriteInternalError(httplib::Response& res, const char* context, const std::exception& error)
{
    std::cerr << "[public_content_handler] " << context << ": " << error.what() << std::endl;
    WriteJsonError(res, 500, "internal server error");
}

bool ParsePagination(const httplib::Request& req, httplib::Response& res, int& page, int& limit)
{
    page = 1;
    limit = 10;
    if (req.has_param("page") &&
        (!SafeStoi(req.get_param_value("page"), page) || page < 1)) {
        WriteJsonError(res, 400, "page must be a positive integer");
        return false;
    }
    if (req.has_param("limit") &&
        (!SafeStoi(req.get_param_value("limit"), limit) || limit < 1 || limit > 100)) {
        WriteJsonError(res, 400, "limit must be an integer between 1 and 100");
        return false;
    }
    return true;
}

json BuildPageResult(const json& data, int page, int limit, int total)
{
    const int total_pages = (total + limit - 1) / limit;
    return {
        {"data", data},
        {"page", page},
        {"limit", limit},
        {"total", total},
        {"total_pages", total_pages},
        {"has_more", page < total_pages}
    };
}

} // namespace

void HandleListPublicPosts(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    int page = 1;
    int limit = 10;
    if (!ParsePagination(req, res, page, limit)) {
        return;
    }

    try {
        PublicPostQuery query;
        query.page = page;
        query.limit = limit;
        query.keyword = req.has_param("q") ? req.get_param_value("q") : "";
        query.tag = req.has_param("tag") ? req.get_param_value("tag") : "";

        json data = json::array();
        for (const Post& post : repo.ListPublishedPosts(query)) {
            data.push_back(post.to_json_summary());
        }
        res.status = 200;
        res.set_content(BuildPageResult(data, page, limit, repo.CountPublishedPosts(query)).dump(),
                        "application/json; charset=utf-8");
    } catch (const std::exception& error) {
        WriteInternalError(res, "failed to list posts", error);
    }
}

void HandleGetPublicConfig(PostRepo& repo, const httplib::Request&, httplib::Response& res)
{
    try {
        res.status = 200;
        res.set_content(repo.GetPublicSiteConfig().to_json_public().dump(),
                        "application/json; charset=utf-8");
    } catch (const std::exception& error) {
        WriteInternalError(res, "failed to get public config", error);
    }
}

void HandleGetNotes(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    int page = 1;
    int limit = 10;
    if (!ParsePagination(req, res, page, limit)) {
        return;
    }

    try {
        json data = json::array();
        for (const Note& note : repo.ListPublishedNotes(page, limit)) {
            data.push_back(note.to_json_public());
        }
        res.status = 200;
        res.set_content(BuildPageResult(data, page, limit, repo.CountPublishedNotes()).dump(),
                        "application/json; charset=utf-8");
    } catch (const std::exception& error) {
        WriteInternalError(res, "failed to list notes", error);
    }
}

void HandleGetProjects(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    int page = 1;
    int limit = 10;
    if (!ParsePagination(req, res, page, limit)) {
        return;
    }

    try {
        json data = json::array();
        for (const Project& project : repo.ListPublishedProjects(page, limit)) {
            data.push_back(project.to_json_public());
        }
        res.status = 200;
        res.set_content(BuildPageResult(data, page, limit, repo.CountPublishedProjects()).dump(),
                        "application/json; charset=utf-8");
    } catch (const std::exception& error) {
        WriteInternalError(res, "failed to list projects", error);
    }
}

void HandleGetTags(PostRepo& repo, const httplib::Request&, httplib::Response& res)
{
    try {
        json data = json::array();
        for (const TagSummary& tag : repo.ListPublishedTags()) {
            data.push_back({{"name", tag.name}, {"post_count", tag.post_count}});
        }
        res.status = 200;
        res.set_content(json({{"data", data}}).dump(), "application/json; charset=utf-8");
    } catch (const std::exception& error) {
        WriteInternalError(res, "failed to list tags", error);
    }
}

void HandleGetPostNavigation(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    int id = 0;
    if (req.matches.size() < 2 || !SafeStoi(req.matches[1], id) || id < 1) {
        WriteJsonError(res, 400, "id must be a positive integer");
        return;
    }

    try {
        const PostNavigation navigation = repo.GetPublishedNavigation(id);
        if (!navigation.post_found) {
            WriteJsonError(res, 404, "post not found");
            return;
        }

        json body;
        body["previous"] = navigation.has_previous
            ? navigation.previous.to_json_summary()
            : json(nullptr);
        body["next"] = navigation.has_next
            ? navigation.next.to_json_summary()
            : json(nullptr);
        res.status = 200;
        res.set_content(body.dump(), "application/json; charset=utf-8");
    } catch (const std::exception& error) {
        WriteInternalError(res, "failed to get post navigation", error);
    }
}

void HandleSearchPublicPosts(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    int limit = 10;
    if (req.has_param("limit") &&
        (!SafeStoi(req.get_param_value("limit"), limit) || limit < 1 || limit > 100)) {
        WriteJsonError(res, 400, "limit must be an integer between 1 and 100");
        return;
    }

    try {
        PublicPostQuery query;
        query.limit = limit;
        query.keyword = req.has_param("q") ? req.get_param_value("q") : "";
        json data = json::array();
        for (const Post& post : repo.ListPublishedPosts(query)) {
            data.push_back(post.to_json_summary());
        }
        res.status = 200;
        res.set_content(json({{"data", data}, {"message", "success"}}).dump(),
                        "application/json; charset=utf-8");
    } catch (const std::exception& error) {
        WriteInternalError(res, "failed to search posts", error);
    }
}
