import { flushPromises, mount } from '@vue/test-utils'
import { beforeEach, describe, expect, it, vi } from 'vitest'

import { projectsApi } from '@/api'
import ProjectsView from './ProjectsView.vue'

vi.mock('@/api', () => ({ projectsApi: { listProjects: vi.fn() } }))

describe('ProjectsView', () => {
  beforeEach(() => vi.mocked(projectsApi.listProjects).mockReset())

  it('shows an empty state without invented projects', async () => {
    vi.mocked(projectsApi.listProjects).mockResolvedValue({ data: [], page: 1, limit: 10, total: 0, total_pages: 0, has_more: false })
    const wrapper = mount(ProjectsView, { global: { stubs: { PublicLayout: { template: '<div><slot /></div>' } } } })
    await flushPromises()
    expect(wrapper.text()).toContain('暂无公开项目')
    expect(wrapper.text()).not.toContain('博客前端重构')
  })
})
