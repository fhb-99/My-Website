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
    // 仅管理端读取发布状态，公开序列化时不会暴露该字段。
    bool is_published = false;
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
    // 项目草稿只在管理端出现，公开项目接口仍只查询已发布数据。
    bool is_published = false;
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

struct MusicTrack {
    int id = 0;
    std::string title;
    std::string artist;
    std::string cover_url;
    std::string audio_url;
    int sort_order = 0;
    bool is_enabled = true;

    nlohmann::json to_json_public() const
    {
        return {
            {"id", id},
            {"title", title},
            {"artist", artist},
            {"cover_url", cover_url},
            {"audio_url", audio_url},
            {"sort_order", sort_order}
        };
    }

    nlohmann::json to_json_admin() const
    {
        nlohmann::json body = to_json_public();
        body["is_enabled"] = is_enabled;
        return body;
    }
};

// 音乐配置仅保存播放器所需的最少字段，音频文件继续由 audio_url 的静态资源提供。
struct MusicConfig {
    bool enabled = false;
    double volume = 0.35;
    std::vector<MusicTrack> tracks;

    nlohmann::json to_json_public() const
    {
        nlohmann::json data = nlohmann::json::array();
        for (const MusicTrack& track : tracks) {
            // 公开接口只返回启用曲目，避免管理员暂存的歌曲被用户端播放。
            if (track.is_enabled) {
                data.push_back(track.to_json_public());
            }
        }
        return {
            {"enabled", enabled},
            {"volume", volume},
            {"tracks", data}
        };
    }

    nlohmann::json to_json_admin() const
    {
        nlohmann::json data = nlohmann::json::array();
        for (const MusicTrack& track : tracks) {
            data.push_back(track.to_json_admin());
        }
        return {
            {"enabled", enabled},
            {"volume", volume},
            {"tracks", data}
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
