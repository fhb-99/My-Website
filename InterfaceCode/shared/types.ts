export type EntityId = string | number
export type PostContentType = 'article' | 'interview'
export const DEFAULT_INTERVIEW_CATEGORIES = ['C++ 基础语法', 'C++ 进阶', 'Linux', '计算机网络', 'Qt', '音视频'] as const
export interface PageResult<T> { data: T[]; page: number; limit: number; total: number; total_pages: number; has_more: boolean }
export interface PostSummary { id: EntityId; title: string; slug: string; summary: string; cover_url?: string; tags: string[]; views: number; created_at: string; content_type?: PostContentType; interview_category?: string }
export interface InterviewDirectory { name: string; post_count: number; posts: PostSummary[] }
export interface InterviewCategory { id: EntityId; name: string; sort_order: number }
export interface LearningRoadmap { id: EntityId; title: string; subtitle: string; cover_url: string; content_html: string; updated_at: string }
export interface AdminLearningRoadmapDraft { id?: EntityId; title: string; subtitle: string; content_md: string; cover_url: string; is_published: boolean }
export interface PostDetail extends PostSummary { content_html: string; updated_at: string }
export interface AdminPostSummary extends Omit<PostSummary, 'id'> { id: EntityId; is_published: boolean; updated_at: string }
export interface AdminPostDetail extends AdminPostSummary { content_md: string; content_html: string }
export interface Comment { id: EntityId; post_id: EntityId; nickname: string; content: string; created_at: string }
export interface CommentPayload { nickname: string; email: string; content: string }
export interface GuestbookMessage { id: EntityId; nickname: string; content: string; created_at: string }
export interface GuestbookPayload { nickname: string; email: string; content: string }
export type ModerationStatus = 'pending' | 'approved' | 'rejected'
export type ModerationStrictness = 'loose' | 'normal' | 'strict'
export interface AdminComment extends Omit<Comment, 'id' | 'post_id'> { id: EntityId; post_id: EntityId; email?: string; is_approved: boolean; status: ModerationStatus; post_title?: string; post_slug?: string; updated_at?: string }
export interface AdminGuestbookMessage extends Omit<GuestbookMessage, 'id'> { id: EntityId; email?: string; is_approved: boolean; status: ModerationStatus; updated_at?: string }
export interface ModerationConfig {
  agent_enabled: boolean
  provider?: string
  api_base_url?: string
  model?: string
  blocked_words: string[]
  strictness: ModerationStrictness
  max_links: number
  confidence_threshold?: number
  system_prompt?: string
  auto_reject_enabled: boolean
  auto_approve_enabled: boolean
}
export interface ModerationLog {
  id: number
  target_type: 'comment' | 'guestbook'
  target_id: number
  decision: ModerationStatus
  source: 'rule' | 'ai' | 'admin'
  reason: string
  confidence?: number
  created_at: string
}
export interface ModerationTestPayload { content: string; target_type: 'comment' | 'guestbook' }
export interface ModerationTestResult { decision: ModerationStatus; reason: string; confidence?: number }
export interface NoteItem { id: EntityId; content: string; mood?: string; created_at: string }
export interface ProjectItem { id: EntityId; name: string; summary: string; url?: string; tags: string[]; created_at: string }
export interface SiteConfig { title: string; subtitle: string; announcement?: string }
/** 用户端音乐播放所需的公开歌单字段，音频文件本身仍由 audio_url 指向的静态资源提供。 */
export interface MusicTrack { id: EntityId; title: string; artist?: string; cover_url?: string; audio_url: string; sort_order: number }
export interface MusicConfig { enabled: boolean; volume: number; tracks: MusicTrack[] }
/** 管理端编辑歌单时允许新曲目尚未拥有数据库 ID。 */
export interface AdminMusicTrack { id?: EntityId; title: string; artist: string; cover_url: string; audio_url: string; sort_order: number; is_enabled: boolean }
export interface AdminMusicConfig { enabled: boolean; volume: number; tracks: AdminMusicTrack[] }
export interface AdminNote extends Omit<NoteItem, 'id'> { id: EntityId; is_published: boolean }
export interface AdminNoteDraft { id?: EntityId; content: string; mood: string; is_published: boolean }
export interface AdminProject extends Omit<ProjectItem, 'id'> { id: EntityId; sort_order: number; is_published: boolean }
export interface AdminProjectDraft { id?: EntityId; name: string; summary: string; url: string; tags: string[]; sort_order: number; is_published: boolean }
export interface TagSummary { name: string; post_count: number }
export interface PostNavigation { previous: PostSummary | null; next: PostSummary | null }
export interface PublicSubmissionResult { id: EntityId; message?: string; status: ModerationStatus; is_approved: boolean }
export interface AdminUser { id: number; username: string; role: 'admin'; display_name?: string }
export interface UploadAsset { url: string; filename: string; size: number; content_type?: string }
export interface LoginPayload { username: string; password: string }
export interface LoginResult { token: string; token_type: 'Bearer'; expires_at: string }
export interface AdminPostDraft { id?: EntityId; title: string; slug: string; summary: string; content_md: string; cover_url?: string; tags: string[]; is_published: boolean; content_type: PostContentType; interview_category: string }
