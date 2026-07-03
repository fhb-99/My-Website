export interface PageResult<T> { data: T[]; page: number; limit: number; total: number; total_pages: number; has_more: boolean }
export interface PostSummary { id: number; title: string; slug: string; summary: string; cover_url?: string; tags: string[]; views: number; created_at: string }
export interface PostDetail extends PostSummary { content_html: string; updated_at: string }
export interface AdminPostSummary extends PostSummary { is_published: boolean; updated_at: string }
export interface AdminPostDetail extends AdminPostSummary { content_md: string; content_html: string }
export interface Comment { id: number; post_id: number; nickname: string; content: string; created_at: string }
export interface CommentPayload { nickname: string; email: string; content: string }
export interface GuestbookMessage { id: number; nickname: string; content: string; created_at: string }
export interface GuestbookPayload { nickname: string; email: string; content: string }
export type ModerationStatus = 'pending' | 'approved' | 'rejected'
export type ModerationStrictness = 'loose' | 'normal' | 'strict'
export interface AdminComment extends Comment { email?: string; is_approved: boolean; status: ModerationStatus; post_title?: string; post_slug?: string; updated_at?: string }
export interface AdminGuestbookMessage extends GuestbookMessage { email?: string; is_approved: boolean; status: ModerationStatus; updated_at?: string }
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
export interface NoteItem { id: number; content: string; mood?: string; created_at: string }
export interface ProjectItem { id: number; name: string; summary: string; url?: string; tags: string[] }
export interface SiteConfig { title: string; subtitle: string; announcement?: string }
export interface AdminUser { id: number; username: string; role: 'admin'; display_name?: string }
export interface UploadAsset { url: string; filename: string; size: number; content_type?: string }
export interface LoginPayload { username: string; password: string }
export interface LoginResult { token: string; token_type: 'Bearer'; expires_at: string }
export interface AdminPostDraft { id?: number; title: string; slug: string; summary: string; content_md: string; cover_url?: string; tags: string[]; is_published: boolean }
