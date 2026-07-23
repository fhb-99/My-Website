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

    expect(wrapper.text()).toContain('暂无发布文章')
    expect(wrapper.text()).not.toContain('前端从静态页走向工程化')
  })
})
