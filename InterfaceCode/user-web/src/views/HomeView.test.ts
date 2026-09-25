import { flushPromises, mount } from '@vue/test-utils'
import { createPinia } from 'pinia'
import { beforeEach, describe, expect, it, vi } from 'vitest'
import { createMemoryHistory, createRouter } from 'vue-router'

import { notesApi, postsApi, siteConfigApi } from '@/api'
import HomeView from './HomeView.vue'

vi.mock('@/api', () => ({
  notesApi: { listNotes: vi.fn() },
  postsApi: { listPosts: vi.fn() },
  siteConfigApi: { getConfig: vi.fn() },
}))

const emptyPage = { data: [], page: 1, limit: 10, total: 0, total_pages: 0, has_more: false }

describe('HomeView search', () => {
  beforeEach(() => {
    vi.mocked(notesApi.listNotes).mockResolvedValue(emptyPage)
    vi.mocked(postsApi.listPosts).mockResolvedValue(emptyPage)
    vi.mocked(siteConfigApi.getConfig).mockResolvedValue({ title: '博客', subtitle: '', announcement: '' })
  })

  it('searches the site by default and uses Bing for web search', async () => {
    const router = createRouter({
      history: createMemoryHistory(),
      routes: [
        { path: '/home', component: HomeView },
        { path: '/search', component: { template: '<div />' } },
      ],
    })
    await router.push('/home')
    await router.isReady()
    const wrapper = mount(HomeView, {
      global: { plugins: [createPinia(), router], stubs: { SiteHeader: true } },
    })
    await flushPromises()

    await wrapper.get('.portal-search input').setValue('WebRTC')
    await wrapper.get('.portal-search').trigger('submit')
    await flushPromises()
    expect(router.currentRoute.value.fullPath).toBe('/search?q=WebRTC')

    const open = vi.spyOn(window, 'open').mockImplementation(() => null)
    await router.push('/home')
    await wrapper.get('.search-engine select').setValue('web')
    await wrapper.get('.portal-search input').setValue('C++ 音视频')
    await wrapper.get('.portal-search').trigger('submit')
    expect(open).toHaveBeenCalledWith('https://www.bing.com/search?q=C%2B%2B+%E9%9F%B3%E8%A7%86%E9%A2%91', '_blank', 'noopener,noreferrer')
    wrapper.unmount()
  })
})
