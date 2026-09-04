import { flushPromises, mount } from '@vue/test-utils'
import { beforeEach, describe, expect, it, vi } from 'vitest'
import { createMemoryHistory, createRouter } from 'vue-router'

import { postsApi } from '@/api'
import PostsView from './PostsView.vue'

vi.mock('@/api', () => ({
  postsApi: {
    listPosts: vi.fn(),
    listTags: vi.fn(),
  },
}))

const emptyPage = { data: [], page: 1, limit: 10, total: 0, total_pages: 0, has_more: false }

describe('PostsView', () => {
  beforeEach(() => {
    vi.mocked(postsApi.listPosts).mockReset()
    vi.mocked(postsApi.listTags).mockReset()
  })

  it('shows an empty article state instead of an example article', async () => {
    vi.mocked(postsApi.listPosts).mockResolvedValue(emptyPage)
    vi.mocked(postsApi.listTags).mockResolvedValue({ data: [] })

    const router = createRouter({ history: createMemoryHistory(), routes: [{ path: '/posts', component: PostsView }] })
    await router.push('/posts')
    await router.isReady()
    const wrapper = mount(PostsView, {
      global: {
        plugins: [router],
        stubs: { PublicLayout: { template: '<div><slot /></div>' }, RouterLink: true },
      },
    })
    await flushPromises()

    expect(postsApi.listPosts).toHaveBeenCalledWith(expect.objectContaining({ contentType: 'article' }))
    expect(postsApi.listTags).toHaveBeenCalledWith('article')
    expect(wrapper.text()).toContain('暂无发布文章')
    expect(wrapper.text()).not.toContain('前端从静态页走向工程化')
  })

  it('uses cover cards to display published articles', async () => {
    vi.mocked(postsApi.listPosts).mockResolvedValue({
      data: [{ id: 1, title: '接口文档', slug: 'api-docs', summary: '整理公开接口。', cover_url: '/cover.png', tags: ['文档'], views: 3, created_at: '2026-09-04' }],
      page: 1, limit: 10, total: 1, total_pages: 1, has_more: false,
    })
    vi.mocked(postsApi.listTags).mockResolvedValue({ data: [{ name: '文档', post_count: 1 }] })

    const router = createRouter({ history: createMemoryHistory(), routes: [{ path: '/posts', component: PostsView }] })
    await router.push('/posts')
    await router.isReady()
    const wrapper = mount(PostsView, {
      global: {
        plugins: [router],
        stubs: { PublicLayout: { template: '<div><slot /></div>' } },
      },
    })
    await flushPromises()

    expect(wrapper.findAll('.article-card')).toHaveLength(1)
    expect(wrapper.get('.article-card-cover h2').text()).toBe('接口文档')
    expect(wrapper.get('.article-category').text()).toBe('文档')
    expect(wrapper.get('.article-card').attributes('href')).toBe('/posts/1')
    expect(wrapper.get('.article-card-body footer').text()).toContain('2026-09-04')
    expect(wrapper.get('.article-card-body footer').text()).toContain('3')
  })

  it('uses an in-site cloud cover when an article has no cover', async () => {
    vi.mocked(postsApi.listPosts).mockResolvedValue({
      data: [{ id: 2, title: 'C++笔记', slug: 'cpp-note', summary: '', tags: [], views: 0, created_at: '2026-09-03' }],
      page: 1, limit: 10, total: 1, total_pages: 1, has_more: false,
    })
    vi.mocked(postsApi.listTags).mockResolvedValue({ data: [] })

    const router = createRouter({ history: createMemoryHistory(), routes: [{ path: '/posts', component: PostsView }, { path: '/posts/:id', component: { template: '<div />' } }] })
    await router.push('/posts')
    await router.isReady()
    const wrapper = mount(PostsView, {
      global: { plugins: [router], stubs: { PublicLayout: { template: '<div><slot /></div>' } } },
    })
    await flushPromises()

    expect(wrapper.get('.article-card-cover img').attributes('src')).toBe('/clouds/cloud-day.webp')
    expect(wrapper.get('.article-category').text()).toBe('文章')
  })
})
