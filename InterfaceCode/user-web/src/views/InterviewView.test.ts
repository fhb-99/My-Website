import { flushPromises, mount } from '@vue/test-utils'
import { beforeEach, describe, expect, it, vi } from 'vitest'
import { createMemoryHistory, createRouter } from 'vue-router'

import { learningRoadmapApi, postsApi } from '@/api'
import InterviewView from './InterviewView.vue'

vi.mock('@/api', () => ({
  postsApi: {
    listPosts: vi.fn(),
    listInterviewDirectory: vi.fn(),
  },
  learningRoadmapApi: { get: vi.fn() },
}))

describe('InterviewView', () => {
  beforeEach(() => {
    vi.mocked(postsApi.listPosts).mockResolvedValue({ data: [], page: 1, limit: 10, total: 0, total_pages: 0, has_more: false })
    vi.mocked(postsApi.listInterviewDirectory).mockResolvedValue({
      data: [{ name: '计算机网络', post_count: 1, posts: [{ id: 'network-1', title: 'TCP 三次握手', slug: 'tcp', summary: '', tags: [], views: 0, created_at: '', content_type: 'interview', interview_category: '计算机网络' }] }],
    })
    vi.mocked(learningRoadmapApi.get).mockResolvedValue({
      id: 'roadmap-1',
      title: 'C++学习路线 - 从C++入门到入土',
      subtitle: '从入门到工程实践',
      cover_url: '/roadmap.png',
      content_html: '<h1>路线</h1>',
      updated_at: '',
    })
  })

  it('shows the topic sidebar and independent learning roadmap by default', async () => {
    const router = createRouter({ history: createMemoryHistory(), routes: [
      { path: '/interview', component: InterviewView },
      { path: '/learning-roadmap', component: { template: '<div>学习路线详情</div>' } },
    ] })
    await router.push('/interview')
    await router.isReady()
    const wrapper = mount(InterviewView, {
      global: {
        plugins: [router],
        stubs: { PublicLayout: { template: '<div><slot /></div>' } },
      },
    })
    await flushPromises()

    expect(postsApi.listPosts).toHaveBeenCalledWith(expect.objectContaining({ contentType: 'interview' }))
    expect(postsApi.listInterviewDirectory).toHaveBeenCalled()
    expect(learningRoadmapApi.get).toHaveBeenCalled()
    expect(wrapper.get('[aria-label="八股文主题目录"]').text()).toContain('计算机网络')
    expect(wrapper.find('[aria-label="八股文子目录"]').exists()).toBe(false)
    expect(wrapper.get('.interview-landing-image').attributes('src')).toBe('/roadmap.png')
    expect(wrapper.get('.learning-roadmap-card').text()).toContain('C++学习路线 - 从C++入门到入土')

    await wrapper.get('.learning-roadmap-card').trigger('click')
    await flushPromises()
    expect(router.currentRoute.value.path).toBe('/learning-roadmap')
    await router.push('/interview')
    await flushPromises()

    const networkTopic = wrapper.findAll('button').find((button) => button.text().includes('计算机网络'))
    expect(networkTopic).toBeTruthy()
    await networkTopic!.trigger('click')
    await flushPromises()

    expect(router.currentRoute.value.query.category).toBe('计算机网络')
    expect(wrapper.get('.topic-files.expanded').text()).toContain('TCP 三次握手')
    expect(postsApi.listPosts).toHaveBeenLastCalledWith(expect.objectContaining({ category: '计算机网络', contentType: 'interview' }))
    expect(wrapper.text()).toContain('暂无发布内容')
    expect(wrapper.find('.learning-roadmap-card').exists()).toBe(false)

    const directoryToggle = wrapper.get('.knowledge-topics-heading')
    expect(directoryToggle.attributes('aria-expanded')).toBe('true')
    await directoryToggle.trigger('click')
    expect(directoryToggle.attributes('aria-expanded')).toBe('false')
  })
})
