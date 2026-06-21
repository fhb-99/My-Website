#pragma once

#include "third_party/httplib.h"
#include "repo/post_repo.h"
#include "models/global.h"

// 后台接口鉴权函数 所有/api/admin/*都必须检查token
bool RequireAdmin(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// GET /api/posts?page=1&limit=10
void HandleGetAllPosts(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// GET /api/posts/{id}
void HandleGetPostByID(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// GET /api/posts/slug/{slug}
void HandleGetPostBySlug(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// GET /api/auth/login
void HandleLogin(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// POST /api/admin/posts
void HandlerCreatePost(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// GET /api/admin/posts
void AdminGetAllPosts(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// POST /api/admin/uploads/images
void AdminPostImages(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// POST /api/admin/uploads/markdown
void AdminPostMarkdown(PostRepo& repo, const httplib::Request& req, httplib::Response& res);