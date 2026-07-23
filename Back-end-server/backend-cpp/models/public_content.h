#pragma once

#include "models/posts.h"

#include <string>
#include <vector>

struct PublicPostQuery {
    int page = 1;
    int limit = 10;
    std::string keyword;
    std::string tag;
};

struct TagSummary {
    std::string name;
    int post_count = 0;
};

struct Note {
    int id = 0;
    std::string content;
    std::string mood;
    std::string created_at;

    nlohmann::json to_json_public() const
    {
        return {
            {"id", id},
            {"content", content},
            {"mood", mood},
            {"created_at", created_at}
        };
    }
};

struct Project {
    int id = 0;
    std::string name;
    std::string summary;
    std::string url;
    std::vector<std::string> tags;
    int sort_order = 0;
    std::string created_at;

    nlohmann::json to_json_public() const
    {
        return {
            {"id", id},
            {"name", name},
            {"summary", summary},
            {"url", url},
            {"tags", tags},
            {"created_at", created_at}
        };
    }
};

struct SiteConfig {
    std::string title;
    std::string subtitle;
    std::string announcement;

    nlohmann::json to_json_public() const
    {
        return {
            {"title", title},
            {"subtitle", subtitle},
            {"announcement", announcement}
        };
    }
};

struct PostNavigation {
    bool post_found = false;
    bool has_previous = false;
    Post previous;
    bool has_next = false;
    Post next;
};
