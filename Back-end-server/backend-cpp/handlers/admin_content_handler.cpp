#include "handlers/admin_content_handler.h"

#include "third_party/json.hpp"

#include <algorithm>
#include <cctype>
#include <exception>
#include <iostream>
#include <sstream>
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

std::string Trim(const std::string& value)
{
    size_t begin = 0;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin]))) {
        ++begin;
    }

    size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    return value.substr(begin, end - begin);
}

void WriteJsonError(httplib::Response& res, int status, const std::string& message)
{
    res.status = status;
    res.set_content(json{{"error", message}}.dump(), "application/json; charset=utf-8");
}

void WriteInternalError(httplib::Response& res, const char* context, const std::exception& error)
{
    std::cerr << "[admin_content_handler] " << context << ": " << error.what() << std::endl;
    WriteJsonError(res, 500, "internal server error");
}

// 两个内容列表共用相同的后台分页规则，避免草稿列表和项目列表的边界不一致。
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

bool ParseRouteID(const httplib::Request& req, httplib::Response& res, int& id)
{
    if (req.matches.size() < 2 || !SafeStoi(req.matches[1], id) || id < 1) {
        WriteJsonError(res, 400, "id must be a positive integer");
        return false;
    }
    return true;
}

bool ParseJsonBody(const httplib::Request& req, httplib::Response& res, json& body)
{
    try {
        body = json::parse(req.body);
    } catch (const std::exception&) {
        WriteJsonError(res, 400, "invalid JSON body");
        return false;
    }
    if (!body.is_object()) {
        WriteJsonError(res, 400, "JSON body must be an object");
        return false;
    }
    return true;
}

bool ReadPublishedValue(const json& body, bool default_value)
{
    if (!body.contains("is_published")) {
        return default_value;
    }
    if (body["is_published"].is_boolean()) {
        return body["is_published"].get<bool>();
    }
    if (body["is_published"].is_number_integer()) {
        return body["is_published"].get<int>() != 0;
    }
    return default_value;
}

std::vector<std::string> ReadTags(const json& body)
{
    std::vector<std::string> tags;
    if (!body.contains("tags") || !body["tags"].is_array()) {
        return tags;
    }

    for (const auto& item : body["tags"]) {
        if (item.is_string()) {
            const std::string tag = Trim(item.get<std::string>());
            if (!tag.empty()) {
                tags.push_back(tag);
            }
        }
    }
    return tags;
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

json ToAdminJson(const Note& note)
{
    json body = note.to_json_public();
    body["is_published"] = note.is_published;
    return body;
}

json ToAdminJson(const Project& project)
{
    json body = project.to_json_public();
    body["sort_order"] = project.sort_order;
    body["is_published"] = project.is_published;
    return body;
}

bool ParseNote(const json& body, httplib::Response& res, Note& note)
{
    if (!body.contains("content") || !body["content"].is_string()) {
        WriteJsonError(res, 400, "content is required");
        return false;
    }

    note.content = Trim(body["content"].get<std::string>());
    if (note.content.empty()) {
        WriteJsonError(res, 400, "content is required");
        return false;
    }
    note.mood = body.contains("mood") && body["mood"].is_string()
        ? Trim(body["mood"].get<std::string>())
        : "";
    note.is_published = ReadPublishedValue(body, false);
    return true;
}

bool ParseProject(const json& body, httplib::Response& res, Project& project)
{
    if (!body.contains("name") || !body["name"].is_string()) {
        WriteJsonError(res, 400, "name is required");
        return false;
    }

    project.name = Trim(body["name"].get<std::string>());
    if (project.name.empty()) {
        WriteJsonError(res, 400, "name is required");
        return false;
    }
    project.summary = body.contains("summary") && body["summary"].is_string()
        ? Trim(body["summary"].get<std::string>())
        : "";
    project.url = body.contains("url") && body["url"].is_string()
        ? Trim(body["url"].get<std::string>())
        : "";
    project.tags = ReadTags(body);
    project.sort_order = body.contains("sort_order") && body["sort_order"].is_number_integer()
        ? body["sort_order"].get<int>()
        : 0;
    project.is_published = ReadPublishedValue(body, false);
    return true;
}

bool ParseMusicConfig(const json& body, httplib::Response& res, MusicConfig& config)
{
    if (!body.contains("enabled") || !body["enabled"].is_boolean()) {
        WriteJsonError(res, 400, "enabled must be a boolean");
        return false;
    }
    if (!body.contains("volume") || !body["volume"].is_number()) {
        WriteJsonError(res, 400, "volume must be a number");
        return false;
    }
    if (!body.contains("tracks") || !body["tracks"].is_array()) {
        WriteJsonError(res, 400, "tracks must be an array");
        return false;
    }

    config.enabled = body["enabled"].get<bool>();
    config.volume = body["volume"].get<double>();
    if (config.volume < 0 || config.volume > 1) {
        WriteJsonError(res, 400, "volume must be between 0 and 1");
        return false;
    }

    for (size_t index = 0; index < body["tracks"].size(); ++index) {
        const json& item = body["tracks"][index];
        if (!item.is_object() || !item.contains("title") || !item["title"].is_string() ||
            !item.contains("audio_url") || !item["audio_url"].is_string()) {
            WriteJsonError(res, 400, "each track requires title and audio_url");
            return false;
        }

        MusicTrack track;
        track.id = item.contains("id") && item["id"].is_number_integer()
            ? item["id"].get<int>()
            : static_cast<int>(index + 1);
        track.title = Trim(item["title"].get<std::string>());
        track.audio_url = Trim(item["audio_url"].get<std::string>());
        if (track.title.empty() || track.audio_url.empty()) {
            WriteJsonError(res, 400, "each track requires title and audio_url");
            return false;
        }

        track.artist = item.contains("artist") && item["artist"].is_string()
            ? Trim(item["artist"].get<std::string>())
            : "";
        track.cover_url = item.contains("cover_url") && item["cover_url"].is_string()
            ? Trim(item["cover_url"].get<std::string>())
            : "";
        track.sort_order = item.contains("sort_order") && item["sort_order"].is_number_integer()
            ? item["sort_order"].get<int>()
            : static_cast<int>(index);
        track.is_enabled = !item.contains("is_enabled") ||
            (item["is_enabled"].is_boolean() && item["is_enabled"].get<bool>());
        config.tracks.push_back(track);
    }
    return true;
}

const char* GetConfigStorageKey(const std::string& key)
{
    if (key == "title") return "site_title";
    if (key == "subtitle") return "site_subtitle";
    if (key == "announcement") return "site_announcement";
    return nullptr;
}

} // namespace

void AdminGetNotes(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    int page = 1;
    int limit = 10;
    if (!ParsePagination(req, res, page, limit)) {
        return;
    }

    try {
        json data = json::array();
        for (const Note& note : repo.ListNotesForAdmin(page, limit)) {
            data.push_back(ToAdminJson(note));
        }
        res.set_content(BuildPageResult(data, page, limit, repo.CountNotesForAdmin()).dump(),
                        "application/json; charset=utf-8");
    } catch (const std::exception& error) {
        WriteInternalError(res, "failed to list admin notes", error);
    }
}

void AdminCreateNote(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    json body;
    Note note;
    if (!ParseJsonBody(req, res, body) || !ParseNote(body, res, note)) {
        return;
    }

    try {
        const int id = repo.CreateNote(note);
        res.status = 201;
        res.set_content(json{{"id", id}, {"message", "success"}}.dump(),
                        "application/json; charset=utf-8");
    } catch (const std::exception& error) {
        WriteInternalError(res, "failed to create note", error);
    }
}

void AdminUpdateNote(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    int id = 0;
    json body;
    Note note;
    if (!ParseRouteID(req, res, id) || !ParseJsonBody(req, res, body) || !ParseNote(body, res, note)) {
        return;
    }

    try {
        if (!repo.UpdateNote(id, note)) {
            WriteJsonError(res, 404, "note not found");
            return;
        }
        res.set_content(json{{"message", "success"}}.dump(), "application/json; charset=utf-8");
    } catch (const std::exception& error) {
        WriteInternalError(res, "failed to update note", error);
    }
}

void AdminDeleteNote(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    int id = 0;
    if (!ParseRouteID(req, res, id)) {
        return;
    }

    try {
        if (!repo.DeleteNote(id)) {
            WriteJsonError(res, 404, "note not found");
            return;
        }
        res.set_content(json{{"message", "success"}}.dump(), "application/json; charset=utf-8");
    } catch (const std::exception& error) {
        WriteInternalError(res, "failed to delete note", error);
    }
}

void AdminGetProjects(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    int page = 1;
    int limit = 10;
    if (!ParsePagination(req, res, page, limit)) {
        return;
    }

    try {
        json data = json::array();
        for (const Project& project : repo.ListProjectsForAdmin(page, limit)) {
            data.push_back(ToAdminJson(project));
        }
        res.set_content(BuildPageResult(data, page, limit, repo.CountProjectsForAdmin()).dump(),
                        "application/json; charset=utf-8");
    } catch (const std::exception& error) {
        WriteInternalError(res, "failed to list admin projects", error);
    }
}

void AdminCreateProject(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    json body;
    Project project;
    if (!ParseJsonBody(req, res, body) || !ParseProject(body, res, project)) {
        return;
    }

    try {
        const int id = repo.CreateProject(project);
        res.status = 201;
        res.set_content(json{{"id", id}, {"message", "success"}}.dump(),
                        "application/json; charset=utf-8");
    } catch (const std::exception& error) {
        WriteInternalError(res, "failed to create project", error);
    }
}

void AdminUpdateProject(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    int id = 0;
    json body;
    Project project;
    if (!ParseRouteID(req, res, id) || !ParseJsonBody(req, res, body) || !ParseProject(body, res, project)) {
        return;
    }

    try {
        if (!repo.UpdateProject(id, project)) {
            WriteJsonError(res, 404, "project not found");
            return;
        }
        res.set_content(json{{"message", "success"}}.dump(), "application/json; charset=utf-8");
    } catch (const std::exception& error) {
        WriteInternalError(res, "failed to update project", error);
    }
}

void AdminDeleteProject(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    int id = 0;
    if (!ParseRouteID(req, res, id)) {
        return;
    }

    try {
        if (!repo.DeleteProject(id)) {
            WriteJsonError(res, 404, "project not found");
            return;
        }
        res.set_content(json{{"message", "success"}}.dump(), "application/json; charset=utf-8");
    } catch (const std::exception& error) {
        WriteInternalError(res, "failed to delete project", error);
    }
}

void AdminGetSiteConfig(PostRepo& repo, const httplib::Request&, httplib::Response& res)
{
    try {
        // 管理端和公开端读取同一组展示配置，保存后访客页面会读取到相同的数据。
        res.set_content(repo.GetPublicSiteConfig().to_json_public().dump(),
                        "application/json; charset=utf-8");
    } catch (const std::exception& error) {
        WriteInternalError(res, "failed to get site config", error);
    }
}

void AdminUpdateSiteConfig(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    if (req.matches.size() < 2) {
        WriteJsonError(res, 400, "config key is required");
        return;
    }

    const std::string config_key = req.matches[1];
    const char* storage_key = GetConfigStorageKey(config_key);
    if (!storage_key) {
        WriteJsonError(res, 400, "invalid config key");
        return;
    }

    json body;
    if (!ParseJsonBody(req, res, body)) {
        return;
    }
    if (!body.contains("value") || !body["value"].is_string()) {
        WriteJsonError(res, 400, "value is required");
        return;
    }

    try {
        repo.SaveSiteSetting(storage_key, body["value"].get<std::string>());
        res.set_content(repo.GetPublicSiteConfig().to_json_public().dump(),
                        "application/json; charset=utf-8");
    } catch (const std::exception& error) {
        WriteInternalError(res, "failed to update site config", error);
    }
}

void AdminGetMusicConfig(PostRepo& repo, const httplib::Request&, httplib::Response& res)
{
    try {
        // 管理端必须拿到未启用曲目，才能继续编辑后再决定是否向访客展示。
        res.status = 200;
        res.set_content(repo.GetAdminMusicConfig().to_json_admin().dump(),
                        "application/json; charset=utf-8");
    } catch (const std::exception& error) {
        WriteInternalError(res, "failed to get music config", error);
    }
}

void AdminUpdateMusicConfig(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    json body;
    MusicConfig config;
    if (!ParseJsonBody(req, res, body) || !ParseMusicConfig(body, res, config)) {
        return;
    }

    try {
        repo.SaveMusicConfig(config);
        // 保存后重新读取，确保曲目按 sort_order 排列并带回分配给新曲目的 ID。
        res.status = 200;
        res.set_content(repo.GetAdminMusicConfig().to_json_admin().dump(),
                        "application/json; charset=utf-8");
    } catch (const std::exception& error) {
        WriteInternalError(res, "failed to update music config", error);
    }
}
