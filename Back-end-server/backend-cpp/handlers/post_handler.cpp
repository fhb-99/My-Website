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

//去除首尾空白字符
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

//HTML 特殊字符转义
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

//递归创建多级目录
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

//获取文件后缀（小写）
std::string ExtensionOf(const std::string& filename)
{
    const size_t dot = filename.find_last_of('.');
    if (dot == std::string::npos) {
        return "";
    }
    return ToLower(filename.substr(dot));
}

//获取纯文件名（不含路径、不含后缀）
std::string BaseNameOf(const std::string& filename)
{
    const size_t slash = filename.find_last_of("/\\");
    const std::string name = slash == std::string::npos ? filename : filename.substr(slash + 1);
    const size_t dot = name.find_last_of('.');
    return dot == std::string::npos ? name : name.substr(0, dot);
}

//生成 URL / 文件名安全短标识（slug）
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

//生成安全唯一存储文件名
std::string SafeStorageName(const std::string& filename)
{
    std::ostringstream out;
    out << std::time(nullptr) << "-" << Slugify(BaseNameOf(filename)) << ExtensionOf(filename);
    return out.str();
}

//校验图片文件后缀白名单
bool IsAllowedImageExtension(const std::string& ext)
{
    return ext == ".jpg" || ext == ".jpeg" || ext == ".png" ||
           ext == ".webp" || ext == ".gif";
}

//二进制字节写入文件
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

//提取 Markdown 第一个一级标题
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

//提取首段摘要
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

// 轻量 Markdown 转 HTML 渲染器
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

//兼容双格式解析标签列表
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

//字符串真值判断
bool IsTruthy(const std::string& value)
{
    const std::string normalized = ToLower(Trim(value));
    return normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on";
}

//HTTP 上传文件兼容
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

    // 正则：用户名允许字母数字._-，域名层级合法，后缀2位以上
    const std::regex reg(R"(^[A-Za-z0-9_\-.]+@[A-Za-z0-9\-]+(\.[A-Za-z0-9\-]+)*\.[A-Za-z]{2,}$)");
    return std::regex_match(email, reg);
}


} // namespace

//从 HTTP 请求头 Authorization 中提取 Bearer 格式的 Token
//比如Authorization: Bearer abc123xyz-token
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

        // 分页元信息：前端可据此渲染"共 42 篇 / 第 1 页 / 下一页"等 UI
        const int total   = repo.GetPublishedCount();
        const int total_pages = (total + limit - 1) / limit;  // 向上取整

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
        // 解析请求完整body为JSON对象
        body = json::parse(req.body);
    } catch (const std::exception&) {
        // JSON格式非法，返回400参数错误
        WriteJsonError(res, 400, "invalid JSON body");
        return;
    }

    // 校验必填字段title：必须存在、字符串类型、非空
    if (!body.contains("title") || !body["title"].is_string() || body["title"].empty()) {
        WriteJsonError(res, 400, "title is required");
        return;
    }
    // 校验唯一标识slug：必须存在、字符串类型、非空
    if (!body.contains("slug") || !body["slug"].is_string() || body["slug"].empty()) {
        WriteJsonError(res, 400, "slug is required");
        return;
    }
    // 校验Markdown正文content_md：必须存在、字符串类型、非空
    if (!body.contains("content_md") || !body["content_md"].is_string() || body["content_md"].empty()) {
        WriteJsonError(res, 400, "content_md is required");
        return;
    }

    // 取出前端传入的原始slug，校验数据库唯一性
    const std::string slug = body["slug"].get<std::string>();
    if (repo.IsSlugExists(slug)) {
        // slug已存在，返回409资源冲突
        WriteJsonError(res, 409, "slug already exists");
        return;
    }

    // 组装Post笔记实体
    Post post;
    // 必填标题
    post.title = body["title"].get<std::string>();
    // 唯一访问标识
    post.slug = slug;
    // 摘要为可选字段，无则赋空字符串
    post.summary = body.value("summary", "");
    // 原始Markdown正文
    post.content_md = body["content_md"].get<std::string>();
    // 预渲染HTML为可选字段，前端可自行传入，无则为空
    post.content_html = body.value("content_html", "");
    // 封面图片地址可选
    post.cover_url = body.value("cover_url", "");
    // 解析标签，默认传入空JSON数组"[]"交由工具函数处理
    post.tags = ParseTagsField(body.value("tags", "[]"));
    // 发布状态可选，不传默认false（草稿状态）
    post.is_published = body.value("is_published", false);

    try {
        // 写入数据库，获取笔记自增ID
        const int id = repo.create(post);
        // ID小于等于0代表入库失败
        if (id <= 0) {
            WriteJsonError(res, 409, "failed to create post");
            return;
        }

        // 组装成功返回JSON
        json data;
        data["id"] = id;
        data["slug"] = slug;
        data["message"] = "success";
        res.status = 201; // HTTP 201 Created 资源创建成功
        res.set_content(data.dump(), "application/json; charset=utf-8");
    } catch (const std::exception& e) {
        // 捕获数据库操作全部异常，返回500内部错误并携带异常详情
        WriteInternalError(res, "failed to create post", e);
    }
}

void AdminGetAllPosts(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    HandleGetAllPosts(repo, req, res);
}

void AdminPostImages(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    // 消除未使用repo参数的编译警告
    (void)repo;

    // 校验请求必须为multipart/form-data文件上传表单
    if (!req.is_multipart_form_data()) {
        WriteJsonError(res, 400, "multipart/form-data is required");
        return;
    }

    // 读取上传文件，优先取image字段，兼容旧前端file字段
    httplib::FormData file = GetUploadFile(req, "image", "file");
    // 校验文件二进制内容、原始文件名不能为空
    if (file.content.empty() || file.filename.empty()) {
        WriteJsonError(res, 400, "image file is required");
        return;
    }

    // 提取文件小写后缀，校验图片格式白名单
    const std::string ext = ExtensionOf(file.filename);
    if (!IsAllowedImageExtension(ext)) {
        WriteJsonError(res, 400, "unsupported image type");
        return;
    }

    // 限制单张图片最大5MB，超大文件返回413负载过大
    const size_t kMaxImageBytes = 5U * 1024U * 1024U;
    if (file.content.size() > kMaxImageBytes) {
        WriteJsonError(res, 413, "image is too large");
        return;
    }

    // 图片存储根目录
    const std::string dir = "uploads/images";
    // 递归创建多级存储目录，创建失败返回内部错误
    if (!EnsureDirectory(dir)) {
        WriteInternalError(res, "failed to prepare image directory", std::runtime_error("invalid directory"));
        return;
    }

    // 生成基础安全存储文件名：时间戳+文件名脱敏slug+小写后缀
    std::string filename = SafeStorageName(file.filename);
    std::string path = dir + "/" + filename;
    int suffix = 1;
    // 循环检测文件是否存在，存在则追加自增数字后缀，避免文件覆盖丢失
    while (FileExists(path)) {
        std::ostringstream renamed;
        renamed << std::time(nullptr) << "-" << suffix++ << "-" << Slugify(BaseNameOf(file.filename)) << ext;
        filename = renamed.str();
        path = dir + "/" + filename;
    }

    // 二进制写入图片到本地磁盘，写入失败返回500内部异常
    if (!SaveBytes(path, file.content)) {
        WriteInternalError(res, "failed to save image", std::runtime_error(path));
        return;
    }

    // 组装上传成功返回JSON数据
    json body;
    body["url"] = "/uploads/images/" + filename; // 前端可直接访问的图片相对路径
    body["filename"] = filename;                 // 磁盘真实存储文件名
    body["size"] = file.content.size();          // 文件字节大小
    body["content_type"] = file.content_type;    // 上传携带的MIME类型
    res.status = 201; // HTTP 201 Created：资源创建成功标准状态码
    res.set_content(body.dump(), "application/json; charset=utf-8");
}

void AdminPostMarkdown(PostRepo& repo, const httplib::Request& req, httplib::Response& res)
{
    // 校验请求必须为multipart/form-data文件上传表单
    if (!req.is_multipart_form_data()) {
        WriteJsonError(res, 400, "multipart/form-data is required");
        return;
    }

    // 读取上传md文件，优先取markdown字段，兼容旧前端file字段
    httplib::FormData file = GetUploadFile(req, "markdown", "file");
    // 校验文件二进制内容、原始文件名不能为空
    if (file.content.empty() || file.filename.empty()) {
        WriteJsonError(res, 400, "markdown file is required");
        return;
    }

    // 提取文件小写后缀，仅放行.md/.markdown格式
    const std::string ext = ExtensionOf(file.filename);
    if (ext != ".md" && ext != ".markdown") {
        WriteJsonError(res, 400, "only .md or .markdown files are supported");
        return;
    }

    // 限制单篇md笔记最大1MB，超大文件返回413负载过大
    const size_t kMaxMarkdownBytes = 1024U * 1024U;
    if (file.content.size() > kMaxMarkdownBytes) {
        WriteJsonError(res, 413, "markdown file is too large");
        return;
    }

    // 笔记标题三级兜底策略：前端传入 > md一级标题 > 原始文件名（去后缀）
    std::string title = req.form.has_field("title") ? Trim(req.form.get_field("title")) : "";
    if (title.empty()) {
        title = FirstHeadingTitle(file.content);
    }
    if (title.empty()) {
        title = BaseNameOf(file.filename);
    }

    // 笔记唯一访问标识slug二级兜底：前端自定义脱敏slug > 文件名自动生成slug
    std::string slug = req.form.has_field("slug") ? Slugify(req.form.get_field("slug")) : "";
    if (slug.empty()) {
        slug = Slugify(BaseNameOf(file.filename));
    }
    // 校验数据库内slug唯一，重复返回409资源冲突
    if (repo.IsSlugExists(slug)) {
        WriteJsonError(res, 409, "slug already exists");
        return;
    }

    // 笔记摘要二级兜底：前端传入 > md第一段前200字符预览文本
    std::string summary = req.form.has_field("summary") ? Trim(req.form.get_field("summary")) : "";
    if (summary.empty()) {
        summary = FirstParagraphSummary(file.content, 200);
    }

    // 解析标签字段，兼容JSON数组/逗号分隔字符串两种格式
    std::vector<std::string> tags;
    if (req.form.has_field("tags")) {
        tags = ParseTagsField(req.form.get_field("tags"));
    }

    // 笔记封面图地址，前端不传则为空
    const std::string cover_url = req.form.has_field("cover_url") ? Trim(req.form.get_field("cover_url")) : "";
    // 笔记发布状态，不传参默认true（直接发布）
    const bool is_published = req.form.has_field("is_published") ? IsTruthy(req.form.get_field("is_published")) : true;

    // 组装笔记数据实体
    Post post;
    post.title = title;
    post.slug = slug;
    post.summary = summary;
    post.content_md = file.content;               // 原始Markdown源码
    post.content_html = RenderMarkdownLite(file.content); // 预渲染安全HTML预览文本
    post.cover_url = cover_url;
    post.tags = tags;
    post.is_published = is_published;

    try {
        // 写入数据库，返回笔记自增主键ID
        const int id = repo.create(post);
        // 创建失败返回409错误
        if (id <= 0) {
            WriteJsonError(res, 409, "failed to create post");
            return;
        }

        // 本地磁盘备份md源文件，按slug命名便于导出/备份
        const std::string content_dir = "content/posts";
        if (EnsureDirectory(content_dir)) {
            SaveBytes(content_dir + "/" + slug + ".md", file.content);
        }

        // 组装导入成功返回JSON数据
        json body;
        body["id"] = id;
        body["slug"] = slug;
        body["title"] = title;
        body["summary"] = summary;
        body["url"] = "/api/posts/" + std::to_string(id); // 笔记详情接口地址
        body["is_published"] = is_published;
        res.status = 201; // HTTP 201 Created：资源创建成功标准状态码
        res.set_content(body.dump(), "application/json; charset=utf-8");
    } catch (const std::exception& e) {
        // 捕获入库/文件IO全部异常，统一返回500内部错误并携带异常信息
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

    // 路由正则已限定字符集，此处二次校验长度防超长输入
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

        // 阅读量 +1（返回的是递增前的值，少 1，与 GET /api/posts/{id} 行为一致）
        repo.incrementViews(post.id);
        res.set_content(post.to_json().dump(), "application/json; charset=utf-8");
    } catch (const std::exception& e) {
        WriteInternalError(res, "failed to get post by slug", e);
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
        // 当前阶段先直接展示；后续接后台审核时只需把默认值改为 false。
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
