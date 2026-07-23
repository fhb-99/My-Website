import { flushPromises, mount } from '@vue/test-utils'
import { beforeEach, describe, expect, it, vi } from 'vitest'

import { notesApi } from '@/api'
import NotesView from './NotesView.vue'

vi.mock('@/api', () => ({ notesApi: { listNotes: vi.fn() } }))

describe('NotesView', () => {
  beforeEach(() => vi.mocked(notesApi.listNotes).mockReset())

  it('shows an empty state without invented notes', async () => {
    vi.mocked(notesApi.listNotes).mockResolvedValue({ data: [], page: 1, limit: 10, total: 0, total_pages: 0, has_more: false })
    const wrapper = mount(NotesView, { global: { stubs: { PublicLayout: { template: '<div><slot /></div>' } } } })
    await flushPromises()
    expect(wrapper.text()).toContain('暂无公开随记')
    expect(wrapper.text()).not.toContain('今天把页面')
  })
})
