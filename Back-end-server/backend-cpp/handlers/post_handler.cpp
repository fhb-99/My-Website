#include "handlers/post_handler.h"
#include "third_party/json.hpp"
#include "middleware/auth_token.h"

#include <regex>
#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <ctime>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cerrno>
#include <cstring>
#include <sys/types.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

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

bool IsSafeSlug(const std::string& value)
{
    if (value.empty() || value.size() > 120) {
        return false;
    }

    for (size_t i = 0; i < value.size(); ++i) {
        const char ch = value[i];
        const bool is_digit = ch >= '0' && ch <= '9';
        const bool is_lower = ch >= 'a' && ch <= 'z';
        const bool is_upper = ch >= 'A' && ch <= 'Z';
        if (!is_digit && !is_lower && !is_upper && ch != '-' && ch != '_') {
            return false;
        }
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

std::string ToLower(std::string value)
{
    for (size_t i = 0; i < value.size(); ++i) {
        value[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(value[i])));
    }
    return value;
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


std::string BuildViewVisitorKey(const httplib::Request& req)
{
    const std::string explicit_id = req.has_header("X-Visitor-Id")
        ? Trim(req.get_header_value("X-Visitor-Id"))
        : "";
    if (!explicit_id.empty()) {
        return "visitor:" + Authorization::HashToken(explicit_id);
    }

    const std::string forwarded_for = req.has_header("X-Forwarded-For")
        ? Trim(req.get_header_value("X-Forwarded-For"))
        : "";
    const std::string user_agent = req.has_header("User-Agent")
        ? req.get_header_value("User-Agent")
        : "";
    const std::string ip = forwarded_for.empty() ? req.remote_addr : forwarded_for;
    return "weak:" + Authorization::HashToken(ip + "|" + user_agent);
}


std::string EscapeHtml(const std::string& value)
{
    std::string out;
    out.reserve(value.size());

    for (size_t i = 0; i < value.size(); ++i) {
        switch (value[i]) {
        case '&': out += "&amp;"; break;
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        case '"': out += "&quot;"; break;
        case '\'': out += "&#39;"; break;
        default: out += value[i]; break;
        }
    }

    return out;
}

bool EnsureDirectory(const std::string& path)
{
    if (path.empty()) {
        return false;
    }

    std::string current;
    for (size_t i = 0; i < path.size(); ++i) {
        const char ch = path[i];
        current += ch;

        if (ch != '/' && ch != '\\' && i + 1 != path.size()) {
            continue;
        }

        std::string directory = current;
        while (!directory.empty() && (directory[directory.size() - 1] == '/' || directory[directory.size() - 1] == '\\')) {
            directory.erase(directory.size() - 1);
        }

        if (directory.empty()) {
            continue;
        }

#ifdef _WIN32
        const int rc = _mkdir(directory.c_str());
#else
        const int rc = mkdir(directory.c_str(), 0755);
#endif
        // Existing directories are fine; other mkdir failures mean the upload
        // target is not writable or an intermediate path is invalid.
        if (rc != 0 && errno != EEXIST) {
            std::cerr << "[post_handler] failed to create directory: "
                      << directory << " (" << std::strerror(errno) << ")" << std::endl;
            return false;
        }
    }

    return true;
}

bool FileExists(const std::string& path)
{
    std::ifstream in(path.c_str(), std::ios::binary);
    return in.good();
}

std::string ExtensionOf(const std::string& filename)
{
    const size_t dot = filename.find_last_of('.');
    if (dot == std::string::npos) {
        return "";
    }
    return ToLower(filename.substr(dot));
}

std::string BaseNameOf(const std::string& filename)
{
    const size_t slash = filename.find_last_of("/\\");
    const std::string name = slash == std::string::npos ? filename : filename.substr(slash + 1);
    const size_t dot = name.find_last_of('.');
    return dot == std::string::npos ? name : name.substr(0, dot);
}

std::string Slugify(const std::string& value)
{
    std::string slug;
    bool last_dash = false;

    for (size_t i = 0; i < value.size(); ++i) {
        const unsigned char ch = static_cast<unsigned char>(value[i]);
        if (std::isalnum(ch)) {
            slug += static_cast<char>(std::tolower(ch));
            last_dash = false;
        } else if (!last_dash && !slug.empty()) {
            slug += '-';
            last_dash = true;
        }
    }

    while (!slug.empty() && slug[slug.size() - 1] == '-') {
        slug.erase(slug.size() - 1);
    }

    if (slug.empty()) {
        std::ostringstream fallback;
        fallback << "post-" << std::time(nullptr);
        slug = fallback.str();
    }

    return slug;
}

std::string SafeStorageName(const std::string& filename)
{
    std::ostringstream out;
    out << std::time(nullptr) << "-" << Slugify(BaseNameOf(filename)) << ExtensionOf(filename);
    return out.str();
}

bool IsAllowedImageExtension(const std::string& ext)
{
    return ext == ".jpg" || ext == ".jpeg" || ext == ".png" ||
           ext == ".webp" || ext == ".gif";
}

bool SaveBytes(const std::string& path, const std::string& content)
{
    std::ofstream out(path.c_str(), std::ios::binary);
    if (!out.good()) {
        std::cerr << "[post_handler] failed to open file for write: "
                  << path << " (" << std::strerror(errno) << ")" << std::endl;
        return false;
    }
    out.write(content.data(), static_cast<std::streamsize>(content.size()));
    return out.good();
}

std::string FirstHeadingTitle(const std::string& markdown)
{
    std::istringstream in(markdown);
    std::string line;

    while (std::getline(in, line)) {
        const std::string trimmed = Trim(line);
        if (trimmed.size() > 2 && trimmed[0] == '#' && trimmed[1] == ' ') {
            return Trim(trimmed.substr(2));
        }
    }

    return "";
}

std::string FirstParagraphSummary(const std::string& markdown, size_t max_len)
{
    std::istringstream in(markdown);
    std::string line;

    while (std::getline(in, line)) {
        std::string trimmed = Trim(line);
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }
        if (trimmed.size() > max_len) {
            trimmed = trimmed.substr(0, max_len);
        }
        return trimmed;
    }

    return "";
}

std::string RenderMarkdownLite(const std::string& markdown)
{
    std::istringstream in(markdown);
    std::ostringstream html;
    std::string line;
    bool in_code = false;
    bool in_list = false;

    while (std::getline(in, line)) {
        const std::string trimmed = Trim(line);

        if (trimmed.size() >= 3 && trimmed.substr(0, 3) == "```") {
            if (in_list) {
                html << "</ul>\n";
                in_list = false;
            }
            html << (in_code ? "</code></pre>\n" : "<pre><code>");
            in_code = !in_code;
            continue;
        }

        if (in_code) {
            html << EscapeHtml(line) << "\n";
            continue;
        }

        if (trimmed.empty()) {
            if (in_list) {
                html << "</ul>\n";
                in_list = false;
            }
            continue;
        }

        size_t level = 0;
        while (level < trimmed.size() && trimmed[level] == '#') {
            ++level;
        }

        if (level > 0 && level <= 6 && level + 1 < trimmed.size() && trimmed[level] == ' ') {
            if (in_list) {
                html << "</ul>\n";
                in_list = false;
            }
            html << "<h" << level << ">" << EscapeHtml(Trim(trimmed.substr(level + 1)))
                 << "</h" << level << ">\n";
            continue;
        }

        if (trimmed.size() > 2 && trimmed[0] == '-' && trimmed[1] == ' ') {
            if (!in_list) {
                html << "<ul>\n";
                in_list = true;
            }
            html << "<li>" << EscapeHtml(Trim(trimmed.substr(2))) << "</li>\n";
            continue;
        }

        if (in_list) {
            html << "</ul>\n";
            in_list = false;
        }
        html << "<p>" << EscapeHtml(trimmed) << "</p>\n";
    }

    if (in_code) {
        html << "</code></pre>\n";
    }
    if (in_list) {
        html << "</ul>\n";
    }

    return html.str();
}

std::vector<std::string> ParseTagsField(const std::string& raw)
{
    std::vector<std::string> tags;
    const std::string trimmed = Trim(raw);

    if (trimmed.empty()) {
        return tags;
    }

    try {
        json parsed = json::parse(trimmed);
        if (parsed.is_array()) {
            for (const auto& item : parsed) {
                if (item.is_string()) {
                    tags.push_back(item.get<std::string>());
                }
            }
            return tags;
        }
    } catch (const std::exception&) {
        // Also accept comma-separated tags for simple form submissions.
    }

    std::istringstream in(trimmed);
    std::string item;
    while (std::getline(in, item, ',')) {
        item = Trim(item);
        if (!item.empty()) {
            tags.push_back(item);
        }
    }
    return tags;
}

bool IsTruthy(const std::string& value)
{
    const std::string normalized = ToLower(Trim(value));
    return normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on";
}

httplib::FormData GetUploadFile(const httplib::Request& req,
                                const std::string& primary_key,
                                const std::string& fallback_key)
{
    if (req.form.has_file(primary_key)) {
        return req.form.get_file(primary_key);
    }
    if (req.form.has_file(fallback_key)) {
        return req.form.get_file(fallback_key);
    }
    return httplib::FormData();
}


bool IsValidEmail(const std::string& value)
{
    const std::string email = Trim(value);
    if (email.empty() || email.size() > 120)
        return false;

    const std::regex reg(R"(^[A-Za-z0-9_\-.]+@[A-Za-z0-9\-]+(\.[A-Za-z0-9\-]+)*\.[A-Za-z]{2,}$)");
    return std::regex_match(email, reg);
}


} // namespace

bool RequireAdmin(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    const std::string prefix = "Bearer ";
    if(!req.has_header("Authorization")) {
        WriteJsonError(res, 401, "authorization token is required");
        return false;
    }

    const std::string header = req.get_header_value("Authorization");
    if(header.size() <= prefix.size() || header.compare(0, prefix.size(), prefix) != 0) {
        WriteJsonError(res, 401, "authorization token must use Bearer scheme");
        return false;
    }

    const std::string token = header.substr(prefix.size());
    if(token.empty()) {
        WriteJsonError(res, 401, "authorization token is required");
        return false;
    }

    try{
        if(!repo.IsAdminSessionValid(Authorization::HashToken(token))) {
            WriteJsonError(res, 401, "invalid or expired authorization token");
            return false;
        }
        return true;
    }
    catch(const std::exception& e) {
        WriteInternalError(res, "failed to authorize request", e);
        return false;
    }
}



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

        const int total   = repo.GetPublishedCount();
        const int total_pages = (total + limit - 1) / limit;

        json body;
        body["data"]  = data;
        body["page"]  = page;
        body["limit"] = limit;
        body["total"] = total;
        body["total_pages"] = total_pages;
        body["has_more"] = page < total_pages;
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

    const std::string username = body.contains("username") && body["username"].is_string()
        ? Trim(body["username"].get<std::string>())
        : "";
    const std::string password = body.contains("password") && body["password"].is_string()
        ? body["password"].get<std::string>()
        : "";

    if (username.empty() || password.empty()) {
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
    }
    catch(const std::exception& e) {
        WriteInternalError(res, "failed to authorize request", e);
    }
}


void HandlerCreatePost(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    json body;
    try {
        body = json::parse(req.body);
    } catch (const std::exception&) {
        WriteJsonError(res, 400, "invalid JSON body");
        return;
    }

    if (!body.contains("title") || !body["title"].is_string() || body["title"].empty()) {
        WriteJsonError(res, 400, "title is required");
        return;
    }
    if (!body.contains("slug") || !body["slug"].is_string() || body["slug"].empty()) {
        WriteJsonError(res, 400, "slug is required");
        return;
    }
    if (!body.contains("content_md") || !body["content_md"].is_string() || body["content_md"].empty()) {
        WriteJsonError(res, 400, "content_md is required");
        return;
    }

    const std::string slug = body["slug"].get<std::string>();
    if (repo.IsSlugExists(slug)) {
        WriteJsonError(res, 409, "slug already exists");
        return;
    }

    Post post;
    post.title = body["title"].get<std::string>();
    post.slug = slug;
    post.summary = body.value("summary", "");
    post.content_md = body["content_md"].get<std::string>();
    post.content_html = body.value("content_html", "");
    post.cover_url = body.value("cover_url", "");
    post.tags = ParseTagsField(body.value("tags", "[]"));
    post.is_published = body.value("is_published", false);

    try {
        const int id = repo.create(post);
        if (id <= 0) {
            WriteJsonError(res, 409, "failed to create post");
            return;
        }

        json data;
        data["id"] = id;
        data["slug"] = slug;
        data["message"] = "success";
        res.status = 201;
        res.set_content(data.dump(), "application/json; charset=utf-8");
    } catch (const std::exception& e) {
        WriteInternalError(res, "failed to create post", e);
    }
}

void AdminGetAllPosts(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    HandleGetAllPosts(repo, req, res);
}


void AdminGetPostByID(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    int id = 0;
    if(req.matches.size() < 2 || !SafeStoi(req.matches[1], id) || id < 1) {
        WriteJsonError(res, 400, "id must be a positive integer");
        return;
    }

    try {
        bool ok = false;
        const Post post = repo.GetByIDForAdmin(id, ok);
        if(!ok) {
            WriteJsonError(res, 404, "post not found");
            return;
        }

        json body = post.to_json_summary();
        body["content_md"] = post.content_md;
        body["content_html"] = post.content_html;
        body["is_published"] = post.is_published;
        body["updated_at"] = post.updated_at;
        res.set_content(body.dump(), "application/json; charset=utf-8");
    }
    catch(const std::exception& e) {
        WriteInternalError(res, "failed to get admin post", e);
    }
}

void AdminUpdatePost(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    int id = 0;
    if (req.matches.size() < 2 || !SafeStoi(req.matches[1], id) || id < 1) {
        WriteJsonError(res, 400, "id must be a positive integer");
        return;
    }

    json body;
    try {
        body = json::parse(req.body);
    } catch (const std::exception&) {
        WriteJsonError(res, 400, "invalid JSON body");
        return;
    }

    if (!body.is_object()) {
        WriteJsonError(res, 400, "JSON body must be an object");
        return;
    }

    const std::string title = body.contains("title") && body["title"].is_string()
        ? Trim(body["title"].get<std::string>())
        : "";
    if (title.empty()) {
        WriteJsonError(res, 400, "title is required");
        return;
    }

    const std::string slug = body.contains("slug") && body["slug"].is_string()
        ? Trim(body["slug"].get<std::string>())
        : "";
    if (slug.empty()) {
        WriteJsonError(res, 400, "slug is required");
        return;
    }
    if (!IsSafeSlug(slug)) {
        WriteJsonError(res, 400, "invalid slug");
        return;
    }

    const std::string content_md = body.contains("content_md") && body["content_md"].is_string()
        ? body["content_md"].get<std::string>()
        : "";
    if (Trim(content_md).empty()) {
        WriteJsonError(res, 400, "content_md is required");
        return;
    }

    Post post;
    post.title = title;
    post.slug = slug;
    post.summary = body.contains("summary") && body["summary"].is_string()
        ? Trim(body["summary"].get<std::string>())
        : "";
    post.content_md = content_md;
    post.content_html = body.contains("content_html") && body["content_html"].is_string()
        ? body["content_html"].get<std::string>()
        : "";
    if (Trim(post.content_html).empty()) {
        post.content_html = RenderMarkdownLite(post.content_md);
    }
    post.cover_url = body.contains("cover_url") && body["cover_url"].is_string()
        ? Trim(body["cover_url"].get<std::string>())
        : "";
    if (body.contains("tags")) {
        if (body["tags"].is_array()) {
            for (const auto& item : body["tags"]) {
                if (item.is_string()) {
                    const std::string tag = Trim(item.get<std::string>());
                    if (!tag.empty()) {
                        post.tags.push_back(tag);
                    }
                }
            }
        } else if (body["tags"].is_string()) {
            post.tags = ParseTagsField(body["tags"].get<std::string>());
        }
    }
    if (body.contains("is_published")) {
        if (body["is_published"].is_boolean()) {
            post.is_published = body["is_published"].get<bool>();
        } else if (body["is_published"].is_number_integer()) {
            post.is_published = body["is_published"].get<int>() != 0;
        } else if (body["is_published"].is_string()) {
            post.is_published = IsTruthy(body["is_published"].get<std::string>());
        }
    }

    try {
        bool ok = false;
        repo.GetByIDForAdmin(id, ok);
        if (!ok) {
            WriteJsonError(res, 404, "post not found");
            return;
        }

        // if (repo.IsSlugExistsForOtherPost(post.slug, id)) {
        //     WriteJsonError(res, 409, "slug already exists");
        //     return;
        // }

        if (!repo.update(id, post)) {
            WriteJsonError(res, 404, "post not found");
            return;
        }

        bool saved_ok = false;
        const Post saved = repo.GetByIDForAdmin(id, saved_ok);
        const Post& response_post = saved_ok ? saved : post;
        json response_body = response_post.to_json_summary();
        response_body["content_md"] = response_post.content_md;
        response_body["content_html"] = response_post.content_html;
        response_body["is_published"] = response_post.is_published;
        response_body["updated_at"] = response_post.updated_at;
        res.set_content(response_body.dump(), "application/json; charset=utf-8");
    } catch (const std::exception& e) {
        WriteInternalError(res, "failed to update admin post", e);
    }
}

void AdminDeletePost(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    int id = 0;
    if (req.matches.size() < 2 || !SafeStoi(req.matches[1], id) || id < 1) {
        WriteJsonError(res, 400, "id must be a positive integer");
        return;
    }

    try {
        if (!repo.remove(id)) {
            WriteJsonError(res, 404, "post not found");
            return;
        }

        json body;
        body["message"] = "success";
        res.set_content(body.dump(), "application/json; charset=utf-8");
    } catch (const std::exception& e) {
        WriteInternalError(res, "failed to delete admin post", e);
    }
}



void AdminPostImages(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    (void)repo;

    if (!req.is_multipart_form_data()) {
        WriteJsonError(res, 400, "multipart/form-data is required");
        return;
    }

    httplib::FormData file = GetUploadFile(req, "image", "file");
    if (file.content.empty() || file.filename.empty()) {
        WriteJsonError(res, 400, "image file is required");
        return;
    }

    const std::string ext = ExtensionOf(file.filename);
    if (!IsAllowedImageExtension(ext)) {
        WriteJsonError(res, 400, "unsupported image type");
        return;
    }

    const size_t kMaxImageBytes = 5U * 1024U * 1024U;
    if (file.content.size() > kMaxImageBytes) {
        WriteJsonError(res, 413, "image is too large");
        return;
    }

    const std::string dir = "uploads/images";
    if (!EnsureDirectory(dir)) {
        WriteInternalError(res, "failed to prepare image directory", std::runtime_error("invalid directory"));
        return;
    }

    std::string filename = SafeStorageName(file.filename);
    std::string path = dir + "/" + filename;
    int suffix = 1;
    while (FileExists(path)) {
        std::ostringstream renamed;
        renamed << std::time(nullptr) << "-" << suffix++ << "-" << Slugify(BaseNameOf(file.filename)) << ext;
        filename = renamed.str();
        path = dir + "/" + filename;
    }

    if (!SaveBytes(path, file.content)) {
        WriteInternalError(res, "failed to save image", std::runtime_error(path));
        return;
    }

    json body;
    body["url"] = "/uploads/images/" + filename;
    body["filename"] = filename;
    body["size"] = file.content.size();
    body["content_type"] = file.content_type;
    res.status = 201;
    res.set_content(body.dump(), "application/json; charset=utf-8");
}

void AdminPostMarkdown(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    if (!req.is_multipart_form_data()) {
        WriteJsonError(res, 400, "multipart/form-data is required");
        return;
    }

    httplib::FormData file = GetUploadFile(req, "markdown", "file");
    if (file.content.empty() || file.filename.empty()) {
        WriteJsonError(res, 400, "markdown file is required");
        return;
    }

    const std::string ext = ExtensionOf(file.filename);
    if (ext != ".md" && ext != ".markdown") {
        WriteJsonError(res, 400, "only .md or .markdown files are supported");
        return;
    }

    const size_t kMaxMarkdownBytes = 1024U * 1024U;
    if (file.content.size() > kMaxMarkdownBytes) {
        WriteJsonError(res, 413, "markdown file is too large");
        return;
    }

    std::string title = req.form.has_field("title") ? Trim(req.form.get_field("title")) : "";
    if (title.empty()) {
        title = FirstHeadingTitle(file.content);
    }
    if (title.empty()) {
        title = BaseNameOf(file.filename);
    }

    std::string slug = req.form.has_field("slug") ? Slugify(req.form.get_field("slug")) : "";
    if (slug.empty()) {
        slug = Slugify(BaseNameOf(file.filename));
    }
    if (repo.IsSlugExists(slug)) {
        WriteJsonError(res, 409, "slug already exists");
        return;
    }

    std::string summary = req.form.has_field("summary") ? Trim(req.form.get_field("summary")) : "";
    if (summary.empty()) {
        summary = FirstParagraphSummary(file.content, 200);
    }

    std::vector<std::string> tags;
    if (req.form.has_field("tags")) {
        tags = ParseTagsField(req.form.get_field("tags"));
    }

    const std::string cover_url = req.form.has_field("cover_url") ? Trim(req.form.get_field("cover_url")) : "";
    const bool is_published = req.form.has_field("is_published") ? IsTruthy(req.form.get_field("is_published")) : true;

    Post post;
    post.title = title;
    post.slug = slug;
    post.summary = summary;
    post.content_md = file.content;
    post.content_html = RenderMarkdownLite(file.content);
    post.cover_url = cover_url;
    post.tags = tags;
    post.is_published = is_published;

    try {
        const int id = repo.create(post);
        if (id <= 0) {
            WriteJsonError(res, 409, "failed to create post");
            return;
        }

        const std::string content_dir = "content/posts";
        if (EnsureDirectory(content_dir)) {
            SaveBytes(content_dir + "/" + slug + ".md", file.content);
        }

        json body;
        body["id"] = id;
        body["slug"] = slug;
        body["title"] = title;
        body["summary"] = summary;
        body["url"] = "/api/posts/" + std::to_string(id);
        body["is_published"] = is_published;
        res.status = 201;
        res.set_content(body.dump(), "application/json; charset=utf-8");
    } catch (const std::exception& e) {
        WriteInternalError(res, "failed to import markdown", e);
    }
}

void HandleGetPostBySlug(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    if (req.matches.size() < 2) {
        WriteJsonError(res, 400, "slug is required");
        return;
    }

    const std::string slug = req.matches[1];

    if (!IsSafeSlug(slug)) {
        WriteJsonError(res, 400, "invalid slug");
        return;
    }

    try {
        bool ok = false;
        Post post = repo.GetBySlug(slug, ok);

        if (!ok) {
            WriteJsonError(res, 404, "post not found");
            return;
        }

        res.set_content(post.to_json().dump(), "application/json; charset=utf-8");
    } catch (const std::exception& e) {
        WriteInternalError(res, "failed to get post by slug", e);
    }
}


void HandleRecordPostView(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    int id = 0;
    if (req.matches.size() < 2 || !SafeStoi(req.matches[1], id) || id < 1) {
        WriteJsonError(res, 400, "id must be a positive integer");
        return;
    }

    try {
        bool ok = false;
        repo.GetByID(id, ok);
        if (!ok) {
            WriteJsonError(res, 404, "post not found");
            return;
        }

        const bool counted = repo.incrementViews(id, BuildViewVisitorKey(req));

        json body;
        body["counted"] = counted;
        body["message"] = counted ? "view counted" : "view already counted today";
        res.set_content(body.dump(), "application/json; charset=utf-8");
    } catch (const std::exception& e) {
        WriteInternalError(res, "failed to record post view", e);
    }
}


void HandleGetPostComments(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    int post_id = 0;
    if (req.matches.size() < 2 || !SafeStoi(req.matches[1], post_id) || post_id < 1) {
        WriteJsonError(res, 400, "post id must be a positive integer");
        return;
    }

    int page = 1;
    int limit = 20;

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
        bool post_ok = false;
        repo.GetByID(post_id, post_ok);
        if (!post_ok) {
            WriteJsonError(res, 404, "post not found");
            return;
        }

        const std::vector<Comment> comments = repo.GetCommentsByPostID(post_id, page, limit);
        json data = json::array();
        for (const auto& comment : comments) {
            data.push_back(comment.to_json_public());
        }

        const int total = repo.GetApprovedCommentCount(post_id);
        const int total_pages = (total + limit - 1) / limit;

        json body;
        body["data"] = data;
        body["page"] = page;
        body["limit"] = limit;
        body["total"] = total;
        body["total_pages"] = total_pages;
        body["has_more"] = (page < total_pages);
        res.set_content(body.dump(), "application/json; charset=utf-8");
    } catch (const std::exception& e) {
        WriteInternalError(res, "failed to list comments", e);
    }
}


void HandleCreatePostComment(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    int post_id = 0;
    if (req.matches.size() < 2 || !SafeStoi(req.matches[1], post_id) || post_id < 1) {
        WriteJsonError(res, 400, "post id must be a positive integer");
        return;
    }

    json body;
    try {
        body = json::parse(req.body);
    } catch (const std::exception&) {
        WriteJsonError(res, 400, "invalid JSON body");
        return;
    }

    const std::string nickname = body.contains("nickname") && body["nickname"].is_string()
        ? Trim(body["nickname"].get<std::string>())
        : "";
    const std::string email = body.contains("email") && body["email"].is_string()
        ? Trim(body["email"].get<std::string>())
        : "";
    const std::string content = body.contains("content") && body["content"].is_string()
        ? Trim(body["content"].get<std::string>())
        : "";

    if (nickname.empty() || nickname.size() > 32) {
        WriteJsonError(res, 400, "nickname is required and must be within 32 characters");
        return;
    }
    if (!IsValidEmail(email)) {
        WriteJsonError(res, 400, "valid email is required");
        return;
    }
    if (content.empty() || content.size() > 800) {
        WriteJsonError(res, 400, "content is required and must be within 800 characters");
        return;
    }

    try {
        bool post_ok = false;
        repo.GetByID(post_id, post_ok);
        if (!post_ok) {
            WriteJsonError(res, 404, "post not found");
            return;
        }

        Comment comment;
        comment.post_id = post_id;
        comment.nickname = nickname;
        comment.email = email;
        comment.content = content;
        comment.is_approved = true;

        const int id = repo.createComment(comment);
        if (id <= 0) {
            WriteJsonError(res, 409, "failed to create comment");
            return;
        }

        json data;
        data["id"] = id;
        data["post_id"] = post_id;
        data["message"] = "success";
        res.status = 201;
        res.set_content(data.dump(), "application/json; charset=utf-8");
    } catch (const std::exception& e) {
        WriteInternalError(res, "failed to create comment", e);
    }
}


void HandleSearchPosts(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    std::string keyword = req.get_param_value("q");
    int limit = 10;
    if (req.has_param("limit")) {
        if (!SafeStoi(req.get_param_value("limit"), limit) || limit < 1 || limit > 100) {
            WriteJsonError(res, 400, "limit must be an integer between 1 and 100");
            return;
        }
    }
    try {
        const std::vector<Post> posts = repo.search(keyword, limit);
        json data = json::array();
        for (const auto& post : posts) {
            data.push_back(post.to_json_summary());
        }

        json body;
        body["data"] = data;
        body["message"] = "success";
        res.set_content(body.dump(), "application/json; charset=utf-8");
    } catch (const std::exception& e) {
        WriteInternalError(res, "failed to search posts", e);
    }
}


void HandleGetGuestbook(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    int page = 1;
    int limit = 10;
    if(req.has_param("page")) {
        if(!SafeStoi(req.get_param_value("page"), page) || page < 1) {
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
        const std::vector<Guestbook> guestbooks = repo.GetGuestbook(page, limit);
        json data = json::array();
        for (const auto& guestbook : guestbooks) {
            data.push_back(guestbook.to_json_public());
        }

        const int total = repo.GetGuestbookCount();
        const int total_pages = (total + limit - 1) / limit;

        json body;
        body["data"] = data;
        body["page"] = page;
        body["limit"] = limit;
        body["total"] = total;
        body["total_pages"] = total_pages;
        body["has_more"] = (page < total_pages);
        res.set_content(body.dump(), "application/json; charset=utf-8");
    }
    catch (const std::exception& e) {
        WriteInternalError(res, "failed to list guestbook", e);
    }
}


void HandleCreateGuestbook(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    json body;
    try {
        body = json::parse(req.body);
    }
    catch(const std::exception&) {
        WriteJsonError(res, 400, "invalid JSON body");
        return;
    }

    const std::string nickname = body.contains("nickname") && body["nickname"].is_string()
        ? Trim(body["nickname"].get<std::string>())
        : "";
    const std::string email = body.contains("email") && body["email"].is_string()
        ? Trim(body["email"].get<std::string>())
        : "";
    const std::string content = body.contains("content") && body["content"].is_string()
        ? Trim(body["content"].get<std::string>())
        : "";

    if(nickname.empty() || nickname.size() > 32) {
        WriteJsonError(res, 400, "nickname is required and must be within 32 characters");
        return;
    }
    if (!IsValidEmail(email)) {
        WriteJsonError(res, 400, "valid email is required");
        return;
    }
    if (content.empty() || content.size() > 800) {
        WriteJsonError(res, 400, "content is required and must be within 800 characters");
        return;
    }

    try {
        Guestbook guestbook;
        guestbook.nickname = nickname;
        guestbook.email = email;
        guestbook.content = content;
        guestbook.is_approved = true;

        const int id = repo.createGuestbook(guestbook);
        if (id <= 0) {
            WriteJsonError(res, 409, "failed to create guestbook");
            return;
        }

        json data;
        data["id"] = id;
        data["message"] = "success";
        res.status = 201;
        res.set_content(data.dump(), "application/json; charset=utf-8");
    }
    catch(const std::exception& e) {
        WriteInternalError(res, "failed to create guestbook", e);
    }
}



void AdminGetComments(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    int page = 1;
    int limit = 20;
    int post_id = 0;

    if(req.has_param("page") && (!SafeStoi(req.get_param_value("page"), page) || page < 1)) {
        WriteJsonError(res, 400, "page must be a positive integer");
        return;
    }
    if (req.has_param("limit") && (!SafeStoi(req.get_param_value("limit"), limit) || limit < 1 || limit > 100)) {
        WriteJsonError(res, 400, "limit must be an integer between 1 and 100");
        return;
    }
    if (req.has_param("postId") && (!SafeStoi(req.get_param_value("postId"), post_id) || post_id < 1)) {
        WriteJsonError(res, 400, "postId must be a positive integer");
        return;
    }

    try {
        const std::vector<Comment> comments = repo.GetCommentsForAdmin(page, limit, post_id);
        json data = json::array();
        for(const auto& comment : comments) {
            data.push_back(comment.to_json_admin());
        }

        const int total = repo.GetAdminCommentCount(post_id);
        const int total_pages = (total + limit - 1) / limit;

        json body;
        body["data"] = data;
        body["page"] = page;
        body["limit"] = limit;
        body["total"] = total;
        body["total_pages"] = total_pages;
        body["has_more"] = (page < total_pages);
        res.set_content(body.dump(), "application/json; charset=utf-8");
    }
    catch(const std::exception& e) {
        WriteInternalError(res, "failed to list comments for admin", e);
    }
}

void AdminApproveComment(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    int id = 0;
    if(req.matches.size() < 2 || !SafeStoi(req.matches[1].str(), id)) {
        WriteJsonError(res, 400, "invalid comment id");
        return;
    }

    try {
        if(!repo.SetCommentApproved(id, true)) {
            WriteJsonError(res, 404, "comment not found");
            return;
        }

        json body;
        body["message"] = "success";
        res.set_content(body.dump(), "application/json; charset=utf-8");
    } 
    catch (const std::exception& e) {
        WriteInternalError(res, "failed to approve comment", e);
    }
}

void AdminRejectComment(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    int id = 0;
    if (req.matches.size() < 2 || !SafeStoi(req.matches[1].str(), id)) {
        WriteJsonError(res, 400, "invalid comment id");
        return;
    }

    try {
        if(!repo.SetCommentApproved(id, false)) {
            WriteJsonError(res, 404, "comment not found");
            return;
        }

        json body;
        body["message"] = "success";
        res.set_content(body.dump(), "application/json; charset=utf-8");
    } 
    catch (const std::exception& e) {
        WriteInternalError(res, "failed to rejected comment", e);
    }
}

void AdminDeleteComment(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    int id = 0;
    if (req.matches.size() < 2 || !SafeStoi(req.matches[1].str(), id)) {
        WriteJsonError(res, 400, "invalid comment id");
        return;
    }
    
    try {
        if(!repo.DeleteComment(id)) {
            WriteJsonError(res, 404, "comment not found");
            return;
        }

        json body;
        body["message"] = "success";
        res.set_content(body.dump(), "application/json; charset=utf-8");
    }
    catch(const std::exception& e) {
        WriteInternalError(res, "failed to delete comment", e);
    }
}




void AdminGetGuestbook(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    int page = 1;
    int limit = 20;

    if (req.has_param("page") && (!SafeStoi(req.get_param_value("page"), page) || page < 1)) {
        WriteJsonError(res, 400, "page must be a positive integer");
        return;
    }
    if (req.has_param("limit") && (!SafeStoi(req.get_param_value("limit"), limit) || limit < 1 || limit > 100)) {
        WriteJsonError(res, 400, "limit must be an integer between 1 and 100");
        return;
    }

    try {
        const std::vector<Guestbook> guestbooks = repo.GetGuestbookForAdmin(page, limit);
        json data = json::array();
        for (const auto& guestbook : guestbooks) {
            data.push_back(guestbook.to_json_admin());
        }

        const int total = repo.GetAdminGuestbookCount();
        const int total_pages = (total + limit - 1) / limit;
        json body;
        body["data"] = data;
        body["page"] = page;
        body["limit"] = limit;
        body["total"] = total;
        body["total_pages"] = total_pages;
        body["has_more"] = page < total_pages;
        res.set_content(body.dump(), "application/json; charset=utf-8");
    } catch (const std::exception& e) {
        WriteInternalError(res, "failed to list admin guestbook", e);
    }
}

void AdminApproveGuestbook(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    int id = 0;
    if (req.matches.size() < 2 || !SafeStoi(req.matches[1].str(), id)) {
        WriteJsonError(res, 400, "invalid guestbook id");
        return;
    }
    
    try {
        if (!repo.SetGuestbookApproved(id, true)) {
            WriteJsonError(res, 404, "guestbook not found");
            return;
        }

        json body;
        body["message"] = "success";
        res.set_content(body.dump(), "application/json; charset=utf-8");
    } catch (const std::exception& e) {
        WriteInternalError(res, "failed to approve guestbook", e);
    }
}

void AdminRejectGuestbook(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    int id = 0;
    if (req.matches.size() < 2 || !SafeStoi(req.matches[1].str(), id)) {
        WriteJsonError(res, 400, "invalid guestbook id");
        return;
    }

    try {
        if (!repo.SetGuestbookApproved(id, false)) {
            WriteJsonError(res, 404, "guestbook not found");
            return;
        }

        json body;
        body["message"] = "success";
        res.set_content(body.dump(), "application/json; charset=utf-8");
    } catch (const std::exception& e) {
        WriteInternalError(res, "failed to reject guestbook", e);
    }
}

void AdminDeleteGuestbook(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    int id = 0;
    if (req.matches.size() < 2 || !SafeStoi(req.matches[1].str(), id)) {
        WriteJsonError(res, 400, "invalid guestbook id");
        return;
    }

    try {
        if (!repo.DeleteGuestbook(id)) {
            WriteJsonError(res, 404, "guestbook not found");
            return;
        }

        json body;
        body["message"] = "success";
        res.set_content(body.dump(), "application/json; charset=utf-8");
    } catch (const std::exception& e) {
        WriteInternalError(res, "failed to delete guestbook", e);
    }
}

void AdminGetModerationConfig(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    (void)req;

    try {
        const ModerationConfig config = repo.GetModerationConfig();
        json body;
        body["agent_enabled"] = config.agent_enabled;
        body["provider"] = config.provider;
        body["api_base_url"] = config.api_base_url;
        body["model"] = config.model;
        body["blocked_words"] = config.blocked_words;
        body["strictness"] = config.strictness;
        body["max_links"] = config.max_links;
        body["confidence_threshold"] = config.confidence_threshold;
        body["system_prompt"] = config.system_prompt;
        body["auto_reject_enabled"] = config.auto_reject_enabled;
        body["auto_approve_enabled"] = config.auto_approve_enabled;
        res.set_content(body.dump(), "application/json; charset=utf-8");
    } catch (const std::exception& e) {
        WriteInternalError(res, "failed to get moderation config", e);
    }
}

void AdminUpdateModerationConfig(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    json body;
    try {
        body = json::parse(req.body);
    } catch (const std::exception&) {
        WriteJsonError(res, 400, "invalid JSON body");
        return;
    }

    try {
        ModerationConfig config = repo.GetModerationConfig();
        config.agent_enabled = body.value("agent_enabled", config.agent_enabled);
        config.provider = Trim(body.value("provider", config.provider));
        config.api_base_url = Trim(body.value("api_base_url", config.api_base_url));
        config.model = Trim(body.value("model", config.model));
        config.strictness = body.value("strictness", config.strictness);
        config.max_links = body.value("max_links", config.max_links);
        config.confidence_threshold = body.value("confidence_threshold", config.confidence_threshold);
        config.system_prompt = body.value("system_prompt", config.system_prompt);
        config.auto_reject_enabled = body.value("auto_reject_enabled", config.auto_reject_enabled);
        config.auto_approve_enabled = body.value("auto_approve_enabled", config.auto_approve_enabled);

        if (body.contains("blocked_words")) {
            if (!body["blocked_words"].is_array()) {
                WriteJsonError(res, 400, "blocked_words must be an array");
                return;
            }

            config.blocked_words.clear();
            for (const auto& item : body["blocked_words"]) {
                if (!item.is_string()) {
                    WriteJsonError(res, 400, "blocked_words only accepts strings");
                    return;
                }
                const std::string word = Trim(item.get<std::string>());
                if (!word.empty()) {
                    config.blocked_words.push_back(word);
                }
            }
        }

        if (config.strictness != "loose" && config.strictness != "normal" && config.strictness != "strict") {
            WriteJsonError(res, 400, "strictness must be loose, normal or strict");
            return;
        }
        if (config.max_links < 0) {
            WriteJsonError(res, 400, "max_links must be greater than or equal to 0");
            return;
        }
        if (config.confidence_threshold < 0.0 || config.confidence_threshold > 1.0) {
            WriteJsonError(res, 400, "confidence_threshold must be between 0 and 1");
            return;
        }

        repo.SaveModerationConfig(config);

        json result;
        result["agent_enabled"] = config.agent_enabled;
        result["provider"] = config.provider;
        result["api_base_url"] = config.api_base_url;
        result["model"] = config.model;
        result["blocked_words"] = config.blocked_words;
        result["strictness"] = config.strictness;
        result["max_links"] = config.max_links;
        result["confidence_threshold"] = config.confidence_threshold;
        result["system_prompt"] = config.system_prompt;
        result["auto_reject_enabled"] = config.auto_reject_enabled;
        result["auto_approve_enabled"] = config.auto_approve_enabled;
        res.set_content(result.dump(), "application/json; charset=utf-8");
    } catch (const std::exception& e) {
        WriteInternalError(res, "failed to update moderation config", e);
    }
}

void AdminGetModerationLogs(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    int page = 1;
    int limit = 20;

    if (req.has_param("page") && (!SafeStoi(req.get_param_value("page"), page) || page < 1)) {
        WriteJsonError(res, 400, "page must be a positive integer");
        return;
    }
    if (req.has_param("limit") && (!SafeStoi(req.get_param_value("limit"), limit) || limit < 1 || limit > 100)) {
        WriteJsonError(res, 400, "limit must be an integer between 1 and 100");
        return;
    }

    try {
        const std::vector<ModerationLog> logs = repo.GetModerationLogs(page, limit);
        json data = json::array();
        for (const auto& log : logs) {
            json item;
            item["id"] = log.id;
            item["target_type"] = log.target_type;
            item["target_id"] = log.target_id;
            item["decision"] = log.decision;
            item["source"] = log.source;
            item["reason"] = log.reason;
            item["confidence"] = log.confidence;
            item["created_at"] = log.created_at;
            data.push_back(item);
        }

        const int total = repo.GetModerationLogCount();
        const int total_pages = (total + limit - 1) / limit;
        json result;
        result["data"] = data;
        result["page"] = page;
        result["limit"] = limit;
        result["total"] = total;
        result["total_pages"] = total_pages;
        result["has_more"] = page < total_pages;
        res.set_content(result.dump(), "application/json; charset=utf-8");
    } catch (const std::exception& e) {
        WriteInternalError(res, "failed to list moderation logs", e);
    }
}

void AdminTestModerationAI(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    json body;
    try {
        body = json::parse(req.body);
    } catch (const std::exception&) {
        WriteJsonError(res, 400, "invalid JSON body");
        return;
    }

    const std::string content = body.contains("content") && body["content"].is_string()
        ? Trim(body["content"].get<std::string>())
        : "";
    const std::string target_type = body.contains("target_type") && body["target_type"].is_string()
        ? body["target_type"].get<std::string>()
        : "comment";

    if (content.empty()) {
        WriteJsonError(res, 400, "content is required");
        return;
    }
    if (target_type != "comment" && target_type != "guestbook") {
        WriteJsonError(res, 400, "target_type must be comment or guestbook");
        return;
    }

    try {
        const ModerationConfig config = repo.GetModerationConfig();
        std::string decision = "pending";
        std::string reason = "no local rule matched, waiting for manual review";
        double confidence = 0.5;
        const std::string source = "rule";

        // 当前阶段只做本地规则审核，第三方 AI 调用后续再接入。
        if (!config.agent_enabled) {
            reason = "moderation agent is disabled";
            confidence = 0.0;
        } else {
            const std::string lowered_content = ToLower(content);
            for (const std::string& word : config.blocked_words) {
                const std::string blocked_word = ToLower(Trim(word));
                if (!blocked_word.empty() && lowered_content.find(blocked_word) != std::string::npos) {
                    decision = config.auto_reject_enabled ? "rejected" : "pending";
                    reason = "blocked word matched: " + blocked_word;
                    confidence = 1.0;
                    break;
                }
            }

            if (reason == "no local rule matched, waiting for manual review") {
                int link_count = 0;
                const std::regex link_regex(R"((https?://|www\.))", std::regex_constants::icase);
                for (std::sregex_iterator it(content.begin(), content.end(), link_regex), end; it != end; ++it) {
                    ++link_count;
                }
                if (link_count > config.max_links) {
                    decision = config.auto_reject_enabled ? "rejected" : "pending";
                    reason = "too many links";
                    confidence = 0.9;
                } else if (config.auto_approve_enabled) {
                    decision = "approved";
                    reason = "no local rule matched";
                    confidence = 0.6;
                }
            }
        }

        json result;
        result["decision"] = decision;
        result["reason"] = reason;
        result["confidence"] = confidence;
        result["source"] = source;
        res.set_content(result.dump(), "application/json; charset=utf-8");
    } catch (const std::exception& e) {
        WriteInternalError(res, "failed to test moderation", e);
    }
}

void AdminModerateCommentWithAI(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    int id = 0;
    if (req.matches.size() < 2 || !SafeStoi(req.matches[1].str(), id) || id < 1) {
        WriteJsonError(res, 400, "invalid comment id");
        return;
    }

    try {
        const Comment comment = repo.GetCommentForAdminByID(id);
        if (comment.id <= 0) {
            WriteJsonError(res, 404, "comment not found");
            return;
        }

        const ModerationConfig config = repo.GetModerationConfig();
        std::string decision = "pending";
        std::string reason = "no local rule matched, waiting for manual review";
        double confidence = 0.5;
        const std::string source = "rule";

        // 命中屏蔽词或链接数量超限时，根据自动拒绝开关决定是否直接隐藏。
        if (!config.agent_enabled) {
            reason = "moderation agent is disabled";
            confidence = 0.0;
        } else {
            const std::string lowered_content = ToLower(comment.content);
            for (const std::string& word : config.blocked_words) {
                const std::string blocked_word = ToLower(Trim(word));
                if (!blocked_word.empty() && lowered_content.find(blocked_word) != std::string::npos) {
                    decision = config.auto_reject_enabled ? "rejected" : "pending";
                    reason = "blocked word matched: " + blocked_word;
                    confidence = 1.0;
                    break;
                }
            }

            if (reason == "no local rule matched, waiting for manual review") {
                int link_count = 0;
                const std::regex link_regex(R"((https?://|www\.))", std::regex_constants::icase);
                for (std::sregex_iterator it(comment.content.begin(), comment.content.end(), link_regex), end; it != end; ++it) {
                    ++link_count;
                }
                if (link_count > config.max_links) {
                    decision = config.auto_reject_enabled ? "rejected" : "pending";
                    reason = "too many links";
                    confidence = 0.9;
                } else if (config.auto_approve_enabled) {
                    decision = "approved";
                    reason = "no local rule matched";
                    confidence = 0.6;
                }
            }
        }

        if (decision == "approved" && !repo.SetCommentApproved(id, true)) {
            WriteJsonError(res, 404, "comment not found");
            return;
        }
        if (decision == "rejected" && !repo.SetCommentApproved(id, false)) {
            WriteJsonError(res, 404, "comment not found");
            return;
        }

        ModerationLog log;
        log.target_type = "comment";
        log.target_id = id;
        log.decision = decision;
        log.source = source;
        log.reason = reason;
        log.confidence = confidence;
        repo.CreateModerationLog(log);

        json result;
        result["decision"] = decision;
        result["reason"] = reason;
        result["confidence"] = confidence;
        result["source"] = source;
        res.set_content(result.dump(), "application/json; charset=utf-8");
    } catch (const std::exception& e) {
        WriteInternalError(res, "failed to moderate comment", e);
    }
}

void AdminModerateGuestbookWithAI(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    int id = 0;
    if (req.matches.size() < 2 || !SafeStoi(req.matches[1].str(), id) || id < 1) {
        WriteJsonError(res, 400, "invalid guestbook id");
        return;
    }

    try {
        const Guestbook guestbook = repo.GetGuestbookForAdminByID(id);
        if (guestbook.id <= 0) {
            WriteJsonError(res, 404, "guestbook not found");
            return;
        }

        const ModerationConfig config = repo.GetModerationConfig();
        std::string decision = "pending";
        std::string reason = "no local rule matched, waiting for manual review";
        double confidence = 0.5;
        const std::string source = "rule";

        // 留言和评论使用同一套审核规则，但仍分别更新各自的数据表。
        if (!config.agent_enabled) {
            reason = "moderation agent is disabled";
            confidence = 0.0;
        } else {
            const std::string lowered_content = ToLower(guestbook.content);
            for (const std::string& word : config.blocked_words) {
                const std::string blocked_word = ToLower(Trim(word));
                if (!blocked_word.empty() && lowered_content.find(blocked_word) != std::string::npos) {
                    decision = config.auto_reject_enabled ? "rejected" : "pending";
                    reason = "blocked word matched: " + blocked_word;
                    confidence = 1.0;
                    break;
                }
            }

            if (reason == "no local rule matched, waiting for manual review") {
                int link_count = 0;
                const std::regex link_regex(R"((https?://|www\.))", std::regex_constants::icase);
                for (std::sregex_iterator it(guestbook.content.begin(), guestbook.content.end(), link_regex), end; it != end; ++it) {
                    ++link_count;
                }
                if (link_count > config.max_links) {
                    decision = config.auto_reject_enabled ? "rejected" : "pending";
                    reason = "too many links";
                    confidence = 0.9;
                } else if (config.auto_approve_enabled) {
                    decision = "approved";
                    reason = "no local rule matched";
                    confidence = 0.6;
                }
            }
        }

        if (decision == "approved" && !repo.SetGuestbookApproved(id, true)) {
            WriteJsonError(res, 404, "guestbook not found");
            return;
        }
        if (decision == "rejected" && !repo.SetGuestbookApproved(id, false)) {
            WriteJsonError(res, 404, "guestbook not found");
            return;
        }

        ModerationLog log;
        log.target_type = "guestbook";
        log.target_id = id;
        log.decision = decision;
        log.source = source;
        log.reason = reason;
        log.confidence = confidence;
        repo.CreateModerationLog(log);

        json result;
        result["decision"] = decision;
        result["reason"] = reason;
        result["confidence"] = confidence;
        result["source"] = source;
        res.set_content(result.dump(), "application/json; charset=utf-8");
    } catch (const std::exception& e) {
        WriteInternalError(res, "failed to moderate guestbook", e);
    }
}
