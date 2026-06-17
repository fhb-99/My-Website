#pragma once

#include "third_party/httplib.h"
#include "repo/post_repo.h"
#include "models/global.h"

// GET /api/posts?page=1&limit=10
void HandleGetAllPosts(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// GET /api/posts/{id}
void HandleGetPostByID(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// POST /api/auth/login
void HandleLogin(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// 管理接口统一鉴权，所有 /api/admin/* 路由进入业务 handler 前都应该先调用。
bool RequireAdmin(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// POST /api/posts
void HandlerCreatePost(PostRepo& repo, const httplib::Request& req, httplib::Response& res);
