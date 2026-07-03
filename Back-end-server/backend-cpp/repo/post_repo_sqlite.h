#pragma once

#include "repo/post_repo.h"
#include "SQLiteCpp/SQLiteCpp.h"
#include "models/global.h"

class PostRepoSqlite : public PostRepo
{
public:
    explicit PostRepoSqlite(SQLite::Database& db) : m_db(&db) {}
    ~PostRepoSqlite() noexcept override = default;

    std::vector<Post> GetAll(int page, int limit) override;

    std::vector<Post> GetAllForAdmin(int page, int limit) override;

    // 获取已发布文章总数，用于分页元信息
    int GetPublishedCount() override;

    int GetAdminPostCount() override;

    Post GetByID(int id, bool& ok) override;

    Post GetByIDForAdmin(int id, bool& ok) override;

    int create(const Post& post) override;

    bool update(int id, const Post& post) override;

    bool remove(int id) override;

    bool incrementViews(int id, const std::string& visitor_id) override;

    std::vector<Post> search(const std::string& keyword, int limit) override;

    //通过slug来判断文章是否存在
    bool IsSlugExists(const std::string& slug) override;

    Post GetBySlug(const std::string& slug, bool& ok) override;

    //管理员登录校验
    User GetUserByUsername(const std::string& name, bool& flag) override;

    //登录成功后，拿到token，保存其哈希值，原始token返回给前端
    std::string CreateAdminSession(int user_id, const std::string& token_hash, 
            int ttl_hours, const std::string& user_agent) override;

    //后端对前端请求传来的原始token进行鉴权
    bool IsAdminSessionValid(const std::string& token_hash) override;

    // 获取某篇文章已发布文章下已通过审核的评论
    std::vector<Comment> GetCommentsByPostID(int post_id, int page, int limit) override;

    // 获取评论总数,用于前端分页展示
    int GetApprovedCommentCount(int post_id) override;

    // 创建文章评论，不返回邮箱
    int createComment(const Comment& comment) override;

    // 管理端评论列表：post_id 为 0 时查询全部文章评论。
    std::vector<Comment> GetCommentsForAdmin(int page, int limit, int post_id) override;

    // 管理端评论总数：post_id 为 0 时统计全部文章评论。
    int GetAdminCommentCount(int post_id) override;

    // 管理端审核评论：approved=true 为通过，false 为拒绝展示。
    bool SetCommentApproved(int id, bool approved) override;

    // 管理端删除评论。
    bool DeleteComment(int id) override;

    // 获取留言列表
    std::vector<Guestbook> GetGuestbook(int page, int limit) override;

    // 创建留言
    int createGuestbook(const Guestbook& guestbook) override;

    // 获取留言总数,用于前端分页展示
    int GetGuestbookCount() override;

    // 管理端留言列表：包含已通过和已拒绝的留言。
    std::vector<Guestbook> GetGuestbookForAdmin(int page, int limit) override;

    // 管理端留言总数。
    int GetAdminGuestbookCount() override;

    // 管理端审核留言：approved=true 为通过，false 为拒绝展示。
    bool SetGuestbookApproved(int id, bool approved) override;

    // 管理端删除留言。
    bool DeleteGuestbook(int id) override;

    // 获取审核配置
    ModerationConfig GetModerationConfig() override;

    // 设置审核
    void SaveModerationConfig(const ModerationConfig& config) override;

    // 获取审核日志
    std::vector<ModerationLog> GetModerationLogs(int page, int limit) override;

    int GetModerationLogCount() override;

    // 创建日志
    void CreateModerationLog(const ModerationLog& log) override;

    Comment GetCommentForAdminByID(int id) override;

    Guestbook GetGuestbookForAdminByID(int id) override;
private:
    SQLite::Database* m_db;
};
