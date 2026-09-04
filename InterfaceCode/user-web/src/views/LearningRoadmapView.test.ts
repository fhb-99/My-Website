import { flushPromises, mount } from '@vue/test-utils'
import { beforeEach, describe, expect, it, vi } from 'vitest'
import { createMemoryHistory, createRouter } from 'vue-router'

import { learningRoadmapApi } from '@/api'
import LearningRoadmapView from './LearningRoadmapView.vue'

vi.mock('@/api', () => ({ learningRoadmapApi: { get: vi.fn() } }))

describe('LearningRoadmapView', () => {
  beforeEach(() => {
    vi.mocked(learningRoadmapApi.get).mockResolvedValue({
      id: 'roadmap-1',
      title: 'C++学习路线 - 从C++入门到入土',
      subtitle: '从基础语法到工程实践',
      cover_url: '/roadmap.png',
      content_html: '<h2>第一阶段</h2><p>掌握基础语法。</p>',
      updated_at: '',
    })
  })

  it('loads and renders the independent roadmap content', async () => {
    const router = createRouter({ history: createMemoryHistory(), routes: [
      { path: '/learning-roadmap', component: LearningRoadmapView },
      { path: '/interview', component: { template: '<div>八股文</div>' } },
    ] })
    await router.push('/learning-roadmap')
    await router.isReady()
    const wrapper = mount(LearningRoadmapView, {
      global: {
        plugins: [router],
        stubs: { PublicLayout: { template: '<div><slot /></div>' } },
      },
    })
    await flushPromises()

    expect(learningRoadmapApi.get).toHaveBeenCalledOnce()
    expect(wrapper.get('h1').text()).toBe('C++学习路线 - 从C++入门到入土')
    expect(wrapper.get('.roadmap-cover').attributes('src')).toBe('/roadmap.png')
    expect(wrapper.get('.roadmap-content').text()).toContain('掌握基础语法')
  })
})
