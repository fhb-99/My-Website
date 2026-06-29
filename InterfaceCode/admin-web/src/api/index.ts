/**
 * @file 管理端 API 通信层
 * @description 统一封装后台管理端与 C++ 后端的通信逻辑。页面层只调用这里的业务方法，
 *              不直接拼 URL、不直接管理 token，后续扩展接口会更稳。
 */

import type { ApiClientConfig, RequestOptions } from '@shared/api'
import type {
  AdminPostDraft,
  Comment,
  GuestbookMessage,
  LoginPayload,
  LoginResult,
  PageResult,
  PostSummary,
  SiteConfig,
  UploadAsset
} from '@shared/types'

const TOKEN_KEY = 'blog-admin-token'
const API_BASE_KEY = 'blog-api-base'

/** 管理端接口错误：保留 HTTP 状态码和后端响应体，方便页面精确提示。 */
export class AdminApiError extends Error {
  status: number
  data: unknown

  constructor(message: string, status: number, data: unknown) {
    super(message)
    this.name = 'AdminApiError'
    this.status = status
    this.data = data
  }
}

/** 去掉 baseURL 末尾多余的斜杠，避免拼接出 //api 这类路径。 */
function trimTrailingSlash(value: string) {
  return value.replace(/\/+$/, '')
}

/**
 * 获取默认后端地址。
 * 优先级：Vite 环境变量 > localStorage 中的开发配置 > 本地 C++ 服务默认地址。
 */
function getDefaultBaseUrl() {
  const envBase = import.meta.env.VITE_API_BASE_URL as string | undefined
  if (envBase) return trimTrailingSlash(envBase)

  const saved = localStorage.getItem(API_BASE_KEY)
  if (saved) return trimTrailingSlash(saved)

  return 'http://127.0.0.1:8080'
}

/** 拼接请求地址，并过滤空 query，避免把 undefined/null 传给后端。 */
function buildUrl(baseUrl: string, path: string, query?: RequestOptions['query']) {
  const params = new URLSearchParams()
  Object.entries(query || {}).forEach(([key, value]) => {
    if (value !== undefined && value !== null && value !== '') {
      params.set(key, String(value))
    }
  })

  const url = `${trimTrailingSlash(baseUrl)}${path}`
  const qs = params.toString()
  return qs ? `${url}?${qs}` : url
}

/** 安全解析响应体：兼容空响应、JSON 响应和纯文本错误响应。 */
async function parseJsonSafe(response: Response) {
  const text = await response.text()
  if (!text) return null

  try {
    return JSON.parse(text) as unknown
  } catch {
    return { message: text }
  }
}

/** 从常见错误结构中提取可读信息，避免页面只能显示 HTTP 状态码。 */
function getErrorMessage(data: unknown) {
  if (typeof data === 'object' && data) {
    const body = data as { error?: unknown; message?: unknown }
    if (body.error || body.message) return String(body.error || body.message)
  }
  return '请求失败'
}

/**
 * 管理端通用请求客户端。
 * 这里不依赖 Vue/Pinia，避免 API 层和状态层互相引用造成循环依赖。
 */
export const adminApiClient = {
  config: {
    baseUrl: getDefaultBaseUrl(),
    token: localStorage.getItem(TOKEN_KEY) || undefined
  } as ApiClientConfig,

  /** 批量设置客户端配置，适合应用启动或调试面板同步配置。 */
  setConfig(config: ApiClientConfig) {
    this.setBaseUrl(config.baseUrl)
    this.setToken(config.token)
  },

  /** 设置后端地址，并持久化到本地，刷新页面后仍然生效。 */
  setBaseUrl(baseUrl: string) {
    this.config.baseUrl = trimTrailingSlash(baseUrl)
    localStorage.setItem(API_BASE_KEY, this.config.baseUrl)
  },

  /** 设置或清除管理员 token；登录和退出都走这里，保证请求头一致。 */
  setToken(token?: string) {
    this.config.token = token || undefined
    if (token) localStorage.setItem(TOKEN_KEY, token)
    else localStorage.removeItem(TOKEN_KEY)
  },

  /**
   * 统一 JSON 请求入口。
   * 自动处理 query、Authorization、JSON 序列化和错误解析。
   */
  async request<T>(path: string, options: RequestOptions = {}) {
    const headers = new Headers()
    const init: RequestInit = { method: options.method || 'GET', headers }

    if (this.config.token && options.auth !== false) {
      headers.set('Authorization', `Bearer ${this.config.token}`)
    }

    if (options.body !== undefined) {
      headers.set('Content-Type', 'application/json; charset=utf-8')
      init.body = JSON.stringify(options.body)
    }

    const response = await fetch(buildUrl(this.config.baseUrl, path, options.query), init)
    const data = await parseJsonSafe(response)

    if (!response.ok) {
      throw new AdminApiError(getErrorMessage(data), response.status, data)
    }

    return data as T
  },

  /**
   * 统一文件上传入口。
   * multipart/form-data 必须由浏览器自动生成 boundary，所以这里不要手动设置 Content-Type。
   */
  async upload<T>(path: string, form: FormData) {
    const headers = new Headers()
    if (this.config.token) headers.set('Authorization', `Bearer ${this.config.token}`)

    const response = await fetch(buildUrl(this.config.baseUrl, path), {
      method: 'POST',
      headers,
      body: form
    })
    const data = await parseJsonSafe(response)

    if (!response.ok) {
      throw new AdminApiError(getErrorMessage(data), response.status, data)
    }

    return data as T
  }
}

/** 登录认证接口。 */
export const authApi = {
  async login(payload: LoginPayload) {
    const result = await adminApiClient.request<LoginResult>('/api/auth/login', {
      method: 'POST',
      body: payload,
      auth: false
    })
    adminApiClient.setToken(result.token)
    return result
  },

  logout() {
    adminApiClient.setToken(undefined)
  }
}

/** 文章管理接口。 */
export const adminPostsApi = {
  /** 获取后台文章列表；当前后端已有 GET /api/admin/posts。 */
  listPosts: (params: { page?: number; limit?: number } = {}) =>
    adminApiClient.request<PageResult<PostSummary>>('/api/admin/posts', { query: params }),

  /** 创建文章；当前后端已有 POST /api/admin/posts。 */
  createPost: (payload: AdminPostDraft) =>
    adminApiClient.request<{ id: number; slug?: string; message?: string }>('/api/admin/posts', {
      method: 'POST',
      body: payload
    }),

  /** 保存文章：有 id 则更新，否则创建，方便表单页复用同一个保存动作。 */
  savePost: (payload: AdminPostDraft) => {
    if (payload.id) return adminPostsApi.updatePost(payload.id, payload)
    return adminPostsApi.createPost(payload)
  },

  /** 目标接口预留：C++ 后端补齐 PUT /api/admin/posts/{id} 后可直接使用。 */
  updatePost: (id: number, payload: AdminPostDraft) =>
    adminApiClient.request<{ id?: number; message?: string }>(`/api/admin/posts/${encodeURIComponent(id)}`, {
      method: 'PUT',
      body: payload
    }),

  /** 目标接口预留：C++ 后端补齐 DELETE /api/admin/posts/{id} 后可直接使用。 */
  deletePost: (id: number) =>
    adminApiClient.request<void>(`/api/admin/posts/${encodeURIComponent(id)}`, { method: 'DELETE' })
}

/** 上传管理接口。 */
export const uploadApi = {
  /** 上传图片；字段名保持和 C++ 后端一致：image。 */
  uploadImage: (file: File) => {
    const form = new FormData()
    form.append('image', file)
    return adminApiClient.upload<UploadAsset>('/api/admin/uploads/images', form)
  },

  /** 上传 Markdown；字段名保持和 C++ 后端一致：markdown，可附带文章元信息。 */
  uploadMarkdown: (
    file: File,
    fields: Partial<Pick<AdminPostDraft, 'title' | 'slug' | 'summary' | 'cover_url'>> = {}
  ) => {
    const form = new FormData()
    form.append('markdown', file)
    Object.entries(fields).forEach(([key, value]) => {
      if (value !== undefined && value !== '') form.append(key, String(value))
    })
    return adminApiClient.upload<{ id: number; slug: string; title: string; summary?: string; url?: string }>(
      '/api/admin/uploads/markdown',
      form
    )
  }
}

/** 留言和评论审核接口。后端对应审核路由未完成时，这些方法会正常返回 404。 */
export const moderationApi = {
  listGuestbook: (params: { page?: number; limit?: number } = {}) =>
    adminApiClient.request<PageResult<GuestbookMessage>>('/api/admin/guestbook', { query: params }),

  approveGuestbook: (id: number) =>
    adminApiClient.request<void>(`/api/admin/guestbook/${encodeURIComponent(id)}/approve`, { method: 'PUT' }),

  deleteGuestbook: (id: number) =>
    adminApiClient.request<void>(`/api/admin/guestbook/${encodeURIComponent(id)}`, { method: 'DELETE' }),

  listComments: (params: { page?: number; limit?: number; postId?: number } = {}) =>
    adminApiClient.request<PageResult<Comment>>('/api/admin/comments', { query: params }),

  approveComment: (id: number) =>
    adminApiClient.request<void>(`/api/admin/comments/${encodeURIComponent(id)}/approve`, { method: 'PUT' }),

  deleteComment: (id: number) =>
    adminApiClient.request<void>(`/api/admin/comments/${encodeURIComponent(id)}`, { method: 'DELETE' })
}

/** 站点基础设置接口。 */
export const settingsApi = {
  /** 目标接口预留：读取站点配置。 */
  getConfig: () => adminApiClient.request<SiteConfig>('/api/admin/config'),

  /** 目标接口预留：按 key 保存单项配置。 */
  saveSetting: (key: string, value: string) =>
    adminApiClient.request<void>(`/api/admin/config/${encodeURIComponent(key)}`, {
      method: 'PUT',
      body: { value }
    })
}
