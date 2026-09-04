import DOMPurify from 'dompurify'
import MarkdownIt from 'markdown-it'

import { DEFAULT_INTERVIEW_CATEGORIES } from '@shared/types'
import type {
  Comment,
  CommentPayload,
  EntityId,
  GuestbookMessage,
  GuestbookPayload,
  InterviewDirectory,
  InterviewCategory,
  LearningRoadmap,
  MusicConfig,
  MusicTrack,
  NoteItem,
  PageResult,
  PostDetail,
  PostContentType,
  PostNavigation,
  PostSummary,
  ProjectItem,
  PublicSubmissionResult,
  SiteConfig,
  TagSummary,
} from '@shared/types'

const configuredUrl = (import.meta.env.VITE_POCKETBASE_URL as string | undefined)?.trim().replace(/\/+$/, '')

export const pocketBaseEnabled = Boolean(configuredUrl)

const pocketBaseUrl = configuredUrl || 'http://127.0.0.1:8090'

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

class PocketBaseHttpError extends Error {
  constructor(public status: number, message: string) {
    super(message)
    this.name = 'PocketBaseHttpError'
  }
}

function collectionPath(collection: string, suffix = '') {
  return `/api/collections/${encodeURIComponent(collection)}/records${suffix}`
}

async function pocketBaseRequest<T>(path: string, init?: RequestInit): Promise<T> {
  const response = await fetch(`${pocketBaseUrl}${path}`, init)
  const data = await response.json().catch(() => null) as { message?: string } | null
  if (!response.ok) throw new PocketBaseHttpError(response.status, data?.message || 'PocketBase 请求失败')
  return data as T
}

function filterValue(value: string | number | boolean) {
  return typeof value === 'string' ? JSON.stringify(value) : String(value)
}

async function listRecords(
  collection: string,
  page: number,
  perPage: number,
  options: { filter?: string; sort?: string; expand?: string } = {},
) {
  const query = new URLSearchParams({ page: String(page), perPage: String(perPage) })
  if (options.filter) query.set('filter', options.filter)
  if (options.sort) query.set('sort', options.sort)
  if (options.expand) query.set('expand', options.expand)
  return pocketBaseRequest<PocketBasePage>(`${collectionPath(collection)}?${query}`)
}

async function getAllRecords(collection: string, options: { filter?: string; sort?: string; expand?: string } = {}) {
  const items: PocketBaseRecord[] = []
  let page = 1

  while (true) {
    const result = await listRecords(collection, page, 200, options)
    items.push(...result.items)
    if (page >= result.totalPages || result.items.length === 0) return items
    page += 1
  }
}

function getRecord(collection: string, id: string, expand?: string) {
  const query = expand ? `?${new URLSearchParams({ expand })}` : ''
  return pocketBaseRequest<PocketBaseRecord>(collectionPath(collection, `/${encodeURIComponent(id)}`) + query)
}

function createRecord(collection: string, body: Record<string, unknown>) {
  return pocketBaseRequest<PocketBaseRecord>(collectionPath(collection), {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(body),
  })
}

const markdown = new MarkdownIt({ html: false, linkify: true, typographer: true })

function renderMarkdown(source: string) {
  return DOMPurify.sanitize(markdown.render(source))
}

function getExpandedRecords(record: PocketBaseRecord, field: string) {
  const expanded = record.expand?.[field]
  if (!expanded) return []
  return Array.isArray(expanded) ? expanded : [expanded]
}

function mapTags(record: PocketBaseRecord) {
  return getExpandedRecords(record, 'tags')
    .map((tag) => String(tag.name || ''))
    .filter(Boolean)
}

function getFileUrl(record: PocketBaseRecord, field: string) {
  const filename = String(record[field] || '')
  const collection = record.collectionId || record.collectionName || ''
  return filename && collection
    ? `${pocketBaseUrl}/api/files/${encodeURIComponent(collection)}/${encodeURIComponent(record.id)}/${encodeURIComponent(filename)}`
    : ''
}

function mapPostSummary(record: PocketBaseRecord): PostSummary {
  return {
    id: record.id,
    title: String(record.title || ''),
    slug: String(record.slug || ''),
    summary: String(record.summary || ''),
    cover_url: getFileUrl(record, 'cover'),
    tags: mapTags(record),
    views: Number(record.views || 0),
    created_at: record.created,
    content_type: record.content_type === 'interview' ? 'interview' : 'article',
    interview_category: String(record.interview_category || ''),
  }
}

function contentTypeFilter(contentType?: PostContentType) {
  if (contentType === 'interview') return 'content_type = "interview"'
  if (contentType === 'article') return '(content_type = "article" || content_type = "")'
  return ''
}

function mapPostDetail(record: PocketBaseRecord): PostDetail {
  const storedHtml = String(record.content_html || '')
  return {
    ...mapPostSummary(record),
    content_html: storedHtml
      ? DOMPurify.sanitize(storedHtml)
      : renderMarkdown(String(record.content_md || '')),
    updated_at: record.updated,
  }
}

function mapPage<T>(result: { items: T[]; page: number; perPage: number; totalItems: number; totalPages: number }): PageResult<T> {
  return {
    data: result.items,
    page: result.page,
    limit: result.perPage,
    total: result.totalItems,
    total_pages: result.totalPages,
    has_more: result.page < result.totalPages,
  }
}

async function findPost(idOrSlug: EntityId) {
  const value = String(idOrSlug)

  try {
    return await getRecord('posts', value, 'tags')
  } catch (reason) {
    if (!(reason instanceof PocketBaseHttpError) || reason.status !== 404) throw reason
  }

  const legacyId = /^\d+$/.test(value) ? Number(value) : -1
  const result = await listRecords(
    'posts',
    1,
    1,
    { filter: `slug = ${filterValue(value)} || legacy_id = ${filterValue(legacyId)}`, expand: 'tags' },
  )
  if (!result.items[0]) throw new PocketBaseHttpError(404, '文章不存在')
  return result.items[0]
}

export const pocketBasePostsApi = {
  async listPosts(params: { page?: number; limit?: number; q?: string; tag?: string; category?: string; contentType?: PostContentType } = {}) {
    const filters = ['is_published = true']
    const typeFilter = contentTypeFilter(params.contentType)
    if (typeFilter) filters.push(typeFilter)
    if (params.q?.trim()) {
      const q = filterValue(params.q.trim())
      filters.push(`(title ~ ${q} || summary ~ ${q} || content_md ~ ${q} || content_html ~ ${q})`)
    }
    if (params.tag?.trim()) {
      filters.push(`tags.name ?= ${filterValue(params.tag.trim())}`)
    }
    if (params.category?.trim()) {
      const category = params.category.trim()
      filters.push(category === '未分类'
        ? '(interview_category = "" || interview_category = "未分类")'
        : `interview_category = ${filterValue(category)}`)
    }

    const result = await listRecords('posts', params.page || 1, params.limit || 10, {
      filter: filters.join(' && '),
      sort: '-created',
      expand: 'tags',
    })
    return mapPage({ ...result, items: result.items.map(mapPostSummary) })
  },

  async listInterviewDirectory(): Promise<{ data: InterviewDirectory[] }> {
    const [records, categoryRecords] = await Promise.all([
      getAllRecords('posts', {
        filter: 'is_published = true && content_type = "interview"',
        sort: '-created',
        expand: 'tags',
      }),
      getAllRecords('interview_categories', { sort: 'sort_order,name' }),
    ])
    const grouped = new Map<string, PostSummary[]>()

    for (const record of records) {
      const post = mapPostSummary(record)
      const category = post.interview_category?.trim() || '未分类'
      const categoryPosts = grouped.get(category) || []
      categoryPosts.push(post)
      grouped.set(category, categoryPosts)
    }

    // 预设目录即使尚未上传文件也会展示，方便先确认知识库结构。
    const categories: InterviewCategory[] = categoryRecords.map((record) => ({
      id: record.id,
      name: String(record.name || ''),
      sort_order: Number(record.sort_order || 0),
    })).filter((category) => category.name)
    const names: string[] = categories.length ? categories.map((category) => category.name) : [...DEFAULT_INTERVIEW_CATEGORIES]
    for (const name of grouped.keys()) {
      if (!names.includes(name)) names.push(name)
    }
    return { data: names.map((name) => ({ name, post_count: grouped.get(name)?.length || 0, posts: grouped.get(name) || [] })) }
  },

  async getPostDetail(idOrSlug: EntityId) {
    return mapPostDetail(await findPost(idOrSlug))
  },

  async listTags(contentType?: PostContentType): Promise<{ data: TagSummary[] }> {
    const tags = await getAllRecords('tags', { sort: 'name' })
    const counts = await Promise.all(tags.map(async (tag) => {
      const typeFilter = contentTypeFilter(contentType)
      const result = await listRecords('posts', 1, 1, {
        filter: ['is_published = true', typeFilter, `tags ?= ${filterValue(tag.id)}`].filter(Boolean).join(' && '),
      })
      return { name: String(tag.name || ''), post_count: result.totalItems }
    }))
    return { data: counts.filter((tag) => tag.post_count > 0) }
  },

  async getNavigation(postId: EntityId, contentType?: PostContentType): Promise<PostNavigation> {
    const current = await findPost(postId)
    const typeFilter = contentTypeFilter(contentType)
    const [previousResult, nextResult] = await Promise.all([
      listRecords('posts', 1, 1, {
        filter: ['is_published = true', typeFilter, `created < ${filterValue(current.created)}`].filter(Boolean).join(' && '),
        sort: '-created',
        expand: 'tags',
      }),
      listRecords('posts', 1, 1, {
        filter: ['is_published = true', typeFilter, `created > ${filterValue(current.created)}`].filter(Boolean).join(' && '),
        sort: 'created',
        expand: 'tags',
      }),
    ])

    return {
      previous: previousResult.items[0] ? mapPostSummary(previousResult.items[0]) : null,
      next: nextResult.items[0] ? mapPostSummary(nextResult.items[0]) : null,
    }
  },

  async recordPostView(postId: EntityId, visitorId: string): Promise<{ counted: boolean; message?: string }> {
    return pocketBaseRequest<{ counted: boolean; message?: string }>(`/api/posts/${encodeURIComponent(String(postId))}/view`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ visitor_id: visitorId }),
    })
  },

  async searchPosts(keyword: string, limit = 10) {
    const result = await this.listPosts({ q: keyword, limit })
    return { data: result.data, message: result.data.length ? '' : '没有找到相关文章' }
  },
}

export const pocketBaseCommentsApi = {
  async listComments(postId: EntityId, params: { page?: number; limit?: number } = {}) {
    const result = await listRecords('comments', params.page || 1, params.limit || 20, {
      filter: `post = ${filterValue(String(postId))} && status = "approved"`,
      sort: '-created',
    })
    return mapPage<Comment>({
      ...result,
      items: result.items.map((record) => ({
        id: record.id,
        post_id: String(record.post),
        nickname: String(record.nickname || ''),
        content: String(record.content || ''),
        created_at: record.created,
      })),
    })
  },

  async createComment(postId: EntityId, payload: CommentPayload): Promise<PublicSubmissionResult> {
    const record = await createRecord('comments', {
      post: String(postId),
      nickname: payload.nickname.trim(),
      content: payload.content.trim(),
      status: 'pending',
    })
    return { id: record.id, message: '已提交，等待审核。', status: 'pending', is_approved: false }
  },
}

export const pocketBaseGuestbookApi = {
  async listMessages(params: { page?: number; limit?: number } = {}) {
    const result = await listRecords('guestbook_messages', params.page || 1, params.limit || 20, {
      filter: 'status = "approved"',
      sort: '-created',
    })
    return mapPage<GuestbookMessage>({
      ...result,
      items: result.items.map((record) => ({
        id: record.id,
        nickname: String(record.nickname || ''),
        content: String(record.content || ''),
        created_at: record.created,
      })),
    })
  },

  async createMessage(payload: GuestbookPayload): Promise<PublicSubmissionResult> {
    const record = await createRecord('guestbook_messages', {
      nickname: payload.nickname.trim(),
      content: payload.content.trim(),
      status: 'pending',
    })
    return { id: record.id, message: '已提交，等待审核。', status: 'pending', is_approved: false }
  },
}

export const pocketBaseNotesApi = {
  async listNotes(params: { page?: number; limit?: number } = {}) {
    const result = await listRecords('notes', params.page || 1, params.limit || 20, {
      filter: 'is_published = true',
      sort: '-created',
    })
    return mapPage<NoteItem>({
      ...result,
      items: result.items.map((record) => ({
        id: record.id,
        content: String(record.content || ''),
        mood: String(record.mood || ''),
        created_at: record.created,
      })),
    })
  },
}

export const pocketBaseProjectsApi = {
  async listProjects(params: { page?: number; limit?: number } = {}) {
    const result = await listRecords('projects', params.page || 1, params.limit || 20, {
      filter: 'is_published = true',
      sort: 'sort_order,-created',
      expand: 'tags',
    })
    return mapPage<ProjectItem>({
      ...result,
      items: result.items.map((record) => ({
        id: record.id,
        name: String(record.name || ''),
        summary: String(record.summary || ''),
        url: String(record.url || ''),
        tags: mapTags(record),
        created_at: record.created,
      })),
    })
  },
}

async function getSiteConfigRecord() {
  const result = await listRecords('site_config', 1, 1, { sort: 'created' })
  return result.items[0]
}

export const pocketBaseSiteConfigApi = {
  async getConfig(): Promise<SiteConfig> {
    const record = await getSiteConfigRecord()
    return {
      title: String(record?.title || ''),
      subtitle: String(record?.subtitle || ''),
      announcement: String(record?.announcement || ''),
    }
  },
}

export const pocketBaseMusicApi = {
  async getConfig(): Promise<MusicConfig> {
    const [config, tracks] = await Promise.all([
      getSiteConfigRecord(),
      getAllRecords('music_tracks', { filter: 'is_enabled = true', sort: 'sort_order' }),
    ])

    return {
      enabled: Boolean(config?.music_enabled),
      volume: Number(config?.music_volume ?? 0.35),
      tracks: tracks.map<MusicTrack>((record) => ({
        id: record.id,
        title: String(record.title || ''),
        artist: String(record.artist || ''),
        cover_url: getFileUrl(record, 'cover'),
        audio_url: getFileUrl(record, 'audio'),
        sort_order: Number(record.sort_order || 0),
      })),
    }
  },
}

export const pocketBaseLearningRoadmapApi = {
  async get(): Promise<LearningRoadmap> {
    const result = await listRecords('learning_roadmap', 1, 1, {
      filter: 'is_published = true',
      sort: '-updated',
    })
    const record = result.items[0]
    if (!record) throw new PocketBaseHttpError(404, '学习路线暂未发布')

    return {
      id: record.id,
      title: String(record.title || ''),
      subtitle: String(record.subtitle || ''),
      cover_url: getFileUrl(record, 'cover'),
      content_html: renderMarkdown(String(record.content_md || '')),
      updated_at: record.updated,
    }
  },
}
