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

// POST /api/posts/{id}/view
void HandleRecordPostView(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// POST /api/auth/login
void HandleLogin(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// POST /api/admin/posts
void HandlerCreatePost(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// GET /api/admin/posts
void AdminGetAllPosts(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// GET /api/admin/posts/{id}
void AdminGetPostByID(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// PUT /api/admin/posts/{id}
void AdminUpdatePost(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// DELETE /api/admin/posts/{id}
void AdminDeletePost(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// POST /api/admin/uploads/images
void AdminPostImages(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// POST /api/admin/uploads/markdown
void AdminPostMarkdown(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// GET /api/posts/{id}/comments?page=1&limit=20
void HandleGetPostComments(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// POST /api/posts/{id}/comments
void HandleCreatePostComment(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// GET /api/search?q={keyword}&limit=10
void HandleSearchPosts(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// GET /api/guestbook?page=1&limit=10
void HandleGetGuestbook(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// POST /api/guestbook
void HandleCreateGuestbook(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// GET /api/admin/comments
void AdminGetComments(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// PUT /api/admin/comments/{id}/approve
void AdminApproveComment(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// PUT /api/admin/comments/{id}/reject
void AdminRejectComment(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// DELETE /api/admin/comments/{id}
void AdminDeleteComment(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// GET /api/admin/guestbook
void AdminGetGuestbook(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// PUT /api/admin/guestbook/{id}/approve
void AdminApproveGuestbook(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// PUT /api/admin/guestbook/{id}/reject
void AdminRejectGuestbook(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// DELETE /api/admin/guestbook/{id}
void AdminDeleteGuestbook(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// GET /api/admin/moderation/config
void AdminGetModerationConfig(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// PUT /api/admin/moderation/config
void AdminUpdateModerationConfig(PostRepo& repo, const httplib::Request& req, httplib::Response& res);


