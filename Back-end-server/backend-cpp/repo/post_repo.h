#pragma once

#include <string>
#include <vector>
#include "models/posts.h"
#include "models/public_content.h"
#include "models/global.h"

class PostRepo
{
public:
    virtual ~PostRepo() = default;
    PostRepo() = default;

    virtual std::vector<Post> GetAll(int page, int limit) = 0;

    // 后台文章列表需要包含草稿和未发布文章，不能复用公开列表过滤逻辑
    virtual std::vector<Post> GetAllForAdmin(int page, int limit) = 0;

    // 获取已发布文章总数，用于分页元信息（total_pages / has_more）
    virtual int GetPublishedCount() = 0;

    virtual std::vector<Post> ListPublishedPosts(const PublicPostQuery& query) = 0;

    virtual int CountPublishedPosts(const PublicPostQuery& query) = 0;

    virtual std::vector<TagSummary> ListPublishedTags() = 0;

    virtual PostNavigation GetPublishedNavigation(int id) = 0;

    virtual SiteConfig GetPublicSiteConfig() = 0;

    virtual std::vector<Note> ListPublishedNotes(int page, int limit) = 0;

    virtual int CountPublishedNotes() = 0;

    // 管理端随记 CRUD 需要包含草稿，和公开查询分开避免意外暴露未发布内容。
    virtual std::vector<Note> ListNotesForAdmin(int page, int limit) = 0;
    virtual int CountNotesForAdmin() = 0;
    virtual int CreateNote(const Note& note) = 0;
    virtual bool UpdateNote(int id, const Note& note) = 0;
    virtual bool DeleteNote(int id) = 0;

    virtual std::vector<Project> ListPublishedProjects(int page, int limit) = 0;

    virtual int CountPublishedProjects() = 0;

    // 管理端项目 CRUD 同样读取全部状态，便于编辑后再决定是否公开。
    virtual std::vector<Project> ListProjectsForAdmin(int page, int limit) = 0;
    virtual int CountProjectsForAdmin() = 0;
    virtual int CreateProject(const Project& project) = 0;
    virtual bool UpdateProject(int id, const Project& project) = 0;
    virtual bool DeleteProject(int id) = 0;

    // 只保存已经明确用于公开展示的站点配置键。
    virtual void SaveSiteSetting(const std::string& key, const std::string& value) = 0;

    // 音乐设置复用站点配置表存储为一份 JSON，公开读取时会过滤未启用曲目。
    virtual MusicConfig GetPublicMusicConfig() = 0;
    virtual MusicConfig GetAdminMusicConfig() = 0;
    virtual void SaveMusicConfig(const MusicConfig& config) = 0;

    // 获取后台文章总数，包含草稿和未发布文章
    virtual int GetAdminPostCount() = 0;

    // C++11 compatibility: use an output flag instead of std::optional.
    virtual Post GetByID(int id, bool& ok) = 0;

    // 后台按 ID 读取文章，包含草稿并返回 Markdown 原文
    virtual Post GetByIDForAdmin(int id, bool& ok) = 0;

    virtual int create(const Post& post) = 0;

    virtual bool update(int id, const Post& post) = 0;

    virtual bool remove(int id) = 0;

    virtual bool incrementViews(int id, const std::string& visitor_id) = 0;

    virtual std::vector<Post> search(const std::string& keyword, int limit) = 0;

    //通过slug来判断文章是否存在
    virtual bool IsSlugExists(const std::string& slug) = 0;

    virtual Post GetBySlug(const std::string& slug, bool& ok) = 0;

    //管理员登录校验
    virtual User GetUserByUsername(const std::string& name, bool& flag) = 0;

    //登录成功后，拿到token，保存其哈希值，原始token返回给前端
    virtual std::string CreateAdminSession(int user_id, const std::string& token_hash, 
                                        int ttl_hours, const std::string& user_agent) = 0;

    //后端对前端请求传来的原始token进行鉴权
    virtual bool IsAdminSessionValid(const std::string& token_hash) = 0;

    // 获取某篇文章已发布文章下已通过审核的评论
    virtual std::vector<Comment> GetCommentsByPostID(int post_id, int page, int limit) = 0;

    // 获取评论总数,用于前端分页展示
    virtual int GetApprovedCommentCount(int post_id) = 0;

    // 创建文章评论，不返回邮箱
    virtual int createComment(const Comment& comment) = 0;

    // 管理端评论列表：post_id 为 0 时查询全部文章评论。
    virtual std::vector<Comment> GetCommentsForAdmin(int page, int limit, int post_id) = 0;

    // 管理端评论总数：post_id 为 0 时统计全部文章评论。
    virtual int GetAdminCommentCount(int post_id) = 0;

    // 管理端审核评论：approved=true 为通过，false 为拒绝展示。
    virtual bool SetCommentApproved(int id, bool approved) = 0;

    // 管理端删除评论。
    virtual bool DeleteComment(int id) = 0;

    // 获取留言列表
    virtual std::vector<Guestbook> GetGuestbook(int page, int limit) = 0;

    // 创建留言
    virtual int createGuestbook(const Guestbook& guestbook) = 0;

    // 获取留言总数,用于前端分页展示
    virtual int GetGuestbookCount() = 0;

    // 管理端留言列表：包含已通过和已拒绝的留言。
    virtual std::vector<Guestbook> GetGuestbookForAdmin(int page, int limit) = 0;

    // 管理端留言总数。
    virtual int GetAdminGuestbookCount() = 0;
 
    // 管理端审核留言：approved=true 为通过，false 为拒绝展示。
    virtual bool SetGuestbookApproved(int id, bool approved) = 0;
 
    // 管理端删除留言。
    virtual bool DeleteGuestbook(int id) = 0;

    // 获取审核配置
    virtual ModerationConfig GetModerationConfig() = 0;

    // 设置审核
    virtual void SaveModerationConfig(const ModerationConfig& config) = 0;

    // 获取审核日志
    virtual std::vector<ModerationLog> GetModerationLogs(int page, int limit) = 0;

    virtual int GetModerationLogCount() = 0;

    // 创建日志
    virtual void CreateModerationLog(const ModerationLog& log) = 0;

    virtual Comment GetCommentForAdminByID(int id) = 0;

    virtual Guestbook GetGuestbookForAdminByID(int id) = 0;
};
