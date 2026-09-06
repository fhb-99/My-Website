import { flushPromises, mount, type VueWrapper } from '@vue/test-utils'
import { createPinia } from 'pinia'
import { createMemoryHistory, createRouter } from 'vue-router'
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest'

import { commentsApi, postsApi } from '@/api'
import PostDetailView from './PostDetailView.vue'

vi.mock('@/api', () => ({
  commentsApi: {
    createComment: vi.fn(),
    listComments: vi.fn(),
  },
  postsApi: {
    getNavigation: vi.fn(),
    getPostDetail: vi.fn(),
    recordPostView: vi.fn(),
  },
}))

const post = {
  id: 12,
  title: '测试文章',
  slug: 'test-post',
  summary: '用于验证阅读页行为。',
  tags: ['测试'],
  views: 8,
  created_at: '2026-09-04',
  updated_at: '2026-09-04',
  content_html: '<h2>开始</h2><p>正文</p><h3>细节</h3><h4>补充方法</h4>',
}

const emptyNavigation = { previous: null, next: null }
const emptyComments = { data: [], page: 1, limit: 20, total: 0, total_pages: 0, has_more: false }
let mountedWrapper: VueWrapper | undefined

async function mountPostDetail() {
  const router = createRouter({
    history: createMemoryHistory(),
    routes: [
      { path: '/posts', component: { template: '<div />' } },
      { path: '/posts/:id', component: PostDetailView },
    ],
  })
  await router.push('/posts/12')
  await router.isReady()

  const wrapper = mount(PostDetailView, {
    attachTo: document.body,
    global: {
      plugins: [createPinia(), router],
      stubs: {
        EmailGate: true,
        PublicLayout: { template: '<div><slot name="title" /><slot name="subtitle" /><slot /></div>' },
      },
    },
  })
  mountedWrapper = wrapper
  await flushPromises()
  return wrapper
}

describe('PostDetailView', () => {
  beforeEach(() => {
    vi.mocked(commentsApi.createComment).mockReset()
    vi.mocked(commentsApi.listComments).mockResolvedValue(emptyComments)
    vi.mocked(postsApi.getNavigation).mockResolvedValue(emptyNavigation)
    vi.mocked(postsApi.getPostDetail).mockResolvedValue(post)
    vi.mocked(postsApi.recordPostView).mockResolvedValue({ counted: false })
  })

  afterEach(() => {
    mountedWrapper?.unmount()
    mountedWrapper = undefined
    document.body.replaceChildren()
    vi.useRealTimers()
    vi.restoreAllMocks()
  })

  it('点击目录时滚动到标题且不改写 Hash 路由', async () => {
    const scrollIntoView = vi.fn()
    Object.defineProperty(HTMLElement.prototype, 'scrollIntoView', { configurable: true, value: scrollIntoView })
    window.location.hash = '#/posts/12'

    const wrapper = await mountPostDetail()
    expect(wrapper.get('.toc-level-4').text()).toBe('补充方法')
    await wrapper.get('.toc button').trigger('click')

    expect(scrollIntoView).toHaveBeenCalledWith({ behavior: 'smooth', block: 'start' })
    expect(window.location.hash).toBe('#/posts/12')
  })

  it('导航接口失败时仍上报阅读并保留文章内容', async () => {
    vi.useFakeTimers()
    vi.mocked(postsApi.getNavigation).mockRejectedValue(new Error('导航接口不可用'))

    const wrapper = await mountPostDetail()
    await vi.advanceTimersByTimeAsync(10_000)

    expect(wrapper.text()).toContain('测试文章')
    expect(postsApi.recordPostView).toHaveBeenCalledWith(12)
  })

  it('阅读上报首次计数成功后更新当前阅读量', async () => {
    vi.useFakeTimers()
    vi.mocked(postsApi.recordPostView).mockResolvedValue({ counted: true })

    const wrapper = await mountPostDetail()
    await vi.advanceTimersByTimeAsync(10_000)
    await flushPromises()

    expect(wrapper.text()).toContain('阅读 9')
  })
})
