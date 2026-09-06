import type { RequestOptions } from '@shared/api'
import type {
  AdminComment,
  AdminGuestbookMessage,
  AdminLearningRoadmapDraft,
  AdminMusicConfig,
  AdminMusicTrack,
  AdminNote,
  AdminNoteDraft,
  AdminPostDetail,
  AdminPostDraft,
  AdminPostSummary,
  AdminProject,
  AdminProjectDraft,
  EntityId,
  LoginPayload,
  LoginResult,
  ModerationConfig,
  ModerationLog,
  ModerationTestPayload,
  ModerationTestResult,
  PageResult,
  PostContentType,
  InterviewCategory,
  SiteConfig,
  UploadAsset,
} from '@shared/types'

const TOKEN_KEY = 'blog-admin-token'
const BASE_URL_KEY = 'blog-api-base'

type PocketBaseRecord = Record<string, unknown> & {
  id: string
  collectionId?: string
  collectionName?: string
  created: string
  updated: string
  expand?: Record<string, PocketBaseRecord | PocketBaseRecord[]>
}

type PocketBasePage = {
  page: number
  perPage: number
  totalItems: number
  totalPages: number
  items: PocketBaseRecord[]
}

export class AdminApiError extends Error {
  constructor(public status: number, message: string) {
    super(message)
    this.name = 'AdminApiError'
  }
}

function configuredBaseUrl() {
  const envUrl = (import.meta.env.VITE_POCKETBASE_URL || import.meta.env.VITE_API_BASE_URL) as string | undefined
  return (envUrl?.trim() || localStorage.getItem(BASE_URL_KEY) || 'http://127.0.0.1:8090').replace(/\/+$/, '')
}

function buildQuery(query?: RequestOptions['query']) {
  const params = new URLSearchParams()
  Object.entries(query || {}).forEach(([key, value]) => {
    if (value !== undefined) params.set(key, String(value))
  })
  const value = params.toString()
  return value ? `?${value}` : ''
}

function errorMessage(data: unknown, fallback: string) {
  if (!data || typeof data !== 'object') return fallback
  const response = data as { message?: string; data?: Record<string, { message?: string }> }
  const details = Object.values(response.data || {}).map((item) => item.message).filter(Boolean)
  return details.length ? details.join('；') : response.message || fallback
}

export const adminApiClient = {
  config: {
    baseUrl: configuredBaseUrl(),
    token: localStorage.getItem(TOKEN_KEY) || undefined,
  },

  setBaseUrl(baseUrl: string) {
    this.config.baseUrl = baseUrl.replace(/\/+$/, '')
    localStorage.setItem(BASE_URL_KEY, this.config.baseUrl)
  },

  setToken(token?: string) {
    this.config.token = token
    if (token) localStorage.setItem(TOKEN_KEY, token)
    else localStorage.removeItem(TOKEN_KEY)
  },

  async request<T>(path: string, options: RequestOptions = {}): Promise<T> {
    const headers = new Headers(options.headers)
    if (options.body !== undefined) headers.set('Content-Type', 'application/json')
    if (options.auth !== false && this.config.token) headers.set('Authorization', this.config.token)

    const response = await fetch(`${this.config.baseUrl}${path}${buildQuery(options.query)}`, {
      method: options.method || 'GET',
      headers,
      body: options.body === undefined ? undefined : JSON.stringify(options.body),
    })
    const data = await response.json().catch(() => null)
    if (!response.ok) throw new AdminApiError(response.status, errorMessage(data, 'PocketBase 请求失败'))
    return data as T
  },

  async upload<T>(path: string, form: FormData, method: 'POST' | 'PATCH' = 'POST'): Promise<T> {
    const headers = new Headers()
    if (this.config.token) headers.set('Authorization', this.config.token)
    const response = await fetch(`${this.config.baseUrl}${path}`, { method, headers, body: form })
    const data = await response.json().catch(() => null)
    if (!response.ok) throw new AdminApiError(response.status, errorMessage(data, 'PocketBase 文件上传失败'))
    return data as T
  },
}

function collectionPath(collection: string, suffix = '') {
  return `/api/collections/${encodeURIComponent(collection)}/records${suffix}`
}

function listRecords(
  collection: string,
  page = 1,
  perPage = 50,
  options: { filter?: string; sort?: string; expand?: string } = {},
) {
  return adminApiClient.request<PocketBasePage>(collectionPath(collection), {
    query: { page, perPage, filter: options.filter, sort: options.sort, expand: options.expand },
  })
}

async function getAllRecords(collection: string, options: { filter?: string; sort?: string; expand?: string } = {}) {
  const items: PocketBaseRecord[] = []
  let page = 1
  while (true) {
    const result = await listRecords(collection, page, 200, options)
    items.push(...result.items)
    if (!result.items.length || page >= result.totalPages) return items
    page += 1
  }
}

function getRecord(collection: string, id: EntityId, expand?: string) {
  return adminApiClient.request<PocketBaseRecord>(collectionPath(collection, `/${encodeURIComponent(String(id))}`), {
    query: { expand },
  })
}

function createRecord(collection: string, body: Record<string, unknown>) {
  return adminApiClient.request<PocketBaseRecord>(collectionPath(collection), { method: 'POST', body })
}

function updateRecord(collection: string, id: EntityId, body: Record<string, unknown>) {
  return adminApiClient.request<PocketBaseRecord>(collectionPath(collection, `/${encodeURIComponent(String(id))}`), {
    method: 'PATCH',
    body,
  })
}

function deleteRecord(collection: string, id: EntityId) {
  return adminApiClient.request<void>(collectionPath(collection, `/${encodeURIComponent(String(id))}`), { method: 'DELETE' })
}

function expandedRecords(record: PocketBaseRecord, field: string) {
  const value = record.expand?.[field]
  if (!value) return []
  return Array.isArray(value) ? value : [value]
}

function tagNames(record: PocketBaseRecord) {
  return expandedRecords(record, 'tags').map((tag) => String(tag.name || '')).filter(Boolean)
}

function fileUrl(record: PocketBaseRecord, field: string) {
  const filename = String(record[field] || '')
  const collection = record.collectionId || record.collectionName || ''
  return filename && collection
    ? `${adminApiClient.config.baseUrl}/api/files/${encodeURIComponent(collection)}/${encodeURIComponent(record.id)}/${encodeURIComponent(filename)}`
    : ''
}

function mapPage<T>(result: PocketBasePage, items: T[]): PageResult<T> {
  return {
    data: items,
    page: result.page,
    limit: result.perPage,
    total: result.totalItems,
    total_pages: result.totalPages,
    has_more: result.page < result.totalPages,
  }
}

function slugify(value: string) {
  const result = value.trim().toLowerCase()
    .replace(/\s+/g, '-')
    .replace(/[^\p{L}\p{N}_-]+/gu, '')
    .replace(/-+/g, '-')
    .replace(/^-|-$/g, '')
  return result || `item-${Date.now()}`
}

async function ensureTagIds(names: string[]) {
  // 管理表单输入的是标签名，PocketBase 关系字段实际保存标签记录 ID。
  const wanted = [...new Set(names.map((name) => name.trim()).filter(Boolean))]
  if (!wanted.length) return []
  const existing = await getAllRecords('tags')
  const byName = new Map(existing.map((record) => [String(record.name), record.id]))
  const ids: string[] = []

  for (const name of wanted) {
    let id = byName.get(name)
    if (!id) {
      const record = await createRecord('tags', { name, slug: slugify(name) })
      id = record.id
      byName.set(name, id)
    }
    ids.push(id)
  }
  return ids
}

function mapPost(record: PocketBaseRecord): AdminPostDetail {
  return {
    id: record.id,
    title: String(record.title || ''),
    slug: String(record.slug || ''),
    summary: String(record.summary || ''),
    content_md: String(record.content_md || ''),
    content_html: String(record.content_html || ''),
    cover_url: fileUrl(record, 'cover'),
    tags: tagNames(record),
    views: Number(record.views || 0),
    is_published: Boolean(record.is_published),
    created_at: record.created,
    updated_at: record.updated,
    content_type: record.content_type === 'interview' ? 'interview' : 'article',
    interview_category: String(record.interview_category || ''),
  }
}

export const authApi = {
  async login(payload: LoginPayload): Promise<LoginResult & { email: string }> {
    // 自定义管理端复用 PocketBase 超级管理员认证，不额外维护第二套账号。
    const result = await adminApiClient.request<{ token: string; record: { email?: string } }>(
      '/api/collections/_superusers/auth-with-password',
      { method: 'POST', auth: false, body: { identity: payload.username, password: payload.password } },
    )
    adminApiClient.setToken(result.token)
    return { token: result.token, token_type: 'Bearer', expires_at: '', email: result.record.email || payload.username }
  },
}

export const adminPostsApi = {
  async listPosts(params: { page?: number; limit?: number; contentType?: PostContentType; category?: string } = {}) {
    const filters: string[] = []
    if (params.contentType === 'interview') filters.push('content_type = "interview"')
    if (params.contentType === 'article') filters.push('(content_type = "article" || content_type = "")')
    if (params.category === '未分类') filters.push('(interview_category = "" || interview_category = "未分类")')
    else if (params.category) filters.push(`interview_category = ${JSON.stringify(params.category)}`)
    const result = await listRecords('posts', params.page || 1, params.limit || 50, { filter: filters.join(' && '), sort: '-updated', expand: 'tags' })
    return mapPage<AdminPostSummary>(result, result.items.map(mapPost))
  },

  async getPost(id: EntityId) {
    return mapPost(await getRecord('posts', id, 'tags'))
  },

  async savePost(payload: AdminPostDraft, cover?: File) {
    const body = {
      title: payload.title,
      slug: payload.slug,
      summary: payload.summary,
      content_md: payload.content_md,
      content_type: payload.content_type,
      interview_category: payload.content_type === 'interview' ? payload.interview_category : '',
      // 管理端以 Markdown 为准，清空旧 HTML 后用户端会重新渲染最新正文。
      content_html: '',
      tags: await ensureTagIds(payload.tags),
      is_published: payload.is_published,
      ...(payload.id ? {} : { views: 0 }),
    }
    let record = payload.id
      ? await updateRecord('posts', payload.id, body)
      : await createRecord('posts', body)

    if (cover) {
      const form = new FormData()
      form.append('cover', cover)
      record = await adminApiClient.upload<PocketBaseRecord>(collectionPath('posts', `/${record.id}`), form, 'PATCH')
    }
    return mapPost(await getRecord('posts', record.id, 'tags'))
  },

  deletePost(id: EntityId) {
    return deleteRecord('posts', id)
  },
}

function mapInterviewCategory(record: PocketBaseRecord): InterviewCategory {
  return { id: record.id, name: String(record.name || ''), sort_order: Number(record.sort_order || 0) }
}

export const adminInterviewCategoriesApi = {
  async list() {
    return (await getAllRecords('interview_categories', { sort: 'sort_order,name' })).map(mapInterviewCategory)
  },

  async create(name: string) {
    const categories = await this.list()
    const normalizedName = name.trim()
    if (categories.some((category) => category.name.toLowerCase() === normalizedName.toLowerCase())) {
      throw new Error('该子目录已存在')
    }
    return mapInterviewCategory(await createRecord('interview_categories', {
      name: normalizedName,
      sort_order: categories.length ? Math.max(...categories.map((category) => category.sort_order)) + 10 : 0,
    }))
  },

  async delete(category: InterviewCategory) {
    const posts = await listRecords('posts', 1, 1, {
      filter: `content_type = "interview" && interview_category = ${JSON.stringify(category.name)}`,
    })
    if (posts.totalItems > 0) throw new Error(`“${category.name}”目录下还有文章，请先移动这些文章`)
    await deleteRecord('interview_categories', category.id)
  },
}

function mapLearningRoadmap(record: PocketBaseRecord): AdminLearningRoadmapDraft {
  return {
    id: record.id,
    title: String(record.title || ''),
    subtitle: String(record.subtitle || ''),
    content_md: String(record.content_md || ''),
    cover_url: fileUrl(record, 'cover'),
    is_published: Boolean(record.is_published),
  }
}

/** 学习路线使用独立集合，不占用八股文章和子目录记录。 */
export const adminLearningRoadmapApi = {
  async get(): Promise<AdminLearningRoadmapDraft | null> {
    const result = await listRecords('learning_roadmap', 1, 1, { sort: '-updated' })
    return result.items[0] ? mapLearningRoadmap(result.items[0]) : null
  },

  async save(payload: AdminLearningRoadmapDraft, cover?: File) {
    const body = {
      title: payload.title.trim(),
      subtitle: payload.subtitle.trim(),
      content_md: payload.content_md,
      is_published: payload.is_published,
    }
    let record = payload.id
      ? await updateRecord('learning_roadmap', payload.id, body)
      : await createRecord('learning_roadmap', body)

    if (cover) {
      const form = new FormData()
      form.append('cover', cover)
      record = await adminApiClient.upload<PocketBaseRecord>(collectionPath('learning_roadmap', `/${record.id}`), form, 'PATCH')
    }
    return mapLearningRoadmap(record)
  },
}

function mapNote(record: PocketBaseRecord): AdminNote {
  return {
    id: record.id,
    content: String(record.content || ''),
    mood: String(record.mood || ''),
    is_published: Boolean(record.is_published),
    created_at: record.created,
  }
}

function mapProject(record: PocketBaseRecord): AdminProject {
  return {
    id: record.id,
    name: String(record.name || ''),
    summary: String(record.summary || ''),
    url: String(record.url || ''),
    tags: tagNames(record),
    sort_order: Number(record.sort_order || 0),
    is_published: Boolean(record.is_published),
    created_at: record.created,
  }
}

export const adminContentApi = {
  async listNotes(params: { page?: number; limit?: number } = {}) {
    const result = await listRecords('notes', params.page || 1, params.limit || 50, { sort: '-created' })
    return mapPage<AdminNote>(result, result.items.map(mapNote))
  },
  async createNote(payload: AdminNoteDraft) {
    return mapNote(await createRecord('notes', { ...payload }))
  },
  async updateNote(id: EntityId, payload: AdminNoteDraft) {
    return mapNote(await updateRecord('notes', id, { ...payload }))
  },
  deleteNote(id: EntityId) {
    return deleteRecord('notes', id)
  },
  async listProjects(params: { page?: number; limit?: number } = {}) {
    const result = await listRecords('projects', params.page || 1, params.limit || 50, { sort: 'sort_order,-created', expand: 'tags' })
    return mapPage<AdminProject>(result, result.items.map(mapProject))
  },
  async createProject(payload: AdminProjectDraft) {
    const record = await createRecord('projects', { ...payload, tags: await ensureTagIds(payload.tags) })
    return mapProject(await getRecord('projects', record.id, 'tags'))
  },
  async updateProject(id: EntityId, payload: AdminProjectDraft) {
    const record = await updateRecord('projects', id, { ...payload, tags: await ensureTagIds(payload.tags) })
    return mapProject(await getRecord('projects', record.id, 'tags'))
  },
  deleteProject(id: EntityId) {
    return deleteRecord('projects', id)
  },
}

function mapComment(record: PocketBaseRecord): AdminComment {
  const post = expandedRecords(record, 'post')[0]
  return {
    id: record.id,
    post_id: String(record.post || ''),
    nickname: String(record.nickname || ''),
    content: String(record.content || ''),
    status: String(record.status || 'pending') as AdminComment['status'],
    is_approved: record.status === 'approved',
    post_title: post ? String(post.title || '') : '',
    post_slug: post ? String(post.slug || '') : '',
    created_at: record.created,
    updated_at: record.updated,
  }
}

function mapGuestbook(record: PocketBaseRecord): AdminGuestbookMessage {
  return {
    id: record.id,
    nickname: String(record.nickname || ''),
    content: String(record.content || ''),
    status: String(record.status || 'pending') as AdminGuestbookMessage['status'],
    is_approved: record.status === 'approved',
    created_at: record.created,
    updated_at: record.updated,
  }
}

const aiUnavailable = () => Promise.reject(new Error('PocketBase 模式暂未启用 AI 自动审核，请先使用人工审核。'))

export const moderationApi = {
  async listComments(params: { page?: number; limit?: number } = {}) {
    const result = await listRecords('comments', params.page || 1, params.limit || 50, { sort: '-created', expand: 'post' })
    return mapPage<AdminComment>(result, result.items.map(mapComment))
  },
  async listGuestbook(params: { page?: number; limit?: number } = {}) {
    const result = await listRecords('guestbook_messages', params.page || 1, params.limit || 50, { sort: '-created' })
    return mapPage<AdminGuestbookMessage>(result, result.items.map(mapGuestbook))
  },
  approveComment(id: EntityId) { return updateRecord('comments', id, { status: 'approved' }) },
  rejectComment(id: EntityId) { return updateRecord('comments', id, { status: 'rejected' }) },
  deleteComment(id: EntityId) { return deleteRecord('comments', id) },
  approveGuestbook(id: EntityId) { return updateRecord('guestbook_messages', id, { status: 'approved' }) },
  rejectGuestbook(id: EntityId) { return updateRecord('guestbook_messages', id, { status: 'rejected' }) },
  deleteGuestbook(id: EntityId) { return deleteRecord('guestbook_messages', id) },
  getConfig: (): Promise<ModerationConfig> => aiUnavailable(),
  saveConfig: (_config: ModerationConfig): Promise<ModerationConfig> => aiUnavailable(),
  listLogs: (_params?: { page?: number; limit?: number }): Promise<PageResult<ModerationLog>> => aiUnavailable(),
  test: (_payload: ModerationTestPayload): Promise<ModerationTestResult> => aiUnavailable(),
  testContent: (_payload: ModerationTestPayload): Promise<ModerationTestResult> => aiUnavailable(),
  moderateComment: (_id: EntityId): Promise<ModerationTestResult> => aiUnavailable(),
  moderateGuestbook: (_id: EntityId): Promise<ModerationTestResult> => aiUnavailable(),
}

async function getSiteConfigRecord() {
  const result = await listRecords('site_config', 1, 1, { sort: 'created' })
  return result.items[0]
}

export const settingsApi = {
  async getConfig(): Promise<SiteConfig> {
    const record = await getSiteConfigRecord()
    return {
      title: String(record?.title || ''),
      subtitle: String(record?.subtitle || ''),
      announcement: String(record?.announcement || ''),
    }
  },
  async saveSetting(key: keyof SiteConfig, value: string) {
    const record = await getSiteConfigRecord()
    if (!record) throw new Error('站点配置记录不存在')
    await updateRecord('site_config', record.id, { [key]: value })
    return { key, value }
  },
}

function mapMusicTrack(record: PocketBaseRecord): AdminMusicTrack {
  return {
    id: record.id,
    title: String(record.title || ''),
    artist: String(record.artist || ''),
    cover_url: fileUrl(record, 'cover'),
    audio_url: fileUrl(record, 'audio'),
    sort_order: Number(record.sort_order || 0),
    is_enabled: Boolean(record.is_enabled),
  }
}

function recordIdFromFileUrl(url: string) {
  // 上传音频时先创建记录，保存歌单时再从文件地址取回对应记录 ID。
  const match = url.match(/\/api\/files\/[^/]+\/([^/]+)\//)
  return match?.[1] || ''
}

export const adminMusicApi = {
  async getConfig(): Promise<AdminMusicConfig> {
    const [config, tracks] = await Promise.all([
      getSiteConfigRecord(),
      getAllRecords('music_tracks', { sort: 'sort_order,-created' }),
    ])
    return {
      enabled: Boolean(config?.music_enabled),
      volume: Number(config?.music_volume ?? 0.35),
      tracks: tracks.map(mapMusicTrack),
    }
  },
  async saveConfig(config: AdminMusicConfig): Promise<AdminMusicConfig> {
    const site = await getSiteConfigRecord()
    if (!site) throw new Error('站点配置记录不存在')
    await updateRecord('site_config', site.id, { music_enabled: config.enabled, music_volume: config.volume })

    const existing = await getAllRecords('music_tracks')
    const keptIds = new Set<string>()
    for (const track of config.tracks) {
      const id = String(track.id || recordIdFromFileUrl(track.audio_url))
      if (!id) throw new Error(`歌曲《${track.title}》尚未上传音频文件`)
      keptIds.add(id)
      await updateRecord('music_tracks', id, {
        title: track.title,
        artist: track.artist,
        sort_order: track.sort_order,
        is_enabled: track.is_enabled,
      })
    }
    await Promise.all(existing.filter((record) => !keptIds.has(record.id)).map((record) => deleteRecord('music_tracks', record.id)))
    return this.getConfig()
  },
}

function titleFromFileName(fileName: string) {
  return fileName.replace(/\.[^.]+$/, '').replace(/^\s*\d{1,3}\s*[._-]\s*/, '').trim() || '未命名内容'
}

export const uploadApi = {
  async uploadMarkdown(file: File, contentType: PostContentType = 'article', interviewCategory = ''): Promise<{ id: EntityId; title: string }> {
    // 导入只创建草稿，避免用户选择文件后文章被立即公开。
    const content = await file.text()
    const firstHeading = content.match(/^#\s+(.+)$/m)?.[1]?.trim()
    const title = firstHeading || titleFromFileName(file.name)
    const summary = content.replace(/^#\s+.+$/m, '').replace(/[#>*_`\[\]()!-]/g, ' ').replace(/\s+/g, ' ').trim().slice(0, 180)
    const record = await createRecord('posts', {
      title,
      slug: `${slugify(title)}-${Date.now().toString(36)}`,
      summary,
      content_md: content,
      content_type: contentType,
      interview_category: contentType === 'interview' ? interviewCategory : '',
      is_published: false,
      views: 0,
    })
    return { id: record.id, title }
  },
  uploadImage: (_file: File): Promise<UploadAsset> => Promise.reject(new Error('请在文章编辑抽屉中直接上传封面图片。')),
  async uploadAudio(file: File): Promise<UploadAsset> {
    const form = new FormData()
    const title = titleFromFileName(file.name)
    form.append('title', title)
    form.append('artist', '')
    form.append('sort_order', '999')
    form.append('is_enabled', 'false')
    form.append('audio', file)
    const record = await adminApiClient.upload<PocketBaseRecord>(collectionPath('music_tracks'), form)
    return { url: fileUrl(record, 'audio'), filename: String(record.audio || file.name), size: file.size, content_type: file.type }
  },
}
