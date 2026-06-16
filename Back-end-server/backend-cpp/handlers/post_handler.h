#pragma once

#include "third_party/httplib.h"
#include "repo/post_repo.h"
#include "models/global.h"

// GET /api/posts?page=1&limit=10
void HandleGetAllPosts(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// GET /api/posts/{id}
void HandleGetPostByID(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// GET /api/auth/login
void HandleLogin(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// POST /api/posts
void HandlerCreatePost(PostRepo& repo, const httplib::Request& req, httplib::Response& res);
