#pragma once

#include <string>
#include "third_party/json.hpp"

struct User
{
    int id;
    std::string username;
    std::string password_hash;
    std::string password_salt;
    std::string password_algo;
    int password_iterations;
    std::string role;
    bool is_active;

    User()
        : id(0),
          password_iterations(0),
          is_active(false)
    {
    }
};


struct Comment
{
    int id = 0;
    int post_id = 0;
    std::string nickname;
    std::string email;
    std::string content;
    bool is_approved = true;
    std::string status;
    std::string post_title;
    std::string post_slug;
    std::string created_at;
    std::string updated_at;

    // 公开接口不返回邮箱，避免访客隐私泄露。
    nlohmann::json to_json_public() const
    {
        return {
            {"id", id},
            {"post_id", post_id},
            {"nickname", nickname},
            {"content", content},
            {"created_at", created_at}
        };
    }

    nlohmann::json to_json_admin() const
    {
        return {
            {"id", id},
            {"post_id", post_id},
            {"nickname", nickname},
            {"email", email},
            {"content", content},
            {"is_approved", is_approved},
            {"status", is_approved ? "approved" : "rejected"},
            {"post_title", post_title},
            {"post_slug", post_slug},
            {"created_at", created_at},
            {"updated_at", updated_at}
        };
    }
};


struct Guestbook
{
    int id = 0;
    std::string nickname;
    std::string email;
    std::string content;
    bool is_approved = true;
    std::string status;
    std::string created_at;
    std::string updated_at; 

    nlohmann::json to_json_public() const
    {
        return {
            {"id", id},
            {"nickname", nickname},
            {"content", content},
            {"created_at", created_at}
        };
    }

    nlohmann::json to_json_admin() const 
    {
        return {
            {"id", id},
            {"nickname", nickname},
            {"email", email},
            {"content", content},
            {"is_approved", is_approved},
            {"status", is_approved ? "approved" : "rejected"},
            {"created_at", created_at},
            {"updated_at", updated_at}
        };
    }
};
