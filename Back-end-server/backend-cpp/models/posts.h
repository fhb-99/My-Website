#pragma once

#include "global.h"
#include <string>
#include <vector>
#include "third_party/json.hpp"

struct Post {
    int id = 0;
    std::string title;
    std::string slug;
    std::string summary;
    std::string content_md;
    std::string content_html;
    std::string cover_url;
    std::vector<std::string> tags;
    bool is_published = false;
    int views = 0;
    std::string created_at;
    std::string updated_at;

    // Detail response for a single post. Keep Markdown private to the API.
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

    // List response without the full article body.
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
