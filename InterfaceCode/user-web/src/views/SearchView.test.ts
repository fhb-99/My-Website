import { flushPromises, mount } from '@vue/test-utils'
import { beforeEach, describe, expect, it, vi } from 'vitest'
import { createMemoryHistory, createRouter } from 'vue-router'

import { notesApi, postsApi, projectsApi } from '@/api'
import SearchView from './SearchView.vue'

vi.mock('@/api', () => ({
  notesApi: { listNotes: vi.fn() },
  postsApi: { listPosts: vi.fn() },
  projectsApi: { listProjects: vi.fn() },
}))

describe('SearchView', () => {
  beforeEach(() => {
    vi.mocked(notesApi.listNotes).mockReset()
    vi.mocked(postsApi.listPosts).mockReset()
    vi.mocked(projectsApi.listProjects).mockReset()
  })

  it('searches and displays every supported public content type', async () => {
    vi.mocked(postsApi.listPosts).mockResolvedValue({
      data: [{ id: 1, title: 'WebRTC 笔记', slug: 'webrtc', summary: '媒体链路整理', tags: [], views: 0, created_at: '2026-09-25', content_type: 'article' }],
      page: 1, limit: 50, total: 1, total_pages: 1, has_more: false,
    })
    vi.mocked(projectsApi.listProjects).mockResolvedValue({
      data: [{ id: 2, name: 'SyncRTC', summary: '实时音视频项目', tags: [], created_at: '2026-09-25' }],
      page: 1, limit: 50, total: 1, total_pages: 1, has_more: false,
    })
    vi.mocked(notesApi.listNotes).mockResolvedValue({
      data: [{ id: 3, content: '整理 WebRTC 媒体传输流程', created_at: '2026-09-25' }],
      page: 1, limit: 50, total: 1, total_pages: 1, has_more: false,
    })

    const router = createRouter({ history: createMemoryHistory(), routes: [{ path: '/search', component: SearchView }] })
    await router.push('/search?q=WebRTC')
    await router.isReady()
    const wrapper = mount(SearchView, {
      global: { plugins: [router], stubs: { PublicLayout: { template: '<div><slot name="title" /><slot name="subtitle" /><slot /></div>' } } },
    })
    await flushPromises()

    expect(postsApi.listPosts).toHaveBeenCalledWith({ page: 1, limit: 50, q: 'WebRTC' })
    expect(projectsApi.listProjects).toHaveBeenCalledWith({ page: 1, limit: 50, q: 'WebRTC' })
    expect(notesApi.listNotes).toHaveBeenCalledWith({ page: 1, limit: 50, q: 'WebRTC' })
    expect(wrapper.text()).toContain('找到 3 条')
    expect(wrapper.text()).toContain('WebRTC 笔记')
    expect(wrapper.text()).toContain('SyncRTC')
    expect(wrapper.text()).toContain('整理 WebRTC 媒体传输流程')
  })
})
