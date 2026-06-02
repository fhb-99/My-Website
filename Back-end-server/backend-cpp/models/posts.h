#pragma once

#include "global.h"
#include <string>
#include <vector>
#include "third_party/json.hpp"

struct Post {
    int id = 0;                     // 主键，自增，唯一标识一篇文章
    std::string title;              // 文章标题（必填）
    std::string slug;               // URL 友好标识，如 "hello-world"
    std::string summary;            // 文章摘要/简介，列表页展示用
    std::string content_md;         // Markdown 原始正文，存编辑时内容
    std::string content_html;       // Markdown 渲染后的 HTML，直接返回给前端
    std::string cover_url;          // 封面图路径，如 "/uploads/cover-123.jpg"
    std::vector<std::string> tags;  // 标签列表，如 ["C++", "后端"]
    bool is_published = false;      // false=草稿仅自己可见，true=已发布可公开
    int views = 0;                  // 阅读量计数，每次查看详情时 +1
    std::string created_at;         // 创建时间
    std::string updated_at;         // 最后更新时间

    // to_json() — 详情页用，返回所有字段（含 content_html 正文）
    nlohmann::json to_json() const {
        nlohmann::json root;
        root["id"] = id;
        root["title"] = title;
        root["slug"] = slug;
        root["summary"] = summary;
        root["content_html"] = content_html;
        root["cover_url"] = cover_url;
        root["tags"] = tags;
        root["views"] = views;
        root["created_at"] = created_at;
        root["updated_at"] = updated_at;
        return root;
    }

    // to_json_summary() — 列表页用，不含 content_html，省带宽
    nlohmann::json to_json_summary() const {
        return {
            {"id", id},
            {"title", title},
            {"slug", slug},
            {"summary", summary},
            {"cover_url", cover_url},
            {"tags", tags},
            {"views", views},
            {"created_at", created_at}
        };
    }
};