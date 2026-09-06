/**
 * @file        API 客户端模块
 * @description 封装了与后端 REST API 的通信逻辑，包括错误处理、URL 构建、各类资源的请求方法等。
 */

// 类型导入
import type { ApiClientConfig, RequestOptions } from "@shared/api";
import type {
  Comment,
  CommentPayload,
  GuestbookMessage,
  GuestbookPayload,
  InterviewDirectory,
  LearningRoadmap,
  MusicConfig,
  NoteItem,
  PageResult,
  PostNavigation,
  PostDetail,
  PostContentType,
  PostSummary,
  PublicSubmissionResult,
  ProjectItem,
  SiteConfig,
  TagSummary,
} from "@shared/types";
import {
  pocketBaseCommentsApi,
  pocketBaseEnabled,
  pocketBaseGuestbookApi,
  pocketBaseLearningRoadmapApi,
  pocketBaseMusicApi,
  pocketBaseNotesApi,
  pocketBasePostsApi,
  pocketBaseProjectsApi,
  pocketBaseSiteConfigApi,
} from "./pocketbase";
import type { EntityId } from "@shared/types";

/** 本地存储中保存 API 基础地址的键名 */
const API_BASE_KEY = "blog-api-base";
const VISITOR_ID_KEY = "blog-visitor-id";

/** 自定义 API 错误类，继承自 Error，用于携带 HTTP 状态码与响应数据 */
export class ApiError extends Error {
  /** HTTP 状态码 */
  status: number;
  /** 服务端返回的响应体数据 */
  data: unknown;

  /**
   * 创建一个 API 错误实例
   * @param message - 错误描述信息
   * @param status  - HTTP 状态码
   * @param data     - 服务端返回的附加数据（如错误详情）
   */
  constructor(message: string, status: number, data: unknown) {
    super(message);
    this.name = "ApiError";
    this.status = status;
    this.data = data;
  }
}

/**
 * 去除字符串末尾的斜杠
 * @param value - 输入字符串
 * @returns 去除末尾斜杠后的字符串
 */
function trimTrailingSlash(value: string) {
  return value.replace(/\/+$/, "");
}

/**
 * 获取默认的 API 基础地址
 * 优先级：环境变量 > localStorage 缓存 > 根据协议自动判断
 * @returns API 基础地址字符串
 */
function getDefaultBaseUrl() {
  const envBase = import.meta.env.VITE_API_BASE_URL as string | undefined;
  if (envBase) return trimTrailingSlash(envBase);

  const saved = localStorage.getItem(API_BASE_KEY);
  if (saved) return trimTrailingSlash(saved);

  return window.location.protocol === "file:" ? "http://127.0.0.1:8080" : "";
}

function createVisitorId() {
  if (crypto.randomUUID) return crypto.randomUUID();
  return `visitor-${Date.now()}-${Math.random().toString(16).slice(2)}`;
}

function getVisitorId() {
  const saved = localStorage.getItem(VISITOR_ID_KEY);
  if (saved) return saved;

  const visitorId = createVisitorId();
  localStorage.setItem(VISITOR_ID_KEY, visitorId);
  return visitorId;
}

/**
 * 构建完整的请求 URL
 * @param baseUrl - API 基础地址
 * @param path    - 请求路径
 * @param query   - 可选的查询参数对象
 * @returns 拼接后的完整 URL（含查询字符串）
 */
function buildUrl(
  baseUrl: string,
  path: string,
  query?: RequestOptions["query"],
) {
  const params = new URLSearchParams();
  Object.entries(query || {}).forEach(([key, value]) => {
    if (value !== undefined && value !== null && value !== "")
      params.set(key, String(value));
  });

  const url = `${trimTrailingSlash(baseUrl)}${path}`;
  const qs = params.toString();
  return qs ? `${url}?${qs}` : url;
}

/**
 * 安全地解析 JSON 响应体
 * @param response - Fetch API 的 Response 对象
 * @returns 解析后的 JSON 对象，解析失败时返回包含原始文本的 { message } 对象，空响应体返回 null
 */
async function parseJsonSafe(response: Response) {
  const text = await response.text();
  if (!text) return null;

  try {
    return JSON.parse(text) as unknown;
  } catch {
    return { message: text };
  }
}

/** API 客户端核心对象，封装了请求配置与通用请求方法 */
export const apiClient = {
  /** 客户端配置，包含基础 URL 和认证令牌 */
  config: { baseUrl: getDefaultBaseUrl(), token: undefined } as ApiClientConfig,

  /**
   * 设置 API 基础地址，并同步保存到 localStorage
   * @param baseUrl - 新的 API 基础地址
   */
  setBaseUrl(baseUrl: string) {
    this.config.baseUrl = trimTrailingSlash(baseUrl);
    localStorage.setItem(API_BASE_KEY, this.config.baseUrl);
  },

  /**
   * 设置认证令牌
   * @param token - JWT 令牌，传 undefined 可清除令牌
   */
  setToken(token?: string) {
    this.config.token = token;
  },

  /**
   * 发起 HTTP 请求
   * @param path    - 请求路径
   * @param options - 请求选项（方法、请求体、查询参数、认证开关等）
   * @returns 解析后的响应数据
   * @throws {ApiError} 当响应状态码不在 2xx 范围内时抛出
   */
  async request<T>(path: string, options: RequestOptions = {}) {
    const headers = new Headers();
    const init: RequestInit = { method: options.method || "GET", headers };

    Object.entries(options.headers || {}).forEach(([key, value]) => {
      headers.set(key, value);
    });

    if (this.config.token && options.auth !== false) {
      headers.set("Authorization", `Bearer ${this.config.token}`);
    }

    if (options.body !== undefined) {
      headers.set("Content-Type", "application/json; charset=utf-8");
      init.body = JSON.stringify(options.body);
    }

    const response = await fetch(
      buildUrl(this.config.baseUrl, path, options.query),
      init,
    );
    const data = await parseJsonSafe(response);

    // 开发服务器可能把不存在的 /api 路由回退到 index.html。
    // 成功响应如果不是 JSON，应按接口异常处理，避免页面继续读取错误结构。
    const contentType = response.headers.get("Content-Type") || "";
    if (response.ok && data !== null && !contentType.toLowerCase().includes("application/json")) {
      throw new ApiError("接口返回格式不正确", response.status, data);
    }

    if (!response.ok) {
      const message =
        typeof data === "object" &&
        data &&
        ("error" in data || "message" in data)
          ? String(
              (data as { error?: unknown; message?: unknown }).error ||
                (data as { message?: unknown }).message,
            )
          : "请求失败";
      throw new ApiError(message, response.status, data);
    }

    return data as T;
  },
};

/** 文章相关 API */
export const postsApi = {
  /**
   * 获取文章列表
   * @param params - 分页参数（page 页码，limit 每页数量）
   */
  listPosts: (params: { page?: number; limit?: number; q?: string; tag?: string; category?: string; contentType?: PostContentType } = {}) => {
    if (pocketBaseEnabled) return pocketBasePostsApi.listPosts(params);
    return apiClient.request<PageResult<PostSummary>>("/api/posts", {
        query: params,
        auth: false,
      });
  },

  /**
   * 获取文章详情
   * @param idOrSlug - 文章 ID（数字）或 slug（字符串）
   */
  getPostDetail: (idOrSlug: EntityId) => {
    if (pocketBaseEnabled) return pocketBasePostsApi.getPostDetail(idOrSlug);
    const value = String(idOrSlug);
    const path = /^\d+$/.test(value)
      ? `/api/posts/${encodeURIComponent(value)}`
      : `/api/posts/slug/${encodeURIComponent(value)}`;
    return apiClient.request<PostDetail>(path, { auth: false });
  },

  listTags: (contentType?: PostContentType) => pocketBaseEnabled
    ? pocketBasePostsApi.listTags(contentType)
    : apiClient.request<{ data: TagSummary[] }>("/api/tags", { auth: false }),

  listInterviewDirectory: () => pocketBaseEnabled
    ? pocketBasePostsApi.listInterviewDirectory()
    : apiClient.request<{ data: InterviewDirectory[] }>("/api/interview/directories", { auth: false }),

  getNavigation: (postId: EntityId, contentType?: PostContentType) => pocketBaseEnabled
    ? pocketBasePostsApi.getNavigation(postId, contentType)
    : apiClient.request<PostNavigation>(`/api/posts/${encodeURIComponent(postId)}/navigation`, {
        auth: false,
      }),

  /**
   * 上报一次有效阅读；后端会按“同一访客、同一文章、同一天”去重。
   * @param postId - 文章 ID
   */
  recordPostView: (postId: EntityId) => pocketBaseEnabled
    ? pocketBasePostsApi.recordPostView(postId, getVisitorId())
    : apiClient.request<{ counted: boolean; message?: string }>(
        `/api/posts/${encodeURIComponent(postId)}/view`,
        {
          method: "POST",
          auth: false,
          headers: { "X-Visitor-Id": getVisitorId() },
        },
      ),

  /**
   * 搜索文章
   * @param keyword - 搜索关键词
   * @param limit   - 返回结果数量上限，默认 10
   */
  searchPosts: (keyword: string, limit = 10) => pocketBaseEnabled
    ? pocketBasePostsApi.searchPosts(keyword, limit)
    : apiClient.request<{ data: PostSummary[]; message: string }>("/api/search", {
        query: { q: keyword, limit },
        auth: false,
      }),
};

/** 独立于八股文目录的 C++ 学习路线。 */
export const learningRoadmapApi = {
  get: (): Promise<LearningRoadmap> => pocketBaseEnabled
    ? pocketBaseLearningRoadmapApi.get()
    : apiClient.request<LearningRoadmap>('/api/learning-roadmap', { auth: false }),
};

/** 评论相关 API */
export const commentsApi = {
  /**
   * 获取文章评论列表
   * @param postId - 文章 ID
   * @param params - 分页参数
   */
  listComments: (
    postId: EntityId,
    params: { page?: number; limit?: number } = {},
  ) => pocketBaseEnabled
    ? pocketBaseCommentsApi.listComments(postId, params)
    : apiClient.request<PageResult<Comment>>(
        `/api/posts/${encodeURIComponent(postId)}/comments`,
        { query: params, auth: false },
      ),

  /**
   * 创建评论
   * @param postId  - 文章 ID
   * @param payload - 评论内容载荷
   */
  createComment: (postId: EntityId, payload: CommentPayload) => pocketBaseEnabled
    ? pocketBaseCommentsApi.createComment(postId, payload)
    : apiClient.request<PublicSubmissionResult>(
        `/api/posts/${encodeURIComponent(postId)}/comments`,
        {
          method: "POST",
          body: payload,
          auth: false,
        },
      ),
};

/**
 * 留言板相关 API
 * 注意：当前 C++ 后端可能返回 404，需要等待对应路由实现。
 */
export const guestbookApi = {
  /**
   * 获取留言列表
   * @param params - 分页参数
   */
  listMessages: (params: { page?: number; limit?: number } = {}) => pocketBaseEnabled
    ? pocketBaseGuestbookApi.listMessages(params)
    : apiClient.request<PageResult<GuestbookMessage>>("/api/guestbook", {
        query: params,
        auth: false,
      }),

  /**
   * 创建留言
   * @param payload - 留言内容载荷
   */
  createMessage: (payload: GuestbookPayload) => pocketBaseEnabled
    ? pocketBaseGuestbookApi.createMessage(payload)
    : apiClient.request<PublicSubmissionResult>("/api/guestbook", {
        method: "POST",
        body: payload,
        auth: false,
      }),
};

/** 笔记相关 API */
export const notesApi = {
  /** 获取公开笔记列表 */
  listNotes: (params: { page?: number; limit?: number } = {}) => pocketBaseEnabled
    ? pocketBaseNotesApi.listNotes(params)
    : apiClient.request<PageResult<NoteItem>>("/api/notes", { query: params, auth: false }),
};

/** 项目相关 API */
export const projectsApi = {
  /** 获取公开项目列表 */
  listProjects: (params: { page?: number; limit?: number } = {}) => pocketBaseEnabled
    ? pocketBaseProjectsApi.listProjects(params)
    : apiClient.request<PageResult<ProjectItem>>("/api/projects", { query: params, auth: false }),
};

/** 站点配置相关 API */
export const siteConfigApi = {
  /** 获取站点全局配置 */
  getConfig: () => pocketBaseEnabled
    ? pocketBaseSiteConfigApi.getConfig()
    : apiClient.request<SiteConfig>("/api/config", { auth: false }),
};

/** 背景音乐公开接口：后端后续只需返回启用状态、默认音量和可播放曲目地址。 */
export const musicApi = {
  getConfig: () => pocketBaseEnabled
    ? pocketBaseMusicApi.getConfig()
    : apiClient.request<MusicConfig>("/api/music", { auth: false }),
};
