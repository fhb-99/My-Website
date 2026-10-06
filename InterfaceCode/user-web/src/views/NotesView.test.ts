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

  it('keeps note line breaks and displays the creation time only to seconds', async () => {
    vi.mocked(notesApi.listNotes).mockResolvedValue({
      data: [{ id: 'note-1', content: '第一段\n\n第二段', mood: '后悔', created_at: '2026-10-06 14:50:18.587Z' }],
      page: 1,
      limit: 10,
      total: 1,
      total_pages: 1,
      has_more: false,
    })
    const wrapper = mount(NotesView, { global: { stubs: { PublicLayout: { template: '<div><slot /></div>' } } } })
    await flushPromises()

    expect(wrapper.get('.note-content').element.textContent).toBe('第一段\n\n第二段')
    expect(wrapper.get('.mini-meta').text()).toBe('2026-10-06 14:50:18 · 后悔')
  })
})
