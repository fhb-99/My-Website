#pragma once

#include "third_party/httplib.h"
#include "repo/post_repo.h"

// 公开文章列表，支持关键词、标签与分页筛选。
void HandleListPublicPosts(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// 公开展示配置、随记、项目、标签和相邻文章接口。
void HandleGetPublicConfig(PostRepo& repo, const httplib::Request& req, httplib::Response& res);
void HandleGetNotes(PostRepo& repo, const httplib::Request& req, httplib::Response& res);
void HandleGetProjects(PostRepo& repo, const httplib::Request& req, httplib::Response& res);
void HandleGetTags(PostRepo& repo, const httplib::Request& req, httplib::Response& res);
void HandleGetPostNavigation(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// 保留旧搜索路径的响应结构，同时使用新的公开查询逻辑。
void HandleSearchPublicPosts(PostRepo& repo, const httplib::Request& req, httplib::Response& res);
