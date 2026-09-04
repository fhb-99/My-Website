#pragma once

#include "third_party/httplib.h"
#include "repo/post_repo.h"

// 管理端碎碎念 CRUD。
void AdminGetNotes(PostRepo& repo, const httplib::Request& req, httplib::Response& res);
void AdminCreateNote(PostRepo& repo, const httplib::Request& req, httplib::Response& res);
void AdminUpdateNote(PostRepo& repo, const httplib::Request& req, httplib::Response& res);
void AdminDeleteNote(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// 管理端项目 CRUD。
void AdminGetProjects(PostRepo& repo, const httplib::Request& req, httplib::Response& res);
void AdminCreateProject(PostRepo& repo, const httplib::Request& req, httplib::Response& res);
void AdminUpdateProject(PostRepo& repo, const httplib::Request& req, httplib::Response& res);
void AdminDeleteProject(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// 管理端站点公开配置读取与单项保存。
void AdminGetSiteConfig(PostRepo& repo, const httplib::Request& req, httplib::Response& res);
void AdminUpdateSiteConfig(PostRepo& repo, const httplib::Request& req, httplib::Response& res);

// 管理端背景音乐配置读取与保存。
void AdminGetMusicConfig(PostRepo& repo, const httplib::Request& req, httplib::Response& res);
void AdminUpdateMusicConfig(PostRepo& repo, const httplib::Request& req, httplib::Response& res);
